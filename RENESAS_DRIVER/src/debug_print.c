/**
 * @file    debug_print.c
 * @brief   UART backend for debug_print() — bare-metal SCI transmit.
 *
 * Only compiled when OS_DEBUG_ENABLE=1 and OS_DEBUG_BACKEND_UART=1
 * (see Config/rtos_config.h).  No heap, no stdio dependency.
 *
 * Supported format specifiers: %s %d %u %x %X %c %%
 * Width/precision/length modifiers are not supported (not needed for
 * embedded diagnostics; keeps code size minimal).
 */

#include "debug_print.h"
#include "rtos_config.h"

#if OS_DEBUG_ENABLE && !OS_DEBUG_BACKEND_SEMIHOST

#include "drv_uart.h"
#include "kernel.h"
#include <stdarg.h>
#include <stdint.h>

/* Channel used for all output (value from rtos_config.h). */
#define DBG_CH  ((UART_t)OS_DEBUG_UART_CHANNEL)

/* -------------------------------------------------------------------------
 * Internal helpers
 * ------------------------------------------------------------------------- */

static void dbg_putc(char c)
{
    UART_SendChar(DBG_CH, c);
}

/** Transmit a null-terminated string. */
static void dbg_puts(const char *s)
{
    if (s == (void *)0) { s = "(null)"; }
    while (*s != '\0') { dbg_putc(*s++); }
}

/**
 * Transmit an unsigned integer in the given base (10 or 16).
 * Upper-case hex when @p upper != 0.
 */
static void dbg_putu(uint32_t val, uint8_t base, uint8_t upper)
{
    /* 10 digits enough for UINT32_MAX in decimal; 8 in hex. */
    char    buf[10];
    uint8_t i = 0U;

    if (val == 0U) { dbg_putc('0'); return; }

    while (val > 0U && i < (uint8_t)sizeof(buf))
    {
        uint8_t digit = (uint8_t)(val % (uint32_t)base);
        if (digit < 10U) {
            buf[i] = (char)('0' + digit);
        } else {
            buf[i] = upper ? (char)('A' + digit - 10U)
                           : (char)('a' + digit - 10U);
        }
        i++;
        val /= (uint32_t)base;
    }

    /* Digits were accumulated least-significant first; reverse-transmit. */
    while (i > 0U) { dbg_putc(buf[--i]); }
}

static void dbg_put_timestamp(void)
{
    dbg_putc('[');
    dbg_putu(OS_GetTick(), 10U, 0U);
    dbg_puts(" ms] ");
}

/* -------------------------------------------------------------------------
 * Public API
 * ------------------------------------------------------------------------- */

/**
 * debug_print_init — initialise the debug UART.
 * Must be called once before any debug_print() call.
 */
void debug_print_init(void)
{
    UART_Init(DBG_CH, OS_DEBUG_UART_BAUDRATE);
}

/**
 * debug_print — printf-like transmit over the debug UART.
 *
 * Supported specifiers:
 *   %s — null-terminated string  (%s of NULL prints "(null)")
 *   %d — signed 32-bit decimal
 *   %u — unsigned 32-bit decimal
 *   %x — unsigned 32-bit hex, lower-case
 *   %X — unsigned 32-bit hex, upper-case
 *   %c — single character
 *   %% — literal '%'
 */
void debug_print(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    dbg_put_timestamp();

    for (const char *p = fmt; *p != '\0'; p++)
    {
        if (*p != '%')
        {
            dbg_putc(*p);
            continue;
        }

        p++;  /* step past '%' */

        switch (*p)
        {
            case 's':
                dbg_puts(va_arg(args, const char *));
                break;

            case 'd': {
                int32_t v = va_arg(args, int32_t);
                if (v < 0) { dbg_putc('-'); dbg_putu((uint32_t)(-v), 10U, 0U); }
                else        {               dbg_putu((uint32_t)  v,  10U, 0U); }
                break;
            }

            case 'u':
                dbg_putu(va_arg(args, uint32_t), 10U, 0U);
                break;

            case 'x':
                dbg_putu(va_arg(args, uint32_t), 16U, 0U);
                break;

            case 'X':
                dbg_putu(va_arg(args, uint32_t), 16U, 1U);
                break;

            case 'c':
                dbg_putc((char)va_arg(args, int));
                break;

            case '%':
                dbg_putc('%');
                break;

            case '\0':
                /* Trailing '%' at end of format string — stop. */
                goto done;

            default:
                /* Unknown specifier: pass through literally. */
                dbg_putc('%');
                dbg_putc(*p);
                break;
        }
    }

done:
    va_end(args);
}

#endif /* OS_DEBUG_ENABLE && !OS_DEBUG_BACKEND_SEMIHOST */
