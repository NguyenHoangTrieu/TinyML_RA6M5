#include "aqi_inference.h"
#include "aqi_model_data.h"
#include "Scaler_Constants.h"

#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/all_ops_resolver.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include "tensorflow/lite/micro/micro_log.h"

namespace {
    // Kích thước Arena: 16KB là đủ cho model 4.4KB
    constexpr int kTensorArenaSize = 16 * 1024; 
    
    // Memory Alignment cực kỳ quan trọng đối với TFLite để tránh HardFault
    alignas(16) uint8_t tensor_arena[kTensorArenaSize];

    const tflite::Model* model = nullptr;
    tflite::MicroInterpreter* interpreter = nullptr;
    TfLiteTensor* input = nullptr;
    TfLiteTensor* output = nullptr;
    
    // Bộ đệm lịch sử: 30 giá trị (6 giờ x 5 cảm biến)
    float sensor_history[SCALER_INPUT_SIZE] = {0.0f};
    
    // TFLite Resolver: Đăng ký toàn bộ operations để dễ dàng chạy model
    tflite::AllOpsResolver resolver;
}

extern "C" int aqi_ai_init(void) {
    // 1. Ánh xạ model từ mảng dữ liệu C
    model = tflite::GetModel(g_aqi_model_data);
    if (model->version() != TFLITE_SCHEMA_VERSION) {
        MicroPrintf("Model schema version mismatch! Expected %d, got %d", 
                    TFLITE_SCHEMA_VERSION, model->version());
        return -1;
    }

    // 2. Khởi tạo Interpreter tĩnh để tránh cấp phát động (heap)
    static tflite::MicroInterpreter static_interpreter(
        model, resolver, tensor_arena, kTensorArenaSize);
    interpreter = &static_interpreter;

    // 3. Cấp phát bộ nhớ cho các Tensors
    TfLiteStatus allocate_status = interpreter->AllocateTensors();
    if (allocate_status != kTfLiteOk) {
        MicroPrintf("AllocateTensors() failed! Consider increasing kTensorArenaSize.");
        return -1;
    }

    // 4. Lấy con trỏ đến tensor đầu vào và đầu ra
    input = interpreter->input(0);
    output = interpreter->output(0);
    
    // Kiểm tra định dạng dữ liệu có phải là FLOAT32 không
    if (input->type != kTfLiteFloat32 || output->type != kTfLiteFloat32) {
        MicroPrintf("Model input/output type is not FLOAT32! Input: %d, Output: %d", input->type, output->type);
        return -1;
    }

    return 0; // Success
}

extern "C" float aqi_ai_predict(float* new_sensor_data) {
    if (interpreter == nullptr || input == nullptr || output == nullptr) {
        return -1.0f; // Hệ thống chưa được khởi tạo thành công
    }
    
    // 1. Dịch bộ đệm (Shift buffer) sang trái 5 vị trí để loại bỏ dữ liệu cũ nhất
    for (int i = 0; i < SCALER_INPUT_SIZE - 5; i++) {
        sensor_history[i] = sensor_history[i + 5];
    }
    
    // 2. Thêm 5 giá trị cảm biến mới nhất vào cuối bộ đệm
    for (int i = 0; i < 5; i++) {
        sensor_history[SCALER_INPUT_SIZE - 5 + i] = new_sensor_data[i];
    }
    
    // 3. Scale input và đưa vào mô hình
    for (int i = 0; i < SCALER_INPUT_SIZE; i++) {
        input->data.f[i] = (sensor_history[i] - AQI_MEAN[i]) / AQI_SCALE[i];
    }
    
    // 4. Chạy hàm suy luận (Inference)
    TfLiteStatus invoke_status = interpreter->Invoke();
    if (invoke_status != kTfLiteOk) {
        MicroPrintf("Invoke failed!");
        return -1.0f;
    }
    
    // 5. Lấy kết quả từ Tensor Output
    float predicted_aqi = output->data.f[0];
    
    // Clip giá trị âm (tránh xuất hiện AQI âm do sai số mô hình)
    if (predicted_aqi < 0.0f) {
        predicted_aqi = 0.0f;
    }
    
    return predicted_aqi;
}
