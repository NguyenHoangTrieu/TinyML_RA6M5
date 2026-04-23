#include "test_runner.h"
#include "test_cases.h"
#include "drv_i2c.h"

/*
 * I2C driver integration tests — run on powered RA6M5 target.
 *
 * These tests verify RIIC0 (SCL=P400, SDA=P401) register state after init.
 * No slave device required — master register state is readable.
 */

static void test_i2c0_ice_after_init(void)
{
    I2C_Init(I2C0, 8U, I2C_SPEED_STANDARD);

    /* ICE (bit7 of ICCR1) must be set after a successful init */
    TEST_ASSERT((ICCR1(0U) & ICCR1_ICE) != 0U);
}

static void test_i2c0_not_in_reset(void)
{
    /* IICRST must be 0 after init completes (reset released) */
    TEST_ASSERT((ICCR1(0U) & ICCR1_IICRST) == 0U);
}

static void test_i2c0_bus_free_after_init(void)
{
    /* Bus must be free (BBSY=0) after init — no spurious START issued */
    TEST_ASSERT((ICCR2(0U) & ICCR2_BBSY) == 0U);
}

void test_i2c_register(void)
{
    test_register("i2c0_ice_set",          test_i2c0_ice_after_init);
    test_register("i2c0_iicrst_cleared",   test_i2c0_not_in_reset);
    test_register("i2c0_bus_free",         test_i2c0_bus_free_after_init);
}
