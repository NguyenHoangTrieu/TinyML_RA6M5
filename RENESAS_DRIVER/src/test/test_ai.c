/**
 * @file    test_ai.c
 * @brief   Test inference AI model (AQI) với dữ liệu cảm biến synthetic.
 *
 * Kịch bản test:
 *   Nạp 6 mẫu sensor vào model tuần tự để lấp đầy sliding window 6h.
 *   Mỗi mẫu gồm 5 kênh: [PM2.5, PM10, CO, NO2, O3].
 *   Sau 6 lần nạp, model có đủ lịch sử để dự đoán AQI.
 *   In kết quả và phân loại mức độ ô nhiễm ra UART.
 *
 * Dữ liệu synthetic:
 *   - Mẫu 1-3: Không khí sạch (PM2.5=12, PM10=20, CO=200, NO2=15, O3=40)
 *   - Mẫu 4-5: Ô nhiễm vừa  (PM2.5=35, PM10=55, CO=400, NO2=35, O3=70)
 *   - Mẫu 6:   Ô nhiễm cao   (PM2.5=75, PM10=110, CO=700, NO2=60, O3=100)
 *
 * Kết quả mong đợi: AQI tăng dần từ ~30 lên ~120-150.
 */

#include "test_ai.h"
#include "aqi_inference.h"
#include "debug_print.h"
#include "kernel.h"
#include <stdint.h>

/* ----------------------------------------------------------------
 * Dữ liệu cảm biến giả (synthetic) - 6 timestep x 5 kênh
 * [PM2.5, PM10, CO, NO2, O3] - đơn vị theo chuẩn AQI model
 * ---------------------------------------------------------------- */
static const float k_synthetic_data[6][5] = {
    /* t-5h: Không khí sạch */  { 12.0f,  20.0f, 200.0f, 15.0f,  40.0f },
    /* t-4h: Không khí sạch */  { 11.5f,  19.5f, 195.0f, 14.5f,  38.0f },
    /* t-3h: Không khí sạch */  { 13.0f,  22.0f, 210.0f, 16.0f,  42.0f },
    /* t-2h: Ô nhiễm vừa */     { 35.0f,  55.0f, 400.0f, 35.0f,  70.0f },
    /* t-1h: Ô nhiễm vừa */     { 40.0f,  62.0f, 450.0f, 40.0f,  80.0f },
    /* t-0:  Ô nhiễm cao */     { 75.0f, 110.0f, 700.0f, 60.0f, 100.0f },
};

/* Trạng thái RTOS task */
static OS_TCB_t g_ai_test_tcb;

/* ----------------------------------------------------------------
 * Hàm phân loại AQI theo thang US EPA
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

    /* Đợi hệ thống khởi động ổn định */
    OS_Task_Delay(500U);

    debug_print("\r\n=== [AI TEST] Starting Model Inference Test ===\r\n");
    debug_print("[AI TEST] Model: AQI Predictor (5 sensors x 6h window)\r\n");
    debug_print("[AI TEST] Mode:  Synthetic data (no real hardware needed)\r\n\r\n");

    /* Bước 1: Khởi tạo TFLite interpreter */
    debug_print("[AI TEST] Initializing TFLite interpreter...\r\n");
    int init_result = aqi_ai_init();
    if (init_result != 0) {
        debug_print("[AI TEST] FAIL: aqi_ai_init() returned %d\r\n", init_result);
        debug_print("[AI TEST] >> Check: kTensorArenaSize, model schema version\r\n");
        for (;;) { OS_Task_Delay(5000U); }
    }
    debug_print("[AI TEST] TFLite init: OK\r\n\r\n");

    /* Bước 2: Nạp tuần tự 6 timestep để lấp đầy sliding window */
    debug_print("[AI TEST] Feeding 6 timesteps into model...\r\n");
    debug_print("%-6s %-10s %-10s %-10s %-10s %-10s  =>  %-8s %s\r\n",
                "Step", "PM2.5", "PM10", "CO", "NO2", "O3", "AQI", "Category");
    debug_print("----------------------------------------------------------------------\r\n");

    float aqi = 0.0f;
    for (int step = 0; step < 6; step++) {
        /* Cast away const: aqi_ai_predict nhận float* nhưng không ghi */
        float sample[5];
        for (int i = 0; i < 5; i++) {
            sample[i] = k_synthetic_data[step][i];
        }

        aqi = aqi_ai_predict(sample);

        /* In kết quả từng bước */
        int32_t aqi_int = (int32_t)aqi;
        int32_t aqi_frac = (int32_t)((aqi - (float)aqi_int) * 10.0f);

        debug_print("[%d]    %5.1f      %5.1f    %5.0f     %5.1f     %5.1f    =>  %3d.%01d   %s\r\n",
                    step + 1,
                    (double)sample[0], (double)sample[1],
                    (double)sample[2], (double)sample[3], (double)sample[4],
                    (int)aqi_int, (int)aqi_frac,
                    aqi_category(aqi));

        OS_Task_Delay(200U);
    }

    /* Bước 3: In kết quả cuối cùng */
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

    /* Task tự suspend vô tận sau khi hoàn thành */
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
                         6U,   /* Priority thấp hơn system tasks (2-4) */
                         "ai_test");
}
