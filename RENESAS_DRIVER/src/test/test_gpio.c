#include "test_runner.h"
#include "test_cases.h"
#include "GPIO.h"

/*
 * GPIO driver unit tests (register-level, non-destructive).
 *
 * These tests verify:
 *  - GPIO_PortToIndex returns the correct index for all valid ports
 *  - Edge cases (port '0', port 'B', invalid port)
 */

static void test_port_index_numeric(void)
{
    TEST_ASSERT_EQUAL(0U,  GPIO_PortToIndex(GPIO_PORT0));
    TEST_ASSERT_EQUAL(1U,  GPIO_PortToIndex(GPIO_PORT1));
    TEST_ASSERT_EQUAL(9U,  GPIO_PortToIndex(GPIO_PORT9));
}

static void test_port_index_alpha(void)
{
    TEST_ASSERT_EQUAL(10U, GPIO_PortToIndex(GPIO_PORTA));
    TEST_ASSERT_EQUAL(11U, GPIO_PortToIndex(GPIO_PORTB));
}

static void test_port_index_invalid_returns_sentinel(void)
{
    /* Invalid port must return GPIO_INVALID_PORT (0xFF), NOT 0 (which is PORT0) */
    uint8_t idx = GPIO_PortToIndex((GPIO_PORT_t)'Z');
    TEST_ASSERT_EQUAL(GPIO_INVALID_PORT, idx);
}

void test_gpio_register(void)
{
    test_register("gpio_port_index_numeric",          test_port_index_numeric);
    test_register("gpio_port_index_alpha",            test_port_index_alpha);
    test_register("gpio_port_index_invalid_sentinel", test_port_index_invalid_returns_sentinel);
}
