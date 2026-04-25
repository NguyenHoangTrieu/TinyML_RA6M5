/**
 * @file    test_ai.c
 * @brief   Test inference AI model (AQI) with synthetic sensor data.
 *
 * Test scenario:
 *   Load 6 sensor samples into model sequentially to fill 6h sliding window.
 *   Each sample contains 5 channels: [PM2.5, PM10, CO, NO2, O3].
 *   After 6 loads, model has sufficient history to predict AQI.
 *   Print results and pollution level classification to UART.
 *
 * Synthetic data:
 *   - Samples 1-3: Clean air (PM2.5=12, PM10=20, CO=200, NO2=15, O3=40)
 *   - Samples 4-5: Moderate pollution (PM2.5=35, PM10=55, CO=400, NO2=35, O3=70)
 *   - Sample 6:    High pollution (PM2.5=75, PM10=110, CO=700, NO2=60, O3=100)
 *
 * Expected result: AQI gradually increases from ~30 to ~120-150.
 */

#include "test_ai.h"
#include "aqi_inference.h"
#include "debug_print.h"
#include "kernel.h"
#include <stdint.h>

/* ----------------------------------------------------------------
 * Synthetic sensor data - 6 timesteps x 5 channels
 * [PM2.5, PM10, CO, NO2, O3] - units per AQI model standard
 * ---------------------------------------------------------------- */
static const float k_synthetic_data[6][5] = {
    /* t-5h: Clean air */       { 12.0f,  20.0f, 200.0f, 15.0f,  40.0f },
    /* t-4h: Clean air */       { 11.5f,  19.5f, 195.0f, 14.5f,  38.0f },
    /* t-3h: Clean air */       { 13.0f,  22.0f, 210.0f, 16.0f,  42.0f },
    /* t-2h: Moderate pollution */ { 35.0f,  55.0f, 400.0f, 35.0f,  70.0f },
    /* t-1h: Moderate pollution */ { 40.0f,  62.0f, 450.0f, 40.0f,  80.0f },
    /* t-0:  High pollution */   { 75.0f, 110.0f, 700.0f, 60.0f, 100.0f },
};

/* RTOS task state */
static OS_TCB_t g_ai_test_tcb;

/* ----------------------------------------------------------------
 * Function to classify AQI according to US EPA scale
 * ---------------------------------------------------------------- */
static const char* aqi_category(float aqi)
{
    if (aqi < 50.0f)   return "Good";
    if (aqi < 100.0f)  return "Moderate";
    if (aqi < 150.0f)  return "Unhealthy for Sensitive Groups";
    if (aqi < 200.0f)  return "Unhealthy";
    if (aqi < 300.0f)  return "Very Unhealthy";
    return "Hazardous";
}

/* ----------------------------------------------------------------
 * Task function
 * ---------------------------------------------------------------- */
static void task_ai_test(void *arg)
{
    (void)arg;

    /* Wait for system to start up stably */
    OS_Task_Delay(500U);

    debug_print("\r\n=== [AI TEST] Starting Model Inference Test ===\r\n");
    debug_print("[AI TEST] Model: AQI Predictor (5 sensors x 6h window)\r\n");
    debug_print("[AI TEST] Mode:  Synthetic data (no real hardware needed)\r\n\r\n");

    /* Step 1: Initialize TFLite interpreter */
    debug_print("[AI TEST] Initializing TFLite interpreter...\r\n");
    int init_result = aqi_ai_init();
    if (init_result != 0) {
        debug_print("[AI TEST] FAIL: aqi_ai_init() returned %d\r\n", init_result);
        debug_print("[AI TEST] >> Check: kTensorArenaSize, model schema version\r\n");
        for (;;) { OS_Task_Delay(5000U); }
    }
    debug_print("[AI TEST] TFLite init: OK\r\n\r\n");

    /* Step 2: Load 6 timesteps sequentially to fill sliding window */
    debug_print("[AI TEST] Feeding 6 timesteps into model...\r\n");
    debug_print("Step   PM2.5  PM10   CO   NO2    O3     =>  AQI    Category\r\n");
    debug_print("----------------------------------------------------------------------\r\n");

    float aqi = 0.0f;
    for (int step = 0; step < 6; step++) {
        /* Cast away const: aqi_ai_predict accepts float* but does not write */
        float sample[5];
        for (int i = 0; i < 5; i++) {
            sample[i] = k_synthetic_data[step][i];
        }

        aqi = aqi_ai_predict(sample);

        /* Print results at each step */
        int32_t aqi_int = (int32_t)aqi;
        int32_t aqi_frac = (int32_t)((aqi - (float)aqi_int) * 10.0f);
        
        int32_t pm25_int = (int32_t)sample[0];
        int32_t pm25_frac = (int32_t)((sample[0] - (float)pm25_int) * 10.0f);
        
        int32_t pm10_int = (int32_t)sample[1];
        int32_t pm10_frac = (int32_t)((sample[1] - (float)pm10_int) * 10.0f);
        
        int32_t co_int = (int32_t)sample[2];
        
        int32_t no2_int = (int32_t)sample[3];
        int32_t no2_frac = (int32_t)((sample[3] - (float)no2_int) * 10.0f);
        
        int32_t o3_int = (int32_t)sample[4];
        int32_t o3_frac = (int32_t)((sample[4] - (float)o3_int) * 10.0f);

        /* Use %d for int, %s for string, remove width/precision modifiers */
        debug_print("[%d]  %d.%d  %d.%d  %d  %d.%d  %d.%d  =>  %d.%d  %s\r\n",
                    step + 1,
                    (int)pm25_int, (int)pm25_frac,
                    (int)pm10_int, (int)pm10_frac,
                    (int)co_int,
                    (int)no2_int, (int)no2_frac,
                    (int)o3_int, (int)o3_frac,
                    (int)aqi_int, (int)aqi_frac,
                    aqi_category(aqi));
        OS_Task_Delay(200U);
    }

    /* Step 3: Print final result */
    debug_print("----------------------------------------------------------------------\r\n");
    debug_print("[AI TEST] Final AQI prediction: %d (Category: %s)\r\n",
                (int)aqi, aqi_category(aqi));

    if (aqi > 0.0f && aqi < 500.0f) {
        debug_print("[AI TEST] Inference: PASS (valid AQI value)\r\n");
    } else {
        debug_print("[AI TEST] Inference: SUSPICIOUS (AQI=%.1f, check model/scaler)\r\n",
                    (double)aqi);
    }

    debug_print("=== [AI TEST] Done ===\r\n\r\n");

    /* Task suspends indefinitely after completion */
    for (;;) {
        OS_Task_Delay(0xFFFFFFFFU);
    }
}

/* ----------------------------------------------------------------
 * Public API
 * ---------------------------------------------------------------- */
void test_ai_inference_init(void)
{
    (void)OS_Task_Create(&g_ai_test_tcb,
                         task_ai_test,
                         (void *)0,
                         6U,   /* Priority lower than system tasks (2-4) */
                         "ai_test");
}
