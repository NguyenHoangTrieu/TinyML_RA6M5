/**
 * @file  test_iaq.c
 * @brief RTOS task: IAQ future-forecast inference test with synthetic data.
 */

#include "test_iaq.h"
#include "debug_print.h"
#include "iaq_predictor.h"
#include "kernel.h"
#include "sensor_simulator.h"
#include <stdbool.h>
#include <stdint.h>

/* --- Configuration --- */
#define CYCLE_DELAY_MS 5000U /* 5-second task cycle */

static OS_TCB_t s_iaq_test_tcb;

/* =========================================================================
 * Task body — runs continuously every 5 seconds
 * ========================================================================= */
static void task_iaq_test(void *arg) {
  (void)arg;

  /* Wait for system to stabilize before booting AI task */
  OS_Task_Delay(800U);

  debug_print("\r\n[IAQ Task] Starting TFLite initialization...\r\n");

  /* Initialize TFLite interpreter */
  bool init_ok = IAQ_Init();
  if (!init_ok) {
    debug_print("FATAL: IAQ_Init() failed. Task halted.\r\n");
    for (;;) {
      OS_Task_Delay(5000U);
    }
  }

  debug_print("[IAQ Task] Init OK. Starting 5s forecast loop...\r\n");
  SensorSim_Init();
  debug_print("[SensorSim_Init OK]\r\n");
  for (;;) {
    /* Get 1 data sample */
    SensorPacket_t pkt = SensorSim_Read();
    debug_print("[SensorSim_Read OK]\r\n");
    /* Run AI prediction */
    float forecast = IAQ_Predict(pkt.tvoc);
    debug_print("[IAQ_Predict OK]\r\n");
    /*
     * Manually process float using integers to avoid debug_print crash.
     * Separate integer, tenths, and hundredths parts to use pure %d.
     */
    int32_t tvoc_10 = (int32_t)(pkt.tvoc * 10.0f);
    int32_t actual_100 = (int32_t)(pkt.iaq_reference * 100.0f);
    int32_t predict_100 = (int32_t)(forecast * 100.0f);

    int t_int = (int)(tvoc_10 / 10);
    int t_frac = (int)(tvoc_10 % 10);

    int a_int = (int)(actual_100 / 100);
    int a_frac1 = (int)((actual_100 % 100) / 10);
    int a_frac2 = (int)(actual_100 % 10);

    int p_int = (int)(predict_100 / 100);
    int p_frac1 = (int)((predict_100 % 100) / 10);
    int p_frac2 = (int)(predict_100 % 10);

    /* Print in the exact required format, without using any modifiers */
    debug_print(
        "Published: TVOC=%d.%dppb | Actual=%d.%d%d | Predict=%d.%d%d\r\n",
        t_int, t_frac, a_int, a_frac1, a_frac2, p_int, p_frac1, p_frac2);

    /* Block task for exactly 5 seconds */
    OS_Task_Delay(CYCLE_DELAY_MS);
  }
}

/* =========================================================================
 * Public API
 * ========================================================================= */
void test_iaq_inference_init(void) {
  (void)OS_Task_Create(&s_iaq_test_tcb, task_iaq_test, (void *)0, 6U,
                       "iaq_test");
}