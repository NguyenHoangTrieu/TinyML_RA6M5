/**
 * @file    bsp_zmod4410.c
 * @brief   ZMOD4410 gas sensor driver – full NVM-init + simplified TVOC.
 *
 * Root-cause fix summary (vs. previous version):
 *   OLD: Read PID → immediately TRIGGER → poll STATUS → TIMEOUT (always)
 *   NEW: Stop → wait idle → Read PID → Read CONF (6 B) → Read PROD_DATA (7 B)
 *        → write init H/D/M/S blocks → run init sequence → read R trim
 *        → write meas H/D/M/S blocks → TRIGGER → poll STATUS → read ADC
 *        → compute Rmox → simplified TVOC estimate
 *
 * Register map (from datasheet + Renesas FSP rm_zmod4xxx.c):
 *   0x00  PID        (2 B, expect 0x2310)
 *   0x20  CONF       (6 B, sensor calibration)
 *   0x40  H register (heater setpoint – written by driver)
 *   0x50  D register (delay timing)
 *   0x60  M register (measurement mode)
 *   0x68  S register (sequencer)
 *   0x93  CMD        (0x00=stop, 0x80=start)
 *   0x94  STATUS     (bit7=sequencer running)
 *   0x97  R/ADC      (result data)
 *   0xB4  PROD_DATA  (7 B, NVM production trim)
 *   0xB7  DEV_ERR    (device error flags)
 */

#include "bsp_zmod4410.h"
#include "debug_print.h"
#include "GPIO.h"
#include "kernel.h"
#include <stdint.h>
#include <string.h>
#include <math.h>

/* -----------------------------------------------------------------------
 * Register addresses (confirmed from Renesas FSP source + datasheet)
 * ----------------------------------------------------------------------- */
#define REG_PID         0x00U  /* Product ID, 2 bytes                        */
#define REG_CONF        0x20U  /* Sensor config block, 6 bytes               */
#define REG_H           0x40U  /* Heater setpoint register                   */
#define REG_D           0x50U  /* Delay register                             */
#define REG_M           0x60U  /* Measurement register                       */
#define REG_S           0x68U  /* Sequencer register                         */
#define REG_CMD         0x93U  /* Command register                           */
#define REG_STATUS      0x94U  /* Status register                            */
#define REG_ADC         0x97U  /* ADC result register                        */
#define REG_PROD_DATA   0xB4U  /* Production/NVM calibration data, 7 bytes   */
#define REG_DEV_ERR     0xB7U  /* Device error flags                         */

#define STATUS_SEQUENCER_RUNNING  (1U << 7U)  /* 1 = busy                   */
#define CMD_STOP        0x00U
#define CMD_START       0x80U

/* IAQ 2nd Gen mode data blocks (from Renesas ZMOD4410 library).
 * These are the fixed "recipe" bytes that configure the sensor's internal
 * ASIC for IAQ 2nd Generation mode.  They are public in every Renesas
 * example / FSP release under ra/fsp/src/rm_zmod4xxx/iaq_2nd_gen/.
 *
 * init_conf: one-time calibration run (writes H, D, M, S then starts)
 * meas_conf: continuous measurement run (writes H, D, M, S)
 */

/* --- init configuration --- */
#define INIT_H_ADDR     0x40U
static const uint8_t INIT_H_DATA[] = { 0x00, 0x50 };
#define INIT_H_LEN      2U

#define INIT_D_ADDR     0x50U
static const uint8_t INIT_D_DATA[] = { 0x00, 0x28, 0xC8 };
#define INIT_D_LEN      3U

#define INIT_M_ADDR     0x60U
static const uint8_t INIT_M_DATA[] = { 0xC3, 0xE3 };
#define INIT_M_LEN      2U

#define INIT_S_ADDR     0x68U
static const uint8_t INIT_S_DATA[] = { 0x00, 0x00, 0x80 };
#define INIT_S_LEN      3U

#define INIT_R_ADDR     0x97U
#define INIT_R_LEN      4U   /* mox_lr (2 B) + mox_er (2 B)                 */
#define INIT_CMD_START  0x80U

/* --- measurement configuration (IAQ 2nd Gen) --- */
#define MEAS_H_ADDR     0x40U
static const uint8_t MEAS_H_DATA[] = {
    0x00, 0xF0, 0x01, 0xE0, 0x03, 0xC0, 0x07, 0x00,
    0x0E, 0x00, 0x1C, 0x00, 0x39, 0x81, 0x73, 0x01
};
#define MEAS_H_LEN      16U

