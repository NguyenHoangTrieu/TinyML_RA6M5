/**
 * @file  sensor_simulator.cpp
 * @brief Synthetic sensor data generator for IAQ forecasting tests.
 *
 * TVOC profile: sine wave oscillating between ~100 and ~5500 ppb.
 *   - One full cycle = 62 steps (approximately 12 hours at 12 min/step).
 *   - Small uniform noise (+/-50 ppb) prevents the model from overfitting
 *     to a perfectly clean sine curve.
 *
 * eCO2 derivation: linear interpolation from TVOC.
 *   eCO2 (ppm) = 400 + TVOC * 0.6  (approximates ZMOD4410 cross-sensitivity)
 *
 * UBA IAQ reference formula (analytical ground truth):
 *   Convert TVOC ppb -> mg/m3 assuming molecular weight ~30 g/mol at STP:
 *     mg_m3 = ppb / 1000 / 0.5
 *   Then map to UBA Class 1-5 using piece-wise linear segments:
 *     [0,    0.3)  -> IAQ 1.0 - 1.9  (Excellent)
 *     [0.3,  1.0)  -> IAQ 2.0 - 2.9  (Good)
 *     [1.0,  3.0)  -> IAQ 3.0 - 3.9  (Moderate)
 *     [3.0, 10.0)  -> IAQ 4.0 - 4.9  (Poor)
 *     [10.0, inf)  -> IAQ 5.0         (Very Poor)
 *
 * The AI model is expected to predict a value CLOSE to iaq_reference but
 * slightly offset in time (it forecasts the future, not the current instant).
 * A well-trained model should lead the reference curve by ~1-2 steps.
 */

#include "sensor_simulator.h"
#include <math.h>
#include <stdlib.h>

/* Internal time counter — increments each call to SensorSim_Read() */
static float s_time = 0.0f;

/* =========================================================================
 * SensorSim_Init
 * ========================================================================= */
void SensorSim_Init(void)
{
    s_time = 0.0f;
}

/* =========================================================================
 * calculate_reference_iaq
 *
 * Piece-wise linear mapping from TVOC (ppb) to UBA IAQ class.
 * This is the analytical "ground truth" used to evaluate model accuracy.
 * ========================================================================= */
static float calculate_reference_iaq(float tvoc_ppb)
{
    /* Convert ppb to mg/m3 (approximate, assumes ~30 g/mol at 25°C, 1 atm) */
    float mg_m3 = (tvoc_ppb / 1000.0f) / 0.5f;

    if      (mg_m3 < 0.3f)  return 1.0f + (mg_m3 / 0.3f) * 0.9f;
    else if (mg_m3 < 1.0f)  return 2.0f + ((mg_m3 - 0.3f) / 0.7f) * 0.9f;
    else if (mg_m3 < 3.0f)  return 3.0f + ((mg_m3 - 1.0f) / 2.0f) * 0.9f;
    else if (mg_m3 < 10.0f) return 4.0f + ((mg_m3 - 3.0f) / 7.0f) * 0.9f;
    else                    return 5.0f;
}

/* =========================================================================
 * SensorSim_Read
 *
 * Returns one synthetic sample and advances the internal time counter.
 *
 * TVOC sweep: 2800 + 2700 * sin(t)  =>  range [100, 5500] ppb
 * Time step:  0.1 radian per call   =>  full sine cycle ~= 62 calls
 * ========================================================================= */
SensorPacket_t SensorSim_Read(void)
{
    SensorPacket_t pkt;

    /* --- TVOC: sine sweep with small noise --- */
    float noise = ((float)rand() / (float)RAND_MAX) * 100.0f - 50.0f;
    pkt.tvoc = 2800.0f + 2700.0f * sinf(s_time) + noise;
    if (pkt.tvoc <   10.0f) pkt.tvoc =   10.0f;
    if (pkt.tvoc > 6000.0f) pkt.tvoc = 6000.0f;

    /* --- eCO2: cross-sensitivity model from TVOC --- */
    float eco2_noise = ((float)rand() / (float)RAND_MAX) * 20.0f - 10.0f;
    pkt.eco2 = 400.0f + (pkt.tvoc * 0.6f) + eco2_noise;
    if (pkt.eco2 <  400.0f) pkt.eco2 =  400.0f;
    if (pkt.eco2 > 5000.0f) pkt.eco2 = 5000.0f;

    /* --- Reference IAQ: analytical UBA formula --- */
    pkt.iaq_reference = calculate_reference_iaq(pkt.tvoc);

    /* Advance time for next sample */
    s_time += 0.1f;

    return pkt;
}
