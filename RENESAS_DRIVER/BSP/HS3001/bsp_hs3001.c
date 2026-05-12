/**
 * @file    bsp_hs3001.c
 * @brief   HS3001 temperature + humidity sensor driver.
 *
 * Data format follows the Renesas HS300x reference implementation:
 *   - humidity[0] bits[7:6] = status, bits[5:0] = humidity[13:8]
 *   - humidity[1]           = humidity[7:0]
 *   - temperature[0]        = temperature[13:6]
 *   - temperature[1] bits[7:2] = temperature[5:0], bits[1:0] reserved
 */

#include "bsp_hs3001.h"

#include "kernel.h"

#include <string.h>

#define HS3001_STATUS_MASK            0xC0U
#define HS3001_HUMIDITY_MASK          0x3FU
#define HS3001_TEMPERATURE_MASK       0xFCU
#define HS3001_MEASUREMENT_DELAY_MS   50U
#define HS3001_MAX_READ_ATTEMPTS      3U
#define HS3001_FULL_SCALE_14BIT       16383.0f

static void hs3001_delay_ms(uint32_t ms)
{
    if ((os_current_task != (OS_TCB_t *)0) && (ms != 0U))
    {
        OS_Task_Delay(ms);
        return;
    }

    volatile uint32_t n = ms * 100000U;
    while (n-- != 0U)
    {
        __asm volatile("nop");
    }
}

static HS3001_Status_t hs3001_start_measurement(I2C_t i2c)
{
    I2C_Start(i2c);
    if (!I2C_Transmit_Address(i2c, HS3001_I2C_ADDR, I2C_WRITE))
    {
        I2C_Stop(i2c);
        return HS3001_ERR_NACK;
    }

    I2C_Stop(i2c);
    return HS3001_OK;
}

static HS3001_Status_t hs3001_read_raw(I2C_t i2c, uint8_t raw_data[4])
{
    I2C_Start(i2c);
    if (!I2C_Transmit_Address(i2c, HS3001_I2C_ADDR, I2C_READ))
    {
        I2C_Stop(i2c);
        return HS3001_ERR_NACK;
    }

    if (!I2C_Master_Receive_Data(i2c, raw_data, 4U))
    {
        return HS3001_ERR_TIMEOUT;
    }

    return HS3001_OK;
}

HS3001_Status_t HS3001_Init(I2C_t i2c)
{
    HS3001_Data_t data;

    return HS3001_Read(i2c, &data);
}

HS3001_Status_t HS3001_Read(I2C_t i2c, HS3001_Data_t *out)
{
    uint8_t attempt;
    uint8_t raw_data[4];
    HS3001_Status_t status;

    if (out == (HS3001_Data_t *)0)
    {
        return HS3001_ERR_INVALID_DATA;
    }

    status = hs3001_start_measurement(i2c);
    if (status != HS3001_OK)
    {
        return status;
    }

    for (attempt = 0U; attempt < HS3001_MAX_READ_ATTEMPTS; attempt++)
    {
        uint16_t raw_humidity;
        uint16_t raw_temperature;
        uint8_t status_bits;

        hs3001_delay_ms(HS3001_MEASUREMENT_DELAY_MS);

        status = hs3001_read_raw(i2c, raw_data);
        if (status != HS3001_OK)
        {
            return status;
        }

        status_bits = raw_data[0] & HS3001_STATUS_MASK;
        if (status_bits != 0U)
        {
            continue;
        }

        raw_humidity = (uint16_t)(((uint16_t)(raw_data[0] & HS3001_HUMIDITY_MASK) << 8U) |
                                  (uint16_t)raw_data[1]);
        raw_temperature = (uint16_t)((((uint16_t)raw_data[2] << 8U) |
                                      (uint16_t)(raw_data[3] & HS3001_TEMPERATURE_MASK)) >> 2U);

        memcpy(out->raw_data, raw_data, sizeof(raw_data));
        out->status_bits = status_bits;
        out->humidity_pct = ((float)raw_humidity * 100.0f) / HS3001_FULL_SCALE_14BIT;
        out->temperature_c = (((float)raw_temperature * 165.0f) / HS3001_FULL_SCALE_14BIT) - 40.0f;
        return HS3001_OK;
    }

    return HS3001_ERR_INVALID_DATA;
}