#define MEAS_D_ADDR     0x50U
static const uint8_t MEAS_D_DATA[] = {
    0x00, 0x28, 0xC8, 0xE5, 0x28, 0xC8, 0xE5, 0x28,
    0xC8, 0xE5, 0x28, 0xC8, 0xE5, 0x28, 0xC8, 0xE5,
    0x28, 0xC8, 0xE5
};
#define MEAS_D_LEN      19U

#define MEAS_M_ADDR     0x60U
static const uint8_t MEAS_M_DATA[] = {
    0xFE, 0xAA, 0xFE, 0xAA, 0xFE, 0xAA, 0xFE, 0xAA,
    0xFE, 0xAA, 0xFE, 0xAA, 0xFE, 0xAA, 0xFE, 0xAA,
    0xFE
};
#define MEAS_M_LEN      17U

#define MEAS_S_ADDR     0x68U
static const uint8_t MEAS_S_DATA[] = {
    0x00, 0x00, 0x00, 0x08, 0x00, 0x10, 0x00, 0x01,
    0x00, 0x01, 0x00, 0x09, 0x00, 0x11, 0x00, 0x01,
    0x00, 0x00
};
#define MEAS_S_LEN      18U

#define MEAS_R_ADDR     0x97U
#define MEAS_R_LEN      18U  /* 9 steps × 2 bytes ADC                       */
#define MEAS_CMD_START  0x80U

/* Production data length for IAQ 2nd Gen */
#define PROD_DATA_LEN   7U

/* RESET pin */
#define ZMOD4410_RESET_PORT  GPIO_PORT3
#define ZMOD4410_RESET_PIN   7U

/* Number of status poll attempts during init (60 × 50 ms = 3 s max per call) */
#define INIT_POLL_TIMEOUT    60U
/* Number of status poll attempts during measurement (100 × 100 ms = 10 s) */
#define MEAS_POLL_TIMEOUT   100U

/* Internal state */
static ZMOD4410_OpMode_t g_operation_mode   = IAQ_2ND_GEN;
static uint32_t          g_measurement_count = 0U;
static float             g_temp_c            = 23.0f;
static float             g_humidity_pct      = 50.0f;

/* Sensor calibration trim read back during init */
static uint16_t          g_mox_lr = 0U;
static uint16_t          g_mox_er = 32768U;  /* safe default */

/* Sensor config block (6 bytes) read from NVM */
static uint8_t           g_config[6] = {0};
/* Production data (7 bytes) read from NVM */
static uint8_t           g_prod_data[PROD_DATA_LEN] = {0};

/* -----------------------------------------------------------------------
 * Helpers
 * ----------------------------------------------------------------------- */
static void zmod4410_delay_ms(uint32_t ms)
{
    if ((os_current_task != (OS_TCB_t *)0) && (ms != 0U))
    {
        OS_Task_Delay(ms);
        return;
    }
    volatile uint32_t n = ms * 200000U;
    while (n-- != 0U) { __asm volatile("nop"); }
}

static void zmod4410_reset_pin_init(void)
{
    GPIO_Config(ZMOD4410_RESET_PORT, ZMOD4410_RESET_PIN, GPIO_CNF_OUT_PP, GPIO_MODE_OUTPUT);
    GPIO_Write_Pin(ZMOD4410_RESET_PORT, ZMOD4410_RESET_PIN, GPIO_PIN_SET);
}

/* -----------------------------------------------------------------------
 * Low-level I2C primitives
 * ----------------------------------------------------------------------- */
static uint8_t zmod4410_read_reg(I2C_t i2c, uint8_t reg, uint8_t *buf, uint8_t len)
{
    if ((buf == NULL) || (len == 0U)) { return 0U; }

    I2C_Start(i2c);
    if (!I2C_Transmit_Address(i2c, ZMOD4410_I2C_ADDR, I2C_WRITE))
    { I2C_Stop(i2c); return 0U; }
    if (!I2C_Master_Transmit_Data(i2c, &reg, 1U))
    { I2C_Stop(i2c); return 0U; }
    I2C_Stop(i2c);

    I2C_Start(i2c);
    if (!I2C_Transmit_Address(i2c, ZMOD4410_I2C_ADDR, I2C_READ))
    { I2C_Stop(i2c); return 0U; }
    if (!I2C_Master_Receive_Data(i2c, buf, len))
    { return 0U; } /* I2C_Stop called inside */
    return len;
}

