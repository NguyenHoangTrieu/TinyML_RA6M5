#include "server_comm.h"

#include "debug_print.h"
#include "drv_usb.h"
#include "drv_uart.h"
#include "fwupdate_receiver.h"
#include "iaq_predictor.h"
#include "kernel.h"
#include "rtos_config.h"
#include "sensor_simulator.h"
#include "semaphore.h"

#include <stdbool.h>
#include <stdint.h>

#define SERVER_COMM_TASK_PRIO_TX  5U
#define SERVER_COMM_TASK_PRIO_RX  6U
#define SERVER_COMM_TASK_PRIO_IAQ 7U

#define IAQ_STARTUP_DELAY_MS      3000U
#define IAQ_STARTUP_POLL_MS         10U
#define IAQ_CYCLE_DELAY_MS        5000U

#define SERVER_COMM_FRAME_STX     0xA5U
#define SERVER_COMM_FRAME_ETX     0x5AU
#define SERVER_COMM_FRAME_RAW     0x10U
#define SERVER_COMM_FRAME_IAQ     0x11U

typedef struct {
    int32_t tvoc_x10;
    int32_t eco2_x10;
    int32_t iaq_x100;
    uint8_t raw_pending;
    uint8_t iaq_pending;
} server_comm_ctx_t;

static OS_TCB_t s_server_comm_tx_tcb;
static OS_TCB_t s_server_comm_rx_tcb;
static OS_TCB_t s_server_comm_iaq_tcb;
static Semaphore_t s_server_comm_tx_sem;
static server_comm_ctx_t s_server_comm;
static uint8_t s_server_comm_inited = 0U;

static int32_t server_comm_round_to_i32(float val)
{
    if (val >= 0.0f)
    {
        return (int32_t)(val + 0.5f);
    }

    return (int32_t)(val - 0.5f);
}

static uint16_t server_comm_crc16(const uint8_t *data, uint16_t len)
{
    uint16_t crc = 0xFFFFU;
    uint16_t i;

    if (data == (const uint8_t *)0)
    {
        return 0U;
    }

    for (i = 0U; i < len; i++)
    {
        uint8_t bit;
        crc ^= (uint16_t)((uint16_t)data[i] << 8U);

        for (bit = 0U; bit < 8U; bit++)
        {
            if ((crc & 0x8000U) != 0U)
            {
                crc = (uint16_t)((uint16_t)(crc << 1U) ^ 0x1021U);
            }
            else
            {
                crc = (uint16_t)(crc << 1U);
            }
        }
    }

    return crc;
}

static uint16_t server_comm_crc16_update(uint16_t crc, uint8_t byte)
{
    uint8_t bit;

    crc ^= (uint16_t)((uint16_t)byte << 8U);
    for (bit = 0U; bit < 8U; bit++)
    {
        if ((crc & 0x8000U) != 0U)
        {
            crc = (uint16_t)((uint16_t)(crc << 1U) ^ 0x1021U);
        }
        else
        {
            crc = (uint16_t)(crc << 1U);
        }
    }

    return crc;
}

static void server_comm_send_u8(uint8_t b)
{
    UART_SendChar((UART_t)OS_DEBUG_UART_CHANNEL, (char)b);
}

static void server_comm_send_frame(uint8_t type, const uint8_t *payload, uint16_t payload_len)
{
    uint16_t crc = 0xFFFFU;
    uint16_t i;

    crc = server_comm_crc16_update(crc, type);
    crc = server_comm_crc16_update(crc, (uint8_t)((payload_len >> 8U) & 0xFFU));
    crc = server_comm_crc16_update(crc, (uint8_t)(payload_len & 0xFFU));
    for (i = 0U; i < payload_len; i++)
    {
        crc = server_comm_crc16_update(crc, payload[i]);
    }

    server_comm_send_u8(SERVER_COMM_FRAME_STX);
    server_comm_send_u8(type);
    server_comm_send_u8((uint8_t)((payload_len >> 8U) & 0xFFU));
    server_comm_send_u8((uint8_t)(payload_len & 0xFFU));

    for (i = 0U; i < payload_len; i++)
    {
        server_comm_send_u8(payload[i]);
    }

    server_comm_send_u8((uint8_t)((crc >> 8U) & 0xFFU));
    server_comm_send_u8((uint8_t)(crc & 0xFFU));
    server_comm_send_u8(SERVER_COMM_FRAME_ETX);
}

