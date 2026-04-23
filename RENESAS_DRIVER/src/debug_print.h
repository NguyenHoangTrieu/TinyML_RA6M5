/**
 * @file    debug_print.h
 * @brief   Portable printf-like debug output with selectable backend.
 *
 * Configure in Config/rtos_config.h:
 *
 *   OS_DEBUG_ENABLE           — 1: output active, 0: all calls compile away
 *   OS_DEBUG_BACKEND_UART     — bare-metal SCI UART (default)
 *   OS_DEBUG_BACKEND_SEMIHOST — ARM semihosting over JTAG/SWD
 *   OS_DEBUG_UART_CHANNEL     — SCI channel number (0–9)
 *   OS_DEBUG_UART_BAUDRATE    — baud rate (e.g. 115200)
 *
 * Usage:
 *   debug_print_init();                       // call once before use
 *   debug_print("val=%d, str=%s\r\n", 42, "ok");
 *
 * UART backend prefixes each message with "[<tick> ms] ".
 * Before the kernel starts, the tick value remains 0.
 *
 * Supported format specifiers: %s %d %u %x %X %c %%
 */

#ifndef DEBUG_PRINT_H
#define DEBUG_PRINT_H

#include "rtos_config.h"

#if OS_DEBUG_ENABLE

  #if OS_DEBUG_BACKEND_SEMIHOST
    /* --- Semihosting backend -------------------------------------------- */
    /* Requires: --specs=rdimon.specs in linker flags.                       */
    #include <stdio.h>
    static inline void debug_print_init(void) { /* nothing needed */ }
    #define debug_print(fmt, ...)  printf(fmt, ##__VA_ARGS__)

  #else
    /* --- UART backend (default) ----------------------------------------- */
    /* Bare-metal SCI transmit; no heap, no stdio dependency.                */
    void debug_print_init(void);
    void debug_print(const char *fmt, ...);

  #endif /* OS_DEBUG_BACKEND_SEMIHOST */

#else  /* OS_DEBUG_ENABLE == 0 -------------------------------------------- */
  /* Strip everything: zero code, zero data, no warnings about unused args. */
  static inline void debug_print_init(void) { }
  #define debug_print(fmt, ...)  do { } while (0)

#endif /* OS_DEBUG_ENABLE */

#endif /* DEBUG_PRINT_H */