static uint8_t zmod4410_write_reg(I2C_t i2c, uint8_t reg, const uint8_t *data, uint8_t len)
{
    /* max payload: 1 reg byte + up to 20 data bytes */
    uint8_t buf[21];
    if ((len == 0U) || ((len + 1U) > (uint8_t)sizeof(buf))) { return 0U; }

    buf[0] = reg;
    memcpy(&buf[1], data, len);

    I2C_Start(i2c);
    if (!I2C_Transmit_Address(i2c, ZMOD4410_I2C_ADDR, I2C_WRITE))
    { I2C_Stop(i2c); return 0U; }
    if (!I2C_Master_Transmit_Data(i2c, buf, (uint8_t)(len + 1U)))
    { I2C_Stop(i2c); return 0U; }
    I2C_Stop(i2c);
    return len;
}

/* Write CMD byte (single-byte convenience) */
static ZMOD4410_Status_t zmod4410_write_cmd(I2C_t i2c, uint8_t cmd)
{
    uint8_t val = cmd;
    if (zmod4410_write_reg(i2c, REG_CMD, &val, 1U) == 0U)
    {
        return ZMOD4410_ERR_NACK;
    }
    return ZMOD4410_OK;
}

/* Poll STATUS until sequencer is idle.
 * Returns ZMOD4410_OK when idle, ZMOD4410_ERR_TIMEOUT if poll_limit exceeded. */
static ZMOD4410_Status_t zmod4410_wait_idle(I2C_t i2c, uint32_t poll_limit, uint32_t interval_ms)
{
    uint32_t count = 0U;
    uint8_t  status;

    while (count < poll_limit)
    {
        if (zmod4410_read_reg(i2c, REG_STATUS, &status, 1U) == 0U)
        {
            return ZMOD4410_ERR_NACK;
        }
        if ((status & STATUS_SEQUENCER_RUNNING) == 0U)
        {
            return ZMOD4410_OK;
        }
        zmod4410_delay_ms(interval_ms);
        count++;
    }
    return ZMOD4410_ERR_TIMEOUT;
}

/* -----------------------------------------------------------------------
 * zmod4410_calc_hsp
 * Compute heater setpoint bytes from raw H data + config[].
 * Formula from Renesas FSP rm_zmod4xxx_calc_factor():
 *   hspf = (-(config[2]*256 + config[3]) *
 *            ((config[4] + 640) * (config[5] + h_raw) - 512000)) / 12288000
 * Result is a 16-bit fixed point split into two bytes (MSB, LSB).
 * ----------------------------------------------------------------------- */
static void zmod4410_calc_hsp(const uint8_t *h_raw_data, uint8_t h_len,
                               const uint8_t *cfg, uint8_t *hsp_out)
{
    uint8_t i;
    for (i = 0U; i < h_len; i += 2U)
    {
        int16_t h_raw = (int16_t)(((uint16_t)h_raw_data[i] << 8U) | h_raw_data[i + 1U]);
        float hspf = (-((float)cfg[2] * 256.0f + (float)cfg[3]) *
                       (((float)cfg[4] + 640.0f) * ((float)cfg[5] + (float)h_raw) -
                        512000.0f)) / 12288000.0f;
        uint16_t hsp16 = (uint16_t)hspf;
        hsp_out[i]     = (uint8_t)(hsp16 >> 8U);
        hsp_out[i + 1U] = (uint8_t)(hsp16 & 0xFFU);
    }
}

/* -----------------------------------------------------------------------
 * Simplified TVOC estimate from Rmox
 *
 * Without the proprietary Renesas algorithm library, use a log-linear
 * approximation based on published ZMOD4410 application notes:
 *   TVOC (ppb) ≈ 10^((log10(Rmox_baseline) - log10(Rmox)) * sensitivity + offset)
 *
 * Where Rmox (Ω) = (raw_adc / 32768.0) * mox_er + mox_lr  (simplified)
 *
 * Constants are typical values — replace with trained coefficients once
 * the Renesas library is available.
 * ----------------------------------------------------------------------- */
static float zmod4410_rmox_from_adc(uint16_t raw_adc)
{
    /* Avoid division by zero */
    if (g_mox_er == 0U) { return 1000.0f; }
    float rmox = ((float)raw_adc / 32768.0f) * (float)g_mox_er + (float)g_mox_lr;
    if (rmox < 1.0f) { rmox = 1.0f; }
    return rmox;
}

