#include "test_runner.h"
#include "test_cases.h"
#include "drv_rwp.h"

/*
 * RWP (Register Write Protection) driver tests.
 *
 * These tests verify the PRCR register values written by
 * RWP_Unlock_Clock_MSTP() and RWP_Lock_Clock_MSTP().
 *
 * NOTE: These write to actual hardware registers.
 *       Run only on a powered RA6M5 target.
 */

static void test_rwp_unlock_sets_prcr(void)
{
    RWP_Unlock_Clock_MSTP();

    /* After unlock: lower byte should have bits[1:0] set (0b11 = 0x03)
     * and upper byte = 0xA5 (write-protect key)
     * Expected value: (0xA5 << 8) | 0x03 = 0xA503 */
    uint16_t val = PRCR;
    /* Lower byte bits [1:0] must be set */
    TEST_ASSERT((val & 0x03U) == 0x03U);
}

static void test_rwp_lock_clears_prcr(void)
{
    RWP_Lock_Clock_MSTP();

    /* After lock: lower byte should be 0x00, upper byte = 0xA5 */
    uint16_t val = PRCR;
    /* Lower byte bits [1:0] must be cleared */
    TEST_ASSERT((val & 0x03U) == 0x00U);
}

void test_rwp_register(void)
{
    test_register("rwp_unlock_sets_prcr",   test_rwp_unlock_sets_prcr);
    test_register("rwp_lock_clears_prcr",   test_rwp_lock_clears_prcr);
}
