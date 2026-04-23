#include "test_runner.h"
#include "test_cases.h"
#include "drv_uart.h"

/*
 * UART driver integration tests — run on powered RA6M5 target.
 *
 * These tests verify UART7 (TX=P613, RX=P614) register state after init.
 * No loopback cable required — TX-path state is readable via SCI registers.
 */

static void test_uart7_scr_after_init(void)
{
    uint8_t n = (uint8_t)UART7;
    UART_Init(UART7, 115200U);

    /* SCR must have TE (bit5) and RE (bit4) set after init */
    TEST_ASSERT((SCI_SCR(n) & (uint8_t)(SCR_TE | SCR_RE)) == (uint8_t)(SCR_TE | SCR_RE));
}

static void test_uart7_tdre_ready(void)
{
    /* TX buffer must be empty (TDRE=1) immediately after init */
    uint8_t n = (uint8_t)UART7;
    TEST_ASSERT((SCI_SSR(n) & SSR_TDRE) != 0U);
}

static void test_uart7_brr_value(void)
{
    /* BRR for 115200 baud at 8 MHz MOCO with BGDM+ABCS=1:
     * BRR = 8000000 / (4 * 115200) - 1 = 16 */
    uint8_t n = (uint8_t)UART7;
    TEST_ASSERT_EQUAL(16U, (uint32_t)SCI_BRR(n));
}

void test_uart_register(void)
{
    test_register("uart7_scr_te_re_set",  test_uart7_scr_after_init);
    test_register("uart7_tdre_ready",     test_uart7_tdre_ready);
    test_register("uart7_brr_115200",     test_uart7_brr_value);
}