static float zmod4410_tvoc_from_rmox(float rmox_ohm)
{
    /* Baseline resistance in clean air (typical ~50 kΩ for ZMOD4410).
     * Replace with actual measured baseline for your deployment. */
    const float RMOX_BASELINE = 50000.0f;
    const float SENSITIVITY   = 3.0f;   /* slope in log space */
    const float OFFSET_PPB    = 50.0f;  /* TVOC at baseline   */

    if (rmox_ohm <= 0.0f) { return 0.0f; }

    float log_ratio = (logf(RMOX_BASELINE) - logf(rmox_ohm)) / logf(10.0f);
    float tvoc = OFFSET_PPB * powf(10.0f, log_ratio * SENSITIVITY);
    if (tvoc < 0.0f) { tvoc = 0.0f; }
    if (tvoc > 10000.0f) { tvoc = 10000.0f; }
    return tvoc;
}

/* -----------------------------------------------------------------------
 * ZMOD4410_HardReset
 * ----------------------------------------------------------------------- */
ZMOD4410_Status_t ZMOD4410_HardReset(void)
{
    zmod4410_reset_pin_init();
    GPIO_Write_Pin(ZMOD4410_RESET_PORT, ZMOD4410_RESET_PIN, GPIO_PIN_RESET);
    zmod4410_delay_ms(5U);
    GPIO_Write_Pin(ZMOD4410_RESET_PORT, ZMOD4410_RESET_PIN, GPIO_PIN_SET);
    zmod4410_delay_ms(50U);  /* sensor needs ~50 ms after reset */
    return ZMOD4410_OK;
}

/* -----------------------------------------------------------------------
 * ZMOD4410_Init — full NVM init sequence
 *
 * Steps (matching Renesas FSP rm_zmod4xxx_configuration):
 *  1. Hard reset
 *  2. CMD = stop → wait idle
 *  3. Read PID → verify 0x2310
 *  4. Read CONF (6 B) → g_config[]
 *  5. Read PROD_DATA (7 B) → g_prod_data[]
 *  6. Compute HSP for init_conf H data
 *  7. Write init H, D, M, S blocks
 *  8. CMD = start_init → wait idle → read R trim (mox_lr, mox_er)
 *  9. Compute HSP for meas_conf H data
 * 10. Write meas H, D, M, S blocks
 * ----------------------------------------------------------------------- */
