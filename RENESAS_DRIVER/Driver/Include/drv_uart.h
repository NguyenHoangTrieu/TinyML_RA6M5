#ifndef DRV_UART_H
#define DRV_UART_H
#include <stdint.h>

/*
 * drv_uart.h — Serial Communication Interface (UART mode) driver for RA6M5.
 *
 * Replaces: SCI.h
 *
 * Register access uses SCI_REG8(n, offset) where n = channel (0–9).
 * RA6M5 SCI channels are clocked from PCLKA.
 * With the current PLL clock tree, PCLKA = 100 MHz.
 *
 * BRR formula (SEMR: BGDM=1 bit6, ABCS=1 bit4 → divisor coefficient = 8):
 *   BRR = SCI_PCLK_HZ / (8 × baudrate) − 1
 *   Example: 100 MHz / (8 × 115200) − 1 ≈ 107 → close to nominal 115200 baud
 */

/* -----------------------------------------------------------------------
 * SCI peripheral base address and per-channel register accessor
 * ----------------------------------------------------------------------- */
#define SCI_BASE    0x40118000UL
#define SCI_STRIDE  0x100UL

#define SCI_REG8(n, off) \
    (*(volatile uint8_t *)(uintptr_t)(SCI_BASE + SCI_STRIDE * (uint32_t)(n) + (uint32_t)(off)))

/* Named register accessors — use SCI_xxx(channel_number) */
#define SCI_SMR(n)   SCI_REG8(n, 0x00U)  /* Serial Mode Register          */
#define SCI_BRR(n)   SCI_REG8(n, 0x01U)  /* Bit Rate Register             */
#define SCI_SCR(n)   SCI_REG8(n, 0x02U)  /* Serial Control Register       */
#define SCI_TDR(n)   SCI_REG8(n, 0x03U)  /* Transmit Data Register        */
#define SCI_SSR(n)   SCI_REG8(n, 0x04U)  /* Serial Status Register        */
#define SCI_RDR(n)   SCI_REG8(n, 0x05U)  /* Receive Data Register         */
#define SCI_SEMR(n)  SCI_REG8(n, 0x07U)  /* Serial Extended Mode Register */

/* -----------------------------------------------------------------------
 * SCR bits (Serial Control Register)
 * ----------------------------------------------------------------------- */
#define SCR_TIE  (1U << 7)   /* Transmit Interrupt Enable */
#define SCR_RIE  (1U << 6)   /* Receive  Interrupt Enable */
#define SCR_TE   (1U << 5)   /* Transmit Enable           */
#define SCR_RE   (1U << 4)   /* Receive  Enable           */

/* -----------------------------------------------------------------------
 * SSR bits (Serial Status Register)
 * RA6M5 HW manual §34: TDRE=bit7, RDRF=bit6
 * ----------------------------------------------------------------------- */
#define SSR_TDRE (1U << 7)   /* Transmit Data Empty (TX buffer ready) */
#define SSR_RDRF (1U << 6)   /* Receive  Data Full  (RX byte ready)   */
#define SSR_ORER (1U << 5)   /* Overrun Error                         */
#define SSR_FER  (1U << 4)   /* Framing Error                         */
#define SSR_PER  (1U << 3)   /* Parity Error                          */

/* -----------------------------------------------------------------------
 * SEMR bits (Serial Extended Mode Register)
 * RA6M5 HW manual §34 Table 34.12:
 *   Bit 6 = BGDM  (Baud Rate Generator Double-Speed Mode)
 *   Bit 4 = ABCS  (Asynchronous Base Clock Select: 1=8clk/bit)
 * With BGDM=1 and ABCS=1: effective divider = /4
 * ----------------------------------------------------------------------- */
#define SEMR_BGDM (1U << 6)
#define SEMR_ABCS (1U << 4)

/* -----------------------------------------------------------------------
 * SCI peripheral clock — on RA6M5 SCI is sourced from PCLKA.
 * CLK_Init() configures PCLKA = 100 MHz (/2 from 200 MHz PLL).
 * ----------------------------------------------------------------------- */
#define SCI_PCLK_HZ  100000000UL

/* -----------------------------------------------------------------------
 * UART channel enumeration
 * Values map directly to SCI channel numbers (UART0 → SCI ch.0, etc.)
 * ----------------------------------------------------------------------- */
typedef enum {
    UART0 = 0,
    UART1 = 1,
    UART2 = 2,
    UART3 = 3,
    UART4 = 4,
    UART5 = 5,
    UART6 = 6,
    UART7 = 7,
    UART8 = 8,
    UART9 = 9
} UART_t;

/* -----------------------------------------------------------------------
 * UART API
 * ----------------------------------------------------------------------- */
void UART_Init(UART_t uart, uint32_t baudrate);
void UART_SendChar(UART_t uart, char data);
void UART_SendString(UART_t uart, const char *str);
char UART_ReceiveChar(UART_t uart);

#endif /* DRV_UART_H */
