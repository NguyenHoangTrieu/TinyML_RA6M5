# FW_UART_Driver

Tags: #done #firmware #uart

UART driver for RA6M5 SCI peripheral in asynchronous mode. Files: `Driver/Include/drv_uart.h`, `Driver/Source/drv_uart.c`. Hardware constraints: [[HW_RA6M5_SCI]]. Clock source: [[FW_Clock_Driver]]. Bug fixes applied: [[RCA_UART_BRR_SEMR]], [[RCA_UART_SSR_Manual_Clear]].

---

## API

```c
void UART_Init(UART_t uart, uint32_t baudrate);
void UART_SendChar(UART_t uart, char data);
void UART_SendString(UART_t uart, const char *str);
char UART_ReceiveChar(UART_t uart);
```

All busy-wait loops are protected by `DRV_TIMEOUT_TICKS`. On timeout: `SendChar` drops the byte silently; `ReceiveChar` returns 0.

---

## Initialization Sequence

```c
void UART_Init(UART_t uart, uint32_t baudrate) {
    uart_clock_init(n);        /* CLK_ModuleStart_SCI — releases MSTPCRB bit */
    uart_pin_config(uart);     /* configure TX/RX pins via PFS */
    SCI_SCR(n)  = 0x00;        /* disable TX/RX before config */
    SCI_SMR(n)  = 0x00;        /* async, 8-bit, no parity, 1 stop */
    SCI_SEMR(n) = SEMR_BGDM | SEMR_ABCS;   /* bit6 + bit4 → divisor coefficient 8 */
    SCI_BRR(n)  = SCI_PCLK_HZ / (8 * baudrate) - 1;
    SCI_SCR(n)  = SCR_TE | SCR_RE;
}
```

SEMR bit positions (from [[HW_RA6M5_SCI]] §34 Table 34.12):
- BGDM = bit 6 (was wrongly at bit 5 in original code)
- ABCS = bit 4 (was wrongly at bit 6 in original code)

---

## BRR Calculation

BRR formula derivation from [[HW_RA6M5_SCI]]:
```
SCI clock = PCLKA = 100 000 000  (PLL /2, set by CLK_Init in [[FW_Clock_Driver]])
BRR       = SCI_PCLK_HZ / (8 × baudrate) - 1
```

| Baudrate | BRR | Actual | Error |
|----------|-----|--------|-------|
| 115200 | 107 | 115 741 | 0.47% |
| 9600 | 1301 | not representable with current fixed setup | use a different divisor mode if low baud is needed |

---

## Pin Mapping (EK-RA6M5)

| Channel | TX | RX | PSEL |
|---------|----|----|------|
| UART7 | P613 | P614 | 0x05 |
| UART0 | P111 | P110 | 0x04 |
| UART2 | P302 | P301 | 0x04 |

Full table in `drv_uart.c: uart_pin_config()`.

---

## Test Coverage

`src/test/test_uart.c` (hardware integration, requires powered target):
- `uart7_scr_te_re_set`: SCR has TE+RE after UART_Init
- `uart7_tdre_ready`: TDRE=1 after init
- `uart7_brr_115200`: BRR register = 107
