/*
 * hal_entry.c  --  Application entry for TESTING_2 bare-metal RA6M5 project.
 *
 * Board: EK-RA6M5 (R7FA6M5BH3CFC, LQFP176)
 *
 * LED assignments (ra/board/ra6m5_ek/board_leds.c, confirmed):
 *   LED1 Blue  -- P006  (Port 0, Pin 6)
 *   LED2 Green -- P007  (Port 0, Pin 7)
 *   LED3 Red   -- P008  (Port 0, Pin 8)
 *
 * LEDs on EK-RA6M5 are active-LOW (driving pin LOW = LED ON).
 *
 * This file:
 *   1. Configures LED1 as GPIO push-pull output
 *   2. Runs the bare-metal driver test suite (src/test/)
 *   3. Blinks LED1 to report results:
 *        Slow blink (500 ms period) -- all tests passed
 *        Fast blink (100 ms period) -- one or more tests failed
 */

#include "hal_entry.h"
#include "GPIO.h"
#include "utils.h"
#include "test/test_runner.h"
#include "test/test_cases.h"

/* -----------------------------------------------------------------
 * LED pin definitions  (Port 0, Pin 6 = P006 = LED1 Blue)
 * ----------------------------------------------------------------- */
#define LED1_PORT   GPIO_PORT0
#define LED1_PIN    6U

/* -----------------------------------------------------------------
 * hal_entry  --  called from main()
 * ----------------------------------------------------------------- */
void hal_entry(void)
{
    /* Configure LED1 (P006) as GPIO push-pull output */
    GPIO_Config(LED1_PORT, LED1_PIN, GPIO_CNF_OUT_PP, GPIO_MODE_OUTPUT);

    /* Drive LED OFF initially (active-low: HIGH = OFF) */
    GPIO_Write_Pin(LED1_PORT, LED1_PIN, GPIO_PIN_SET);

    /* Run driver test suite */
    test_gpio_register();
    test_rwp_register();
    test_uart_register();
    test_i2c_register();

    uint32_t failures = test_run_all();

    /* Indicate result via LED blink rate */
    uint32_t half_period_ms = (failures == 0U) ? 500U : 100U;

    while (1)
    {
        GPIO_Write_Pin(LED1_PORT, LED1_PIN, GPIO_PIN_RESET); /* LED ON  */
        delay_ms(half_period_ms);
        GPIO_Write_Pin(LED1_PORT, LED1_PIN, GPIO_PIN_SET);   /* LED OFF */
        delay_ms(half_period_ms);
    }
}
