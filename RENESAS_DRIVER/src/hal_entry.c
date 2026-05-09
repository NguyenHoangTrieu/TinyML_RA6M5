/*
 * hal_entry.c  --  Application entry for TESTING_2 bare-metal RA6M5 project.
 *
 * Board: CK-RA6M5 (R7FA6M5BH3CFC, LQFP176)
 *
 * LED assignments (CK-RA6M5):
 *   LED1 Red   -- P610  (Port 6, Pin 10)
 *   LED2 Green -- P603  (Port 6, Pin 3)
 *   LED3 Green -- P609  (Port 6, Pin 9)
 *
 * LEDs on CK-RA6M5 are active-HIGH (driving pin HIGH = LED ON).
 * UART: SCI3 via Arduino UNO connector D0/D1 (P706 RX / P707 TX) @ 115200 baud
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
#include <sys/types.h>


/* -----------------------------------------------------------------
 * LED pin definitions  (Port 6, Pin 10 = P610 = LED1 Red on CK-RA6M5)
 * ----------------------------------------------------------------- */
#define LED1_PORT   GPIO_PORT6
#define LED1_PIN    10U

/* Ghi đè hàm _sbrk để Linker không đòi symbol 'end' từ file .ld nữa */
#ifdef __cplusplus
extern "C" {
#endif
caddr_t _sbrk(int incr) {
    (void)incr;
    // Trả về -1 để báo hết bộ nhớ, chặn đứng mọi nỗ lực gọi malloc() ngầm
    return (caddr_t)-1; 
}
#ifdef __cplusplus
}
#endif

/* -----------------------------------------------------------------
 * hal_entry  --  called from main()
 * ----------------------------------------------------------------- */
void hal_entry(void)
{
    /* Configure LED1 (P610) as GPIO push-pull output */
    GPIO_Config(LED1_PORT, LED1_PIN, GPIO_CNF_OUT_PP, GPIO_MODE_OUTPUT);

    /* CK LEDs are active-high: LOW = OFF, HIGH = ON */
    GPIO_Write_Pin(LED1_PORT, LED1_PIN, GPIO_PIN_RESET);

    /* Run driver test suite (only IAQ and RTOS tests enabled) */
    test_iaq_register();
    test_rtos_register();

    uint32_t failures = test_run_all();

    /* Indicate result via LED blink rate */
    uint32_t half_period_ms = (failures == 0U) ? 500U : 100U;

    while (1)
    {
        GPIO_Write_Pin(LED1_PORT, LED1_PIN, GPIO_PIN_SET);   /* LED ON  */
        delay_ms(half_period_ms);
        GPIO_Write_Pin(LED1_PORT, LED1_PIN, GPIO_PIN_RESET); /* LED OFF */
        delay_ms(half_period_ms);
    }
}