ZMOD4410_Status_t ZMOD4410_Init(I2C_t i2c, ZMOD4410_OpMode_t operation_mode)
{
    uint8_t           pid_raw[2];
    uint16_t          pid;
    uint8_t           hsp[32];  /* big enough for any H block */
    uint8_t           r_trim[INIT_R_LEN];
    ZMOD4410_Status_t st;

    g_operation_mode    = operation_mode;
    g_measurement_count = 0U;

    /* Step 1 — hard reset */
    debug_print("[ZMOD_Init] Step1 HardReset\r\n");
    (void)ZMOD4410_HardReset();

    /* Step 2 — stop any running measurement, wait idle */
    debug_print("[ZMOD_Init] Step2 Stop+idle\r\n");
    st = zmod4410_write_cmd(i2c, CMD_STOP);
    if (st != ZMOD4410_OK) { debug_print("[ZMOD_Init] Stop NACK\r\n"); return st; }
    zmod4410_delay_ms(10U);
    st = zmod4410_wait_idle(i2c, INIT_POLL_TIMEOUT, 50U);
    if (st != ZMOD4410_OK) { debug_print("[ZMOD_Init] Stop idle timeout (ignore)\r\n"); }

    /* Step 3 — read & verify PID */
    debug_print("[ZMOD_Init] Step3 PID\r\n");
    if (zmod4410_read_reg(i2c, REG_PID, pid_raw, 2U) == 0U)
    { debug_print("[ZMOD_Init] PID NACK\r\n"); return ZMOD4410_ERR_NACK; }
    pid = (uint16_t)(((uint16_t)pid_raw[0] << 8U) | pid_raw[1]);
    debug_print("[ZMOD_Init] PID=0x%X\r\n", (unsigned)pid);
    if (pid != ZMOD4410_PRODUCT_ID)
    { debug_print("[ZMOD_Init] PID mismatch!\r\n"); return ZMOD4410_ERR_CONFIG; }

    /* Step 4 — read sensor CONF (6 bytes) from NVM */
    debug_print("[ZMOD_Init] Step4 CONF\r\n");
    if (zmod4410_read_reg(i2c, REG_CONF, g_config, 6U) == 0U)
    { return ZMOD4410_ERR_NACK; }
    debug_print("[ZMOD_Init] CONF=%X %X %X %X %X %X\r\n",
                g_config[0],g_config[1],g_config[2],g_config[3],g_config[4],g_config[5]);

    /* Step 5 — read production data (7 bytes) from NVM */
    debug_print("[ZMOD_Init] Step5 PROD_DATA\r\n");
    if (zmod4410_read_reg(i2c, REG_PROD_DATA, g_prod_data, PROD_DATA_LEN) == 0U)
    { return ZMOD4410_ERR_NACK; }

    /* Step 6 — compute HSP for init H block */
    debug_print("[ZMOD_Init] Step6 CalcHSP\r\n");
    zmod4410_calc_hsp(INIT_H_DATA, INIT_H_LEN, g_config, hsp);

    /* Step 7 — write init blocks (H, D, M, S) */
    debug_print("[ZMOD_Init] Step7 Write init H/D/M/S\r\n");
    if (zmod4410_write_reg(i2c, INIT_H_ADDR, hsp, INIT_H_LEN) == 0U)
    { return ZMOD4410_ERR_NACK; }
    if (zmod4410_write_reg(i2c, INIT_D_ADDR, INIT_D_DATA, INIT_D_LEN) == 0U)
    { return ZMOD4410_ERR_NACK; }
    if (zmod4410_write_reg(i2c, INIT_M_ADDR, INIT_M_DATA, INIT_M_LEN) == 0U)
    { return ZMOD4410_ERR_NACK; }
    if (zmod4410_write_reg(i2c, INIT_S_ADDR, INIT_S_DATA, INIT_S_LEN) == 0U)
    { return ZMOD4410_ERR_NACK; }

    /* Step 8 — run init measurement, wait for it to complete */
    debug_print("[ZMOD_Init] Step8 Init-run CMD\r\n");
    st = zmod4410_write_cmd(i2c, INIT_CMD_START);
    if (st != ZMOD4410_OK) { return st; }
    zmod4410_delay_ms(50U);
    debug_print("[ZMOD_Init] Step8 Wait idle...\r\n");
    st = zmod4410_wait_idle(i2c, INIT_POLL_TIMEOUT, 50U);
    if (st != ZMOD4410_OK) { debug_print("[ZMOD_Init] Step8 idle timeout\r\n"); return ZMOD4410_ERR_TIMEOUT; }
    debug_print("[ZMOD_Init] Step8 idle OK\r\n");

    /* Read R trim: [0..1] = mox_lr, [2..3] = mox_er */
    if (zmod4410_read_reg(i2c, INIT_R_ADDR, r_trim, INIT_R_LEN) == 0U)
    { return ZMOD4410_ERR_NACK; }
    g_mox_lr = (uint16_t)(((uint16_t)r_trim[0] << 8U) | r_trim[1]);
    g_mox_er = (uint16_t)(((uint16_t)r_trim[2] << 8U) | r_trim[3]);
    debug_print("[ZMOD_Init] mox_lr=%u mox_er=%u\r\n", (unsigned)g_mox_lr, (unsigned)g_mox_er);

    /* Step 9 — compute HSP for measurement H block */
    debug_print("[ZMOD_Init] Step9 CalcHSP meas\r\n");
    zmod4410_calc_hsp(MEAS_H_DATA, MEAS_H_LEN, g_config, hsp);

    /* Step 10 — write measurement configuration blocks */
    debug_print("[ZMOD_Init] Step10 Write meas H/D/M/S\r\n");
    if (zmod4410_write_reg(i2c, MEAS_H_ADDR, hsp, MEAS_H_LEN) == 0U)
    { return ZMOD4410_ERR_NACK; }
    if (zmod4410_write_reg(i2c, MEAS_D_ADDR, MEAS_D_DATA, MEAS_D_LEN) == 0U)
    { return ZMOD4410_ERR_NACK; }
    if (zmod4410_write_reg(i2c, MEAS_M_ADDR, MEAS_M_DATA, MEAS_M_LEN) == 0U)
    { return ZMOD4410_ERR_NACK; }
    if (zmod4410_write_reg(i2c, MEAS_S_ADDR, MEAS_S_DATA, MEAS_S_LEN) == 0U)
    { return ZMOD4410_ERR_NACK; }
    debug_print("[ZMOD_Init] Done\r\n");

    return ZMOD4410_OK;
}

