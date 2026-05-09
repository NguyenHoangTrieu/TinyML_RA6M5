/*
 * board_config.h — Unified board configuration for RA6M5 CK and EK boards.
 *
 * Set BOARD_TYPE to select target board:
 *   BOARD_TYPE_CK  = CK-RA6M5 (Cortex-M33 Cloud Kit)
 *   BOARD_TYPE_EK  = EK-RA6M5 (Evaluation Kit)
 *
 * This header ensures all drivers use the same pin mapping and sensor configuration
 * across both boards.
 */

#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

#include <stdint.h>

/* -----------------------------------------------------------------------
 * SELECT TARGET BOARD HERE
 * ----------------------------------------------------------------------- */
#define BOARD_TYPE_CK   1
#define BOARD_TYPE_EK   2

/* Change this to switch board target */
#define BOARD_TYPE      BOARD_TYPE_EK

/* -----------------------------------------------------------------------
 * Board-Specific Pin Definitions (UART, SPI, I2C, etc.)
 * ----------------------------------------------------------------------- */

#if (BOARD_TYPE == BOARD_TYPE_CK)
  /* CK-RA6M5: Cortex-M33 Cloud Kit with Arduino UNO connectors */
  
  /* UART3 debug console: Arduino D0/D1 (P706/P707) */
  #define DEBUG_UART_CHANNEL    3
  #define DEBUG_UART_RX_PORT    7
  #define DEBUG_UART_RX_PIN     6   /* P706 */
  #define DEBUG_UART_TX_PORT    7
  #define DEBUG_UART_TX_PIN     7   /* P707 */
  #define DEBUG_UART_PSEL       0x05U
  
  /* LED assignments (active-HIGH) */
  #define LED1_PORT  6
  #define LED1_PIN   10  /* P610 Red    */
  #define LED2_PORT  6
  #define LED2_PIN   3   /* P603 Green  */
  #define LED3_PORT  6
  #define LED3_PIN   9   /* P609 Green  */
  
  /* Sensor configuration: REAL sensors */
  #define USE_REAL_SENSORS    1
  #define USE_SENSOR_AHT20    1
  #define USE_SENSOR_ZMOD4410 1
  #define USE_SENSOR_SIMULATOR 0

#elif (BOARD_TYPE == BOARD_TYPE_EK)
  /* EK-RA6M5: Evaluation Kit with standard debug header */
  
  /* UART7 debug console: debug header (P614/P613) */
  #define DEBUG_UART_CHANNEL    7
  #define DEBUG_UART_RX_PORT    6
  #define DEBUG_UART_RX_PIN     14  /* P614 */
  #define DEBUG_UART_TX_PORT    6
  #define DEBUG_UART_TX_PIN     13  /* P613 */
  #define DEBUG_UART_PSEL       0x05U
  
  /* LED assignments (active-HIGH) */
  #define LED1_PORT  0
  #define LED1_PIN   6   /* P006 */
  #define LED2_PORT  0
  #define LED2_PIN   7   /* P007 */
  #define LED3_PORT  0
  #define LED3_PIN   8   /* P008 */
  
  /* Sensor configuration: SIMULATOR (no physical sensors on EK) */
  #define USE_REAL_SENSORS    0
  #define USE_SENSOR_AHT20    0
  #define USE_SENSOR_ZMOD4410 0
  #define USE_SENSOR_SIMULATOR 1

#else
  #error "BOARD_TYPE not defined or invalid. Set to BOARD_TYPE_CK or BOARD_TYPE_EK"
#endif

/* -----------------------------------------------------------------------
 * Board-Independent Settings (same for both boards)
 * ----------------------------------------------------------------------- */
#define DEBUG_UART_BAUDRATE   115200U
#define DEBUG_UART_BRR_DIV    4U     /* /4 mode: BGDM=1, ABCS=1 */

/* Helper macros to identify board */
#define IS_BOARD_CK()   (BOARD_TYPE == BOARD_TYPE_CK)
#define IS_BOARD_EK()   (BOARD_TYPE == BOARD_TYPE_EK)

#endif /* BOARD_CONFIG_H */
