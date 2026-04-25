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

    /* Wait for system to stabilize */
    OS_Task_Delay(500U);

    debug_print("\r\n=== [AQI MONITOR] Starting Continuous Inference ===\r\n");

    /* 1. Initialize TFLite Model */
    if (aqi_ai_init() != 0) {
        debug_print("[AQI MONITOR] FAIL: TFLite init\r\n");
        /* Suspend task if hardware/memory initialization fails */
        for (;;) { OS_Task_Delay(5000U); } 
    }

    debug_print("[AQI MONITOR] TFLite init OK. Running infinite loop.\r\n");
    debug_print("PM2.5  PM10   CO   NO2    O3     =>  AQI    Category\r\n");
    debug_print("----------------------------------------------------------------------\r\n");

    /* Main task loop */
    for (;;) {
        /* 2. Collect hardware data (Hardware Acquisition) */
        float sample[5];
        
        /* TODO: Replace this section with actual ADC/I2C/UART sensor API calls
         * Currently using simulated data with auto-increment for AI response testing */
        static float dummy_pm25 = 15.0f;
        sample[0] = dummy_pm25;           /* PM2.5 */
        sample[1] = dummy_pm25 + 10.0f;   /* PM10 */
        sample[2] = 250.0f;               /* CO */
        sample[3] = 20.0f;                /* NO2 */
        sample[4] = 45.0f;                /* O3 */

        /* Reset dummy data when threshold exceeded */
        dummy_pm25 += 8.5f; 
        if (dummy_pm25 > 150.0f) {
            dummy_pm25 = 15.0f;
        }

        /* 3. Feed data to Model (Firmware Inference) */
        float aqi = aqi_ai_predict(sample);

        /* 4. Cast for bare-metal UART output */
        int32_t aqi_int = (int32_t)aqi;
        int32_t aqi_frac = (int32_t)((aqi - (float)aqi_int) * 10.0f);
        
        int32_t pm25_int = (int32_t)sample[0];
        int32_t pm25_frac = (int32_t)((sample[0] - (float)pm25_int) * 10.0f);
        
        int32_t pm10_int = (int32_t)sample[1];
        int32_t pm10_frac = (int32_t)((sample[1] - (float)pm10_int) * 10.0f);

        debug_print("%d.%d  %d.%d  %d  %d  %d  =>  %d.%d  %s\r\n",
                    (int)pm25_int, (int)pm25_frac,
                    (int)pm10_int, (int)pm10_frac,
                    (int)sample[2], (int)sample[3], (int)sample[4],
                    (int)aqi_int, (int)aqi_frac,
                    aqi_category(aqi));

        /* 5. Manage sampling cycle */
        /* Currently set to 5 seconds for easy debugging.
         * IMPORTANT HARDWARE NOTE: For AI to run correct 6h window logic,
         * DO NOT feed data to aqi_ai_predict every 5 seconds.
         * Read sensors every 5 seconds and accumulate average. Only after
         * exactly 1 HOUR (3600000U), pass the averaged array to aqi_ai_predict(). */
        OS_Task_Delay(5000U); 
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
