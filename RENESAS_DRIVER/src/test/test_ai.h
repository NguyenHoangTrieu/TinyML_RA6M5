#ifndef TEST_AI_H
#define TEST_AI_H

/**
 * @file    test_ai.h
 * @brief   Test AI inference (aqi_ai_predict) với dữ liệu cảm biến giả.
 *
 * Kịch bản:
 *   - Nạp 6 mẫu dữ liệu synthetic (6 giờ = 30 giá trị) vào model tuần tự.
 *   - In kết quả AQI dự đoán và trạng thái (Good/Moderate/Unhealthy) ra UART.
 *   - Chạy như một RTOS Task độc lập, tự kết thúc sau khi hoàn tất.
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Tạo Task chạy bài test inference AI.
 *        Gọi sau OS_Init(), trước OS_Start().
 */
void test_ai_inference_init(void);

#ifdef __cplusplus
}
#endif

#endif /* TEST_AI_H */
