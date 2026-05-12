/**
 * @file    bsp_hs3001.h
 * @brief   BSP driver for HS3001 temperature + humidity sensor (I2C).
 *
 * Sensor: Renesas HS3001 / HS300x family
 * Interface: I2C, 7-bit address 0x44
 * Requires: drv_i2c driver (I2C_Init must be called before HS3001_Read)
 */

#ifndef BSP_HS3001_H
#define BSP_HS3001_H

#include "drv_i2c.h"
#include <stdint.h>

#define HS3001_I2C_ADDR 0x44U

typedef enum {
    HS3001_OK               = 0,
    HS3001_ERR_NACK         = 1,
    HS3001_ERR_TIMEOUT      = 2,
    HS3001_ERR_INVALID_DATA = 3
} HS3001_Status_t;

typedef struct {
    float temperature_c;
    float humidity_pct;
    uint8_t status_bits;
    uint8_t raw_data[4];
} HS3001_Data_t;

HS3001_Status_t HS3001_Init(I2C_t i2c);
HS3001_Status_t HS3001_Read(I2C_t i2c, HS3001_Data_t *out);

#endif /* BSP_HS3001_H */