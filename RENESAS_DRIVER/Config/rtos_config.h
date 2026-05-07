/**
 * @file    rtos_config.h
 * @brief   Central RTOS Configuration — Renesas RA6M5 (Cortex-M33)
 *
 * All tunable kernel parameters in one place (cf. FreeRTOSConfig.h).
 * Every OS module includes this header for sizing and timing constants.
 */

#ifndef RTOS_CONFIG_H
#define RTOS_CONFIG_H

/* ======================================================================
 * Clock & Tick
 * ====================================================================== */

/** ICLK frequency in Hz.  CLK_Init() selects PLL /1 → 200 MHz. */
#define OS_CPU_CLOCK_HZ             200000000UL

/** Kernel tick rate.  1000 Hz → 1 ms per tick. */
#define OS_TICK_RATE_HZ             1000U

/** SysTick reload value.  (CPU_CLK / TICK_RATE) − 1 = 199 999. */
#define OS_SYSTICK_RELOAD           ((OS_CPU_CLOCK_HZ / OS_TICK_RATE_HZ) - 1UL)

/* ======================================================================
 * Task Limits
 * ====================================================================== */

/** Maximum concurrent tasks (including idle + timer daemon). */
#define OS_MAX_TASKS                32U

/** Number of discrete priority levels.  0 = highest, N−1 = lowest. */
#define OS_MAX_PRIORITIES           32U

/** Default per-task stack in bytes.  4 KB supports FOTA / TinyML. */
#define OS_DEFAULT_STACK_SIZE       4096U

/** Stack size in 32-bit words (derived). */
#define OS_DEFAULT_STACK_WORDS      (OS_DEFAULT_STACK_SIZE / 4U)

/* ======================================================================
 * Semaphore Limits
 * ====================================================================== */

/** Max tasks that can pend on a single semaphore simultaneously. */
#define OS_SEM_MAX_WAITERS          16U

/* ======================================================================
 * Software Timer Configuration
 * ====================================================================== */

/** Maximum number of software timers that can be registered. */
#define OS_MAX_SOFTWARE_TIMERS      16U

/** Timer daemon task priority.  High (1) so callbacks run promptly. */
#define OS_TIMER_TASK_PRIORITY      1U

/** Timer daemon stack size in bytes. */
#define OS_TIMER_TASK_STACK_SIZE    4096U

/* ======================================================================
 * API Return Codes
 * ====================================================================== */

#define OS_OK                       ( 0)    /**< Operation succeeded.      */
#define OS_ERR_TIMEOUT              (-1)    /**< Timed out waiting.        */
#define OS_ERR_PARAM                (-2)    /**< Invalid parameter.        */
#define OS_ERR_FULL                 (-3)    /**< Resource table full.      */
#define OS_ERR_ISR                  (-4)    /**< Illegal call from ISR.    */

/* ======================================================================
 * Timeout Sentinels
 * ====================================================================== */

/** Block indefinitely until resource is available. */
#define OS_WAIT_FOREVER             0xFFFFFFFFU

/** Do not block — return immediately if resource unavailable. */
#define OS_NO_WAIT                  0U

/* ======================================================================
 * Debug Output Configuration
 * ====================================================================== */

/**
 * Master switch for debug_print().
 * Set to 0 to eliminate all debug code from the binary (zero overhead).
 */
#define OS_DEBUG_ENABLE             1

/**
 * Backend selection — set exactly one to 1.
 *
 *   OS_DEBUG_BACKEND_UART     : bare-metal SCI UART (default, no OS required)
 *   OS_DEBUG_BACKEND_SEMIHOST : ARM semihosting via JTAG/SWD debugger
 *                               (requires --specs=rdimon.specs at link time)
 */
#define OS_DEBUG_BACKEND_UART       1
#define OS_DEBUG_BACKEND_SEMIHOST   0

/**
 * SCI channel used for UART output (0–9).
 * UART7 (SCI7): TX=P613, RX=P614, PSEL=0x05 — connect external USB-UART adapter.
 */
#define OS_DEBUG_UART_CHANNEL       7U

/** Baud rate for the debug UART channel. */
#define OS_DEBUG_UART_BAUDRATE      115200UL

/* ======================================================================
 * USB Integration Test Selection
 * ====================================================================== */

/** USB integration tests disabled. */
#define OS_USB_TEST_MODE_NONE             0U

/** USB device-mode CDC logger task enabled. */
#define OS_USB_TEST_MODE_DEVICE_CDC       1U

/** USB host-mode descriptor enumeration task enabled. */
#define OS_USB_TEST_MODE_HOST_DESCRIPTOR  2U

/**
 * Select one USB integration test mode:
 *   NONE             : USB tests disabled
 *   DEVICE_CDC       : RTOS task publishes periodic CDC log frames
 *   HOST_DESCRIPTOR  : One-shot host enumeration test logs SIM descriptors
 */
#ifndef OS_USB_TEST_MODE
#define OS_USB_TEST_MODE             OS_USB_TEST_MODE_DEVICE_CDC
#endif

/* ======================================================================
 * Data Flash Scratch Test
 * ====================================================================== */

/**
 * Enable the one-shot RTOS task that erases, writes, reads, and verifies
 * the final 64-byte Data Flash block reserved as a scratch area.
 */
#ifndef OS_FLASH_NVS_TEST_ENABLE
#define OS_FLASH_NVS_TEST_ENABLE      1U
#endif

#endif /* RTOS_CONFIG_H */