/* -----------------------------------------------------------------------
 * ZMOD4410_Trigger_Measurement
 * ----------------------------------------------------------------------- */
ZMOD4410_Status_t ZMOD4410_Trigger_Measurement(I2C_t i2c)
{
    return zmod4410_write_cmd(i2c, MEAS_CMD_START);
}

/* -----------------------------------------------------------------------
 * ZMOD4410_Measurement_Ready — poll STATUS register.
 * Returns: 1=ready, 0=still running, <0=error
 * ----------------------------------------------------------------------- */
int ZMOD4410_Measurement_Ready(I2C_t i2c)
{
    uint8_t status;

    if (zmod4410_read_reg(i2c, REG_STATUS, &status, 1U) == 0U)
    {
        return -(int)ZMOD4410_ERR_NACK;
    }

    if ((status & STATUS_SEQUENCER_RUNNING) == 0U)
    {
        return 1;   /* done */
    }
    return 0;       /* still running */
}

/* -----------------------------------------------------------------------
 * ZMOD4410_Read — read ADC bytes and compute TVOC.
 * ----------------------------------------------------------------------- */
ZMOD4410_Status_t ZMOD4410_Read(I2C_t i2c, ZMOD4410_Data_t *out)
{
    uint8_t  status;
    uint8_t  adc_buf[MEAS_R_LEN];
    uint32_t warmup;
    uint16_t raw_adc;
    float    rmox;

    if (out == NULL) { return ZMOD4410_ERR_CONFIG; }

    /* Read STATUS for diagnostics only — do NOT gate on BUSY here.
     * The caller (server_comm_wait_zmod_ready) already handled the
     * timeout / ready check. We proceed with the ADC read regardless. */
    if (zmod4410_read_reg(i2c, REG_STATUS, &status, 1U) == 0U)
    { return ZMOD4410_ERR_NACK; }
    /* Store raw status for caller inspection */

    /* Read 18 bytes of ADC result (9 steps × 2 bytes) */
    if (zmod4410_read_reg(i2c, MEAS_R_ADDR, adc_buf, MEAS_R_LEN) == 0U)
    { return ZMOD4410_ERR_NACK; }

    memcpy(out->raw_adc, adc_buf, MEAS_R_LEN);
    out->raw_len = MEAS_R_LEN;

    /* Compute Rmox from first step ADC value */
    raw_adc = (uint16_t)(((uint16_t)adc_buf[0] << 8U) | adc_buf[1]);
    rmox    = zmod4410_rmox_from_adc(raw_adc);

    /* Estimate TVOC using simplified log-linear model */
    float tvoc_ppb = zmod4410_tvoc_from_rmox(rmox);

    /* Populate output fields */
    out->tvoc_ppb   = (uint16_t)(tvoc_ppb + 0.5f);
    out->tvoc_mg_m3 = (uint16_t)((tvoc_ppb * 4.57f) / 1000.0f); /* approx isobutylene */
    out->eco2_ppm   = 0U;   /* requires separate algorithm */
    out->iaq_index  = 0U;   /* requires full Renesas library */

    out->temperature_c = g_temp_c;
    out->humidity_pct  = g_humidity_pct;

    g_measurement_count++;

    /* Determine warmup status */
    switch (g_operation_mode)
    {
        case IAQ_2ND_GEN_ULP:
        case RELATIVE_IAQ_ULP:
            warmup = 10U;
            break;
        case PUBLIC_BUILDING_AIQ:
            warmup = 60U;
            break;
        default:
            warmup = 100U;
            break;
    }

    out->status = (g_measurement_count < warmup)
                  ? ZMOD4410_STATUS_WARM_UP
                  : ZMOD4410_STATUS_VALID;

    return ZMOD4410_OK;
}

/* -----------------------------------------------------------------------
 * ZMOD4410_Set_Ambient_Conditions
 * ----------------------------------------------------------------------- */
ZMOD4410_Status_t ZMOD4410_Set_Ambient_Conditions(I2C_t i2c, float temperature_c, float humidity_pct)
{
    (void)i2c;
    g_temp_c       = temperature_c;
    g_humidity_pct = humidity_pct;
    return ZMOD4410_OK;
}
