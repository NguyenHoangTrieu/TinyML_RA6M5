#ifndef TEST_AI_H
#define TEST_AI_H

/**
 * @file    test_ai.h
 * @brief   Test AI inference (aqi_ai_predict) with synthetic sensor data.
 *
 * Scenario:
 *   - Load 6 synthetic data samples (6 hours = 30 values) into model sequentially.
 *   - Print predicted AQI results and status (Good/Moderate/Unhealthy) to UART.
 *   - Run as an independent RTOS Task, self-terminate after completion.
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Create Task to run AI inference test.
 *        Call after OS_Init(), before OS_Start().
 */
void test_ai_inference_init(void);

#ifdef __cplusplus
}
#endif

#endif /* TEST_AI_H */
