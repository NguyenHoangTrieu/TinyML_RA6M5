#include "aqi_inference.h"
#include "aqi_model_data.h"
#include "Scaler_Constants.h"

#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/micro/kernels/micro_ops.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include "tensorflow/lite/micro/micro_log.h"

extern "C" {
    #include "debug_print.h"
}

namespace {
    // Arena size: 16KB is sufficient for 4.4KB model
    constexpr int kTensorArenaSize = 16 * 1024; 
    
    // Memory alignment is critical for TFLite to avoid HardFault
    alignas(16) uint8_t tensor_arena[kTensorArenaSize];

    const tflite::Model* model = nullptr;
    tflite::MicroInterpreter* interpreter = nullptr;
    TfLiteTensor* input = nullptr;
    TfLiteTensor* output = nullptr;
    
    // History buffer: 30 values (6 hours x 5 sensors)
    float sensor_history[SCALER_INPUT_SIZE] = {0.0f};
    
    // MicroMutableOpResolver: register only the ops needed for AQI model
    // (Dense Neural Net: FullyConnected + Relu/Logistic + Reshape)
    // Avoid using AllOpsResolver (deprecated) or full MicroMutableOpResolver
    // as it pulls in signal/irfft.h causing issues with KissFFT.
    static tflite::MicroMutableOpResolver<4> resolver;
}

extern "C" int aqi_ai_init(void) {
    // 0. Register required ops (only once)
    resolver.AddFullyConnected();
    resolver.AddRelu();
    resolver.AddLogistic();
    resolver.AddReshape();

    // 1. Map model from C data array
    model = tflite::GetModel(g_aqi_model_data);
    if (model->version() != TFLITE_SCHEMA_VERSION) {
        MicroPrintf("Model schema version mismatch! Expected %d, got %d", 
                    TFLITE_SCHEMA_VERSION, model->version());
        return -1;
    }

    // 2. Initialize static Interpreter to avoid dynamic allocation (heap)
    static tflite::MicroInterpreter static_interpreter(
        model, resolver, tensor_arena, kTensorArenaSize);
    interpreter = &static_interpreter;

    // 3. Allocate memory for Tensors
    TfLiteStatus allocate_status = interpreter->AllocateTensors();
    if (allocate_status != kTfLiteOk) {
        MicroPrintf("AllocateTensors() failed! Consider increasing kTensorArenaSize.");
        return -1;
    }

    // 4. Get pointers to input and output tensors
    input = interpreter->input(0);
    output = interpreter->output(0);
    
    // Check if data format is FLOAT32
    if (input->type != kTfLiteFloat32 || output->type != kTfLiteFloat32) {
        MicroPrintf("Model input/output type is not FLOAT32! Input: %d, Output: %d", input->type, output->type);
        return -1;
    }

    return 0; // Success
}

extern "C" float aqi_ai_predict(float* new_sensor_data) {
    if (interpreter == nullptr || input == nullptr || output == nullptr) {
        return -1.0f; // System not initialized successfully
    }
    
    // 1. Shift buffer left by 5 positions to remove oldest data
    for (int i = 0; i < SCALER_INPUT_SIZE - 5; i++) {
        sensor_history[i] = sensor_history[i + 5];
    }
    
    // 2. Add 5 newest sensor values to end of buffer
    for (int i = 0; i < 5; i++) {
        sensor_history[SCALER_INPUT_SIZE - 5 + i] = new_sensor_data[i];
    }
    
    // 3. Scale input and pass to model
    for (int i = 0; i < SCALER_INPUT_SIZE; i++) {
        input->data.f[i] = (sensor_history[i] - AQI_MEAN[i]) / AQI_SCALE[i];
    }
    
    // 4. Run inference
    TfLiteStatus invoke_status = interpreter->Invoke();
    if (invoke_status != kTfLiteOk) {
        MicroPrintf("Invoke failed!");
        return -1.0f;
    }
    
    // 5. Get result from output tensor
    float predicted_aqi = output->data.f[0];
    
    // Clip negative values (avoid negative AQI from model error)
    if (predicted_aqi < 0.0f) {
        predicted_aqi = 0.0f;
    }
    
    return predicted_aqi;
}
