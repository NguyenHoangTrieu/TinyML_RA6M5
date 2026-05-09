/**
 * @file    main.c
 * @brief   RTOS application test for scheduler and software timer services.
 *
 * Tasks:
 *   1. LED1 blink task            → toggles via OS_Task_Delay()
 *   2. LED2 timer task            → wakes on semaphore posted by software timer
 */

#include "GPIO.h"
#include "debug_print.h"
#include "drv_clk.h"
#include "drv_i2c.h"
#include "drv_usb.h"
#include "kernel.h"
#include "rtos_config.h"
#include "server_comm.h"
#include "semaphore.h"
#include "software_timer.h"
#include "test/app_test_config.h"
#include "test/test_flash_nvs.h"
#include "test/test_rtos.h"
#include "test/test_usb.h"
#include <stdint.h>

#define LED_PORT GPIO_PORT6
#define LED1_PIN 10U
#define LED2_PIN 3U
#define LED3_PIN 9U

/* Set to 1 for a bare-minimum CPU/alive test (infinite LED pattern loop). */
#define MCU_SMOKE_TEST_ONLY 0U

#define TASK_PRIO_LED_TIMER 3U
#define TASK_PRIO_LED_BLINK 4U
#define TASK_PRIO_USB_SVC   2U

#define LED1_ON() GPIO_Write_Pin(LED_PORT, LED1_PIN, GPIO_PIN_SET)
#define LED2_ON() GPIO_Write_Pin(LED_PORT, LED2_PIN, GPIO_PIN_SET)
#define LED3_ON() GPIO_Write_Pin(LED_PORT, LED3_PIN, GPIO_PIN_SET)
#define LED1_OFF() GPIO_Write_Pin(LED_PORT, LED1_PIN, GPIO_PIN_RESET)
#define LED2_OFF() GPIO_Write_Pin(LED_PORT, LED2_PIN, GPIO_PIN_RESET)
#define LED3_OFF() GPIO_Write_Pin(LED_PORT, LED3_PIN, GPIO_PIN_RESET)

static OS_TCB_t g_led_blink_tcb;
static OS_TCB_t g_timer_led_tcb;
#if OS_DEBUG_BACKEND_USB_CDC
static OS_TCB_t g_usb_svc_tcb;
#endif
static Semaphore_t g_led_timer_sem;
static Timer_t g_led_timer;

static void led_init(void) {
  GPIO_Config(LED_PORT, LED1_PIN, GPIO_CNF_OUT_PP, GPIO_MODE_OUTPUT);
  GPIO_Config(LED_PORT, LED2_PIN, GPIO_CNF_OUT_PP, GPIO_MODE_OUTPUT);
  GPIO_Config(LED_PORT, LED3_PIN, GPIO_CNF_OUT_PP, GPIO_MODE_OUTPUT);
  LED1_OFF();
  LED2_OFF();
  LED3_OFF();
}

static void led_toggle(uint8_t pin) {
  uint8_t cur = GPIO_Read_Pin(LED_PORT, pin);
  GPIO_Write_Pin(LED_PORT, pin, (cur != 0U) ? GPIO_PIN_RESET : GPIO_PIN_SET);
}

static void delay_ms_bm(uint32_t ms) {
  volatile uint32_t n = ms * 100000U;
  while (n-- != 0U) {
    __asm volatile("nop");
  }
}