static void server_comm_send_raw(int32_t tvoc_x10, int32_t eco2_x10)
{
    uint8_t payload[8];

    payload[0] = (uint8_t)((tvoc_x10 >> 24U) & 0xFFU);
    payload[1] = (uint8_t)((tvoc_x10 >> 16U) & 0xFFU);
    payload[2] = (uint8_t)((tvoc_x10 >> 8U) & 0xFFU);
    payload[3] = (uint8_t)(tvoc_x10 & 0xFFU);

    payload[4] = (uint8_t)((eco2_x10 >> 24U) & 0xFFU);
    payload[5] = (uint8_t)((eco2_x10 >> 16U) & 0xFFU);
    payload[6] = (uint8_t)((eco2_x10 >> 8U) & 0xFFU);
    payload[7] = (uint8_t)(eco2_x10 & 0xFFU);

    server_comm_send_frame(SERVER_COMM_FRAME_RAW, payload, (uint16_t)sizeof(payload));
}

static void server_comm_send_iaq(int32_t iaq_x100)
{
    uint8_t payload[4];
    uint16_t crc_a;
    uint16_t crc_b;

    payload[0] = (uint8_t)((iaq_x100 >> 24U) & 0xFFU);
    payload[1] = (uint8_t)((iaq_x100 >> 16U) & 0xFFU);
    payload[2] = (uint8_t)((iaq_x100 >> 8U) & 0xFFU);
    payload[3] = (uint8_t)(iaq_x100 & 0xFFU);

    crc_a = server_comm_crc16(payload, (uint16_t)sizeof(payload));
    crc_b = crc16_ccitt(payload, (uint32_t)sizeof(payload));
    if (crc_a != crc_b)
    {
        return;
    }

    server_comm_send_frame(SERVER_COMM_FRAME_IAQ, payload, (uint16_t)sizeof(payload));
}

static void task_server_comm_tx(void *arg)
{
    int32_t tvoc_x10;
    int32_t eco2_x10;
    int32_t iaq_x100;
    uint8_t raw_pending;
    uint8_t iaq_pending;

    (void)arg;

    for (;;)
    {
        (void)OS_SemPend(&s_server_comm_tx_sem, OS_WAIT_FOREVER);

        OS_EnterCritical();
        tvoc_x10 = s_server_comm.tvoc_x10;
        eco2_x10 = s_server_comm.eco2_x10;
        iaq_x100 = s_server_comm.iaq_x100;
        raw_pending = s_server_comm.raw_pending;
        iaq_pending = s_server_comm.iaq_pending;
        s_server_comm.raw_pending = 0U;
        s_server_comm.iaq_pending = 0U;
        OS_ExitCritical();

        if (raw_pending != 0U)
        {
            server_comm_send_raw(tvoc_x10, eco2_x10);
        }

        if (iaq_pending != 0U)
        {
            server_comm_send_iaq(iaq_x100);
        }
    }
}

static void task_server_comm_rx(void *arg)
{
    (void)arg;

    for (;;)
    {
        (void)fwupdate_receiver_run();
        OS_Task_Delay(1U);
    }
}

