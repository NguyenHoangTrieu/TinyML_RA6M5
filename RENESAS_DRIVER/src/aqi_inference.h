#ifndef AQI_INFERENCE_H
#define AQI_INFERENCE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Initialize AI system
int aqi_ai_init(void);

// AQI prediction function
// Input: array of 5 latest sensor values [PM2.5, PM10, CO, NO2, O3]
// Output: predicted AQI value
float aqi_ai_predict(float* new_sensor_data);

#ifdef __cplusplus
}
#endif

#endif