#if MCU_SMOKE_TEST_ONLY
static void mcu_smoke_test_loop(void) {
  /* CK board LEDs */
  GPIO_Config(GPIO_PORT6, 10U, GPIO_CNF_OUT_PP, GPIO_MODE_OUTPUT); /* P610 */
  GPIO_Config(GPIO_PORT6, 9U, GPIO_CNF_OUT_PP, GPIO_MODE_OUTPUT);  /* P609 */
  GPIO_Config(GPIO_PORT6, 3U, GPIO_CNF_OUT_PP, GPIO_MODE_OUTPUT);  /* P603 */
  GPIO_Config(GPIO_PORT6, 1U, GPIO_CNF_OUT_PP, GPIO_MODE_OUTPUT);  /* P601 */

  /* EK board LEDs (in case board mapping still differs) */
  GPIO_Config(GPIO_PORT0, 6U, GPIO_CNF_OUT_PP, GPIO_MODE_OUTPUT);  /* P006 */
  GPIO_Config(GPIO_PORT0, 7U, GPIO_CNF_OUT_PP, GPIO_MODE_OUTPUT);  /* P007 */
  GPIO_Config(GPIO_PORT0, 8U, GPIO_CNF_OUT_PP, GPIO_MODE_OUTPUT);  /* P008 */

  for (;;) {
    GPIO_Write_Pin(GPIO_PORT6, 10U, GPIO_PIN_SET);
    GPIO_Write_Pin(GPIO_PORT6, 9U, GPIO_PIN_SET);
    GPIO_Write_Pin(GPIO_PORT6, 3U, GPIO_PIN_SET);
    GPIO_Write_Pin(GPIO_PORT6, 1U, GPIO_PIN_SET);
    GPIO_Write_Pin(GPIO_PORT0, 6U, GPIO_PIN_SET);
    GPIO_Write_Pin(GPIO_PORT0, 7U, GPIO_PIN_SET);
    GPIO_Write_Pin(GPIO_PORT0, 8U, GPIO_PIN_SET);
    delay_ms_bm(120U);

    GPIO_Write_Pin(GPIO_PORT6, 10U, GPIO_PIN_RESET);
    GPIO_Write_Pin(GPIO_PORT6, 9U, GPIO_PIN_RESET);
    GPIO_Write_Pin(GPIO_PORT6, 3U, GPIO_PIN_RESET);
    GPIO_Write_Pin(GPIO_PORT6, 1U, GPIO_PIN_RESET);
    GPIO_Write_Pin(GPIO_PORT0, 6U, GPIO_PIN_RESET);
    GPIO_Write_Pin(GPIO_PORT0, 7U, GPIO_PIN_RESET);
    GPIO_Write_Pin(GPIO_PORT0, 8U, GPIO_PIN_RESET);
    delay_ms_bm(120U);
  }
}
#endif

static void app_panic(const char *step, int32_t err) {
  debug_print("FATAL: %s failed (%d)\r\n", step, (int)err);

  for (;;) {
    led_toggle(LED3_PIN);
    delay_ms_bm(120U);
  }
}

static void i2c_scan(I2C_t i2c) {
  uint8_t count = 0U;

  debug_print("I2C scan (0x08-0x77):\r\n");

  for (uint8_t addr = 0x08U; addr <= 0x77U; addr++) {
    I2C_Start(i2c);
    uint8_t ack = I2C_Transmit_Address(i2c, addr, I2C_WRITE);
    I2C_Stop(i2c);

    if (ack != 0U) {
      debug_print("  [0x%x] ACK\r\n", (unsigned)addr);
      count++;
    }
  }

  if (count == 0U) {
    debug_print("  No devices found.\r\n");
    debug_print("  Check pull-ups, power, and SCL/SDA routing.\r\n");
  }

  debug_print("\r\n");
}

static void timer_signal_led_task(void *arg) {
  (void)arg;
  (void)OS_SemPost(&g_led_timer_sem);
}

static void task_led_blink(void *arg) {
  (void)arg;

  for (;;) {
    led_toggle(LED1_PIN);
    OS_Task_Delay(250U);
  }
}

static void task_timer_led(void *arg) {
  uint32_t wake_count = 0U;
  (void)arg;

  for (;;) {
    (void)OS_SemPend(&g_led_timer_sem, OS_WAIT_FOREVER);
    led_toggle(LED2_PIN);
    wake_count++;

    if ((wake_count % 10U) == 0U) {
      debug_print("[timer] LED2 toggles=%u\r\n", (unsigned)wake_count);
    }
  }
}

#if OS_DEBUG_BACKEND_USB_CDC
static void task_usb_service(void *arg) {
  (void)arg;

  for (;;) {
    USB_PollEvents();
    OS_Task_Delay(1U);
  }
}
#endif