static void task_server_comm_iaq(void *arg)
{
    uint32_t startup_wait_ms = 0U;
    (void)arg;

#if OS_DEBUG_BACKEND_USB_CDC
    while ((startup_wait_ms < IAQ_STARTUP_DELAY_MS) && (USB_Dev_IsConfigured() == 0U))
    {
        OS_Task_Delay(IAQ_STARTUP_POLL_MS);
        startup_wait_ms += IAQ_STARTUP_POLL_MS;
    }
#endif

    debug_print("\r\n[IAQ Task] Starting TFLite initialization...\r\n");

    if (!IAQ_Init())
    {
        debug_print("FATAL: IAQ_Init() failed. Task halted.\r\n");
        for (;;)
        {
            OS_Task_Delay(5000U);
        }
    }

    debug_print("[IAQ Task] Init OK. Starting 5s forecast loop...\r\n");
    SensorSim_Init();
    debug_print("[SensorSim_Init OK]\r\n");

    for (;;)
    {
        SensorPacket_t pkt = SensorSim_Read();
        float forecast = IAQ_Predict(pkt.tvoc);

        int32_t tvoc_10 = (int32_t)(pkt.tvoc * 10.0f);
        int32_t actual_100 = (int32_t)(pkt.iaq_reference * 100.0f);
        int32_t predict_100 = (int32_t)(forecast * 100.0f);

        int t_int = (int)(tvoc_10 / 10);
        int t_frac = (int)(tvoc_10 % 10);

        int a_int = (int)(actual_100 / 100);
        int a_frac1 = (int)((actual_100 % 100) / 10);
        int a_frac2 = (int)(actual_100 % 10);

        int p_int = (int)(predict_100 / 100);
        int p_frac1 = (int)((predict_100 % 100) / 10);
        int p_frac2 = (int)(predict_100 % 10);

        debug_print("[SensorSim_Read OK]\r\n");
        debug_print("[IAQ_Predict OK]\r\n");

        /* Send all sensor values as plain-text strings over UART so the
         * ESP32-C6 can forward them verbatim to the MQTT broker.
         * Format matches the regex patterns in backend/mqtt_client.py:
         *   "Published: TVOC=<v>ppb | Actual=<a> | Predict=<p>"
         *   "T=<t> C  RH=<h>%"
         * NOTE: binary frame sends (server_comm_publish_raw / _predict)
         * are intentionally removed - they corrupted the text stream. */
        debug_print("Published: TVOC=%d.%dppb | Actual=%d.%d%d | Predict=%d.%d%d\r\n",
                    t_int, t_frac, a_int, a_frac1, a_frac2, p_int, p_frac1, p_frac2);

        OS_Task_Delay(IAQ_CYCLE_DELAY_MS);
    }
}

void server_comm_init(void)
{
    if (s_server_comm_inited != 0U)
    {
        return;
    }

    s_server_comm.tvoc_x10 = 0;
    s_server_comm.eco2_x10 = 0;
    s_server_comm.iaq_x100 = 0;
    s_server_comm.raw_pending = 0U;
    s_server_comm.iaq_pending = 0U;

    fwupdate_receiver_init();

    if (OS_SemCreate(&s_server_comm_tx_sem, 0U, 1U) != OS_OK)
    {
        return;
    }

    if (OS_Task_Create(&s_server_comm_tx_tcb,
                       task_server_comm_tx,
                       (void *)0,
                       SERVER_COMM_TASK_PRIO_TX,
                       "srv_tx") != OS_OK)
    {
        return;
    }

    if (OS_Task_Create(&s_server_comm_rx_tcb,
                       task_server_comm_rx,
                       (void *)0,
                       SERVER_COMM_TASK_PRIO_RX,
                       "srv_rx") != OS_OK)
    {
        return;
    }

    if (OS_Task_Create(&s_server_comm_iaq_tcb,
                       task_server_comm_iaq,
                       (void *)0,
                       SERVER_COMM_TASK_PRIO_IAQ,
                       "srv_iaq") != OS_OK)
    {
        return;
    }

    s_server_comm_inited = 1U;
}

void server_comm_publish_raw(float tvoc_ppb, float eco2_ppm)
{
    if (s_server_comm_inited == 0U)
    {
        return;
    }

    OS_EnterCritical();
    s_server_comm.tvoc_x10 = server_comm_round_to_i32(tvoc_ppb * 10.0f);
    s_server_comm.eco2_x10 = server_comm_round_to_i32(eco2_ppm * 10.0f);
    s_server_comm.raw_pending = 1U;
    OS_ExitCritical();

    (void)OS_SemPost(&s_server_comm_tx_sem);
}

void server_comm_publish_predict(float iaq_predict)
{
    if (s_server_comm_inited == 0U)
    {
        return;
    }

    OS_EnterCritical();
    s_server_comm.iaq_x100 = server_comm_round_to_i32(iaq_predict * 100.0f);
    s_server_comm.iaq_pending = 1U;
    OS_ExitCritical();

    (void)OS_SemPost(&s_server_comm_tx_sem);
}