int main(void) {
  int32_t status;
  uint8_t dbg_link_ok;

  led_init();

#if MCU_SMOKE_TEST_ONLY
  mcu_smoke_test_loop();
#endif

  debug_print_init();
  dbg_link_ok = debug_print_backend_ready();
  if (dbg_link_ok == 0U) {
    /* Continue app execution even when debug link is not ready yet. */
    LED3_OFF();
  }
  
  /* Check if clock fell back to HOCO during startup */
  if (CLK_GetFallbackOccurred() != 0U) {
    debug_print("[WARN] Clock fallback to HOCO triggered - running at 48 MHz instead of 200 MHz\r\n");
  }
  
  debug_print("\r\n=== RA6M5 RTOS Application Test ===\r\n");
#if OS_DEBUG_BACKEND_USB_CDC
  debug_print("DEBUG : USB CDC (Device FS)\r\n");
#else
  debug_print("UART  : SCI3 D0/D1 (P706/P707) @ %u baud\r\n",
              (unsigned)OS_DEBUG_UART_BAUDRATE);
#endif
  debug_print("CLK   : ICLK=%u Hz, SCI_CLK=%u Hz\r\n",
              (unsigned)CLK_GetActualICLK(),
              (unsigned)CLK_GetActualSCIClock());
  debug_print("UARTCFG: BRR_DIV=%u\r\n", (unsigned)OS_DEBUG_UART_BRR_DIVISOR);
  debug_print("I2C   : RIIC1 P512(SCL)/P511(SDA) @ 100 kHz\r\n");
  debug_print("LINK  : %s\r\n", dbg_link_ok != 0U ? "OK" : "FAIL");

  I2C_Init(I2C1, 50U, I2C_SPEED_STANDARD);
  i2c_scan(I2C1);
  delay_ms_bm(120U);

  OS_Init();

  /* Khởi tạo RTOS Preemptive Test Task */
  test_rtos_preemptive_init();

#if OS_DEBUG_BACKEND_USB_CDC
  status = OS_Task_Create(&g_usb_svc_tcb, task_usb_service, (void *)0,
                          TASK_PRIO_USB_SVC, "usb_svc");
  if (status != OS_OK) {
    app_panic("OS_Task_Create usb_svc", status);
  }
#endif

#if (OS_USB_TEST_MODE == OS_USB_TEST_MODE_DEVICE_CDC)
  /* test_usb_cdc_logger: calls USB_Init + USB_PollEvents internally.
   * Drains USB trace ring buffer to UART and sends periodic test
   * messages via USB_Dev_Printf (blocking, waits for BEMP). */
  test_usb_cdc_logger_init();
#endif

  server_comm_init();

#if OS_FLASH_NVS_TEST_ENABLE != 0U
  debug_print("[FLASH NVS TEST] Mode: scratch-block self-test\r\n");
  test_flash_nvs_init();
#else
  debug_print("[FLASH NVS TEST] Mode: disabled\r\n");
#endif

  status = OS_SemCreate(&g_led_timer_sem, 0U, 1U);
  if (status != OS_OK) {
    app_panic("OS_SemCreate", status);
  }

  status = OS_TimerCreate(&g_led_timer, timer_signal_led_task, (void *)0, 500U,
                          OS_TIMER_AUTO_RELOAD);
  if (status != OS_OK) {
    app_panic("OS_TimerCreate", status);
  }

  status = OS_Task_Create(&g_led_blink_tcb, task_led_blink, (void *)0,
                          TASK_PRIO_LED_BLINK, "led_blink");
  if (status != OS_OK) {
    app_panic("OS_Task_Create led_blink", status);
  }

  status = OS_Task_Create(&g_timer_led_tcb, task_timer_led, (void *)0,
                          TASK_PRIO_LED_TIMER, "led_timer");
  if (status != OS_OK) {
    app_panic("OS_Task_Create led_timer", status);
  }

  status = OS_TimerStart(&g_led_timer);
  if (status != OS_OK) {
    app_panic("OS_TimerStart", status);
  }

  debug_print("Tasks ready: LED1 blink, LED2 timer blink\r\n");
  LED3_ON();

  OS_Start();

  for (;;) {
    __asm volatile("nop");
  }
}
