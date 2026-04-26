/**
 * @file  test_iaq.c
 * @brief RTOS task: IAQ future-forecast inference test with synthetic data.
 *
 * Runs continuously — each cycle feeds TEST_SAMPLE_COUNT synthetic sensor
 * readings through the IAQ model, prints results, then waits CYCLE_DELAY_MS
 * before starting the next cycle.
 *
 * Output format per row:
 *   [N]  TVOC=XXXX ppb  Fore=X.X  Ref=X.X  Err=X.X  Category
 *
 * PASS condition  : all forecasts in [1.0, 5.0] and no Invoke() errors.
 * WARN condition  : MAE > 0.5 vs UBA reference (informational, not failure —
 *                   some offset is expected for a future-state predictor).
 *
 * Debug note on format strings:
 *   debug_print() uses a minimal printf that does NOT support width/alignment
 *   modifiers (e.g. %-4s, %4d). Use plain %s, %d, %u only.
 */

#include "test_iaq.h"
#include "iaq_predictor.h"
#include "sensor_simulator.h"
#include "debug_print.h"
#include "kernel.h"
#include <stdbool.h>
#include <stdint.h>

/* --- Configuration --- */
#define TEST_SAMPLE_COUNT  20U        /* Inference calls per cycle            */
#define CYCLE_DELAY_MS     3000U      /* Wait between cycles (ms)             */
#define IAQ_WARN_THRESHOLD_X10 5      /* MAE threshold * 10 (= 0.5 IAQ pts)  */

static OS_TCB_t s_iaq_test_tcb;

/* =========================================================================
 * UBA category label (no float printf — compare as integers)
 * ========================================================================= */
static const char* iaq_category_str(int32_t iaq_x10)
{
    if (iaq_x10 < 15)  return "Excellent";
    if (iaq_x10 < 25)  return "Good";
    if (iaq_x10 < 35)  return "Moderate";
    if (iaq_x10 < 45)  return "Poor";
    return "Very Poor";
}

/* =========================================================================
 * run_one_cycle — execute one full test sweep and print results
 * Returns number of out-of-range forecasts (0 = PASS)
 * ========================================================================= */
static uint32_t run_one_cycle(uint32_t cycle_num)
{
    SensorSim_Init();

    debug_print("\r\n--- [IAQ TEST] Cycle %u ---\r\n", (unsigned)cycle_num);
    debug_print("[N]  TVOC(ppb)  Fore  Ref   Err   Category\r\n");
    debug_print("--------------------------------------------------\r\n");

    uint32_t fail_count  = 0U;
    int32_t  total_err_x10 = 0;

    for (uint32_t i = 0U; i < TEST_SAMPLE_COUNT; i++) {

        /* Generate synthetic sensor packet */
        SensorPacket_t pkt = SensorSim_Read();

        /* Run inference */
        float forecast = IAQ_Predict(pkt.tvoc);

        /* Convert to integer tenths to avoid float printf */
        int32_t tvoc_i  = (int32_t)pkt.tvoc;
        int32_t fore_x10 = (int32_t)(forecast * 10.0f);
        int32_t ref_x10  = (int32_t)(pkt.iaq_reference * 10.0f);
        int32_t err_x10  = fore_x10 - ref_x10;
        if (err_x10 < 0) err_x10 = -err_x10;

        /* Range check */
        bool in_range = (forecast >= 1.0f && forecast <= 5.0f);
        if (!in_range) {
            fail_count++;
        }

        total_err_x10 += err_x10;

        /* Print row — NO width modifiers (minimal printf limitation) */
        debug_print("[%u]  %d ppb  %d.%d  %d.%d  %d.%d  %s\r\n",
                    (unsigned)i + 1U,
                    (int)tvoc_i,
                    (int)(fore_x10 / 10), (int)(fore_x10 % 10),
                    (int)(ref_x10  / 10), (int)(ref_x10  % 10),
                    (int)(err_x10  / 10), (int)(err_x10  % 10),
                    iaq_category_str(fore_x10));

        OS_Task_Delay(100U);
    }

    /* Summary */
    int32_t mae_x100 = (int32_t)((total_err_x10 * 10) / (int32_t)TEST_SAMPLE_COUNT);
    int32_t mae_x10  = (int32_t)(total_err_x10 / (int32_t)TEST_SAMPLE_COUNT);

    debug_print("--------------------------------------------------\r\n");
    debug_print("[IAQ TEST] In-range: %u/%u  MAE: %d.%02d IAQ pts\r\n",
                (unsigned)(TEST_SAMPLE_COUNT - fail_count),
                (unsigned)TEST_SAMPLE_COUNT,
                (int)(mae_x100 / 100), (int)(mae_x100 % 100));

    if (fail_count == 0U) {
        debug_print("[IAQ TEST] -> PASS\r\n");
    } else {
        debug_print("[IAQ TEST] -> FAIL (%u out-of-range)\r\n",
                    (unsigned)fail_count);
    }

    if (mae_x10 > IAQ_WARN_THRESHOLD_X10) {
        debug_print("[IAQ TEST] -> WARN: MAE > 0.5 — check model or training labels\r\n");
        debug_print("[IAQ TEST]    NOTE: Future-state models may legitimately offset ref.\r\n");
    }

    return fail_count;
}

/* =========================================================================
 * Task body — runs indefinitely
 * ========================================================================= */
static void task_iaq_test(void *arg)
{
    (void)arg;

    /* Wait for system tasks to settle before first output */
    OS_Task_Delay(800U);

    debug_print("\r\n=== [IAQ TEST] Future-Forecast Inference Test ===\r\n");
    debug_print("[IAQ TEST] Model : MLP 1->32->16->1, input: TVOC(ppb)\r\n");
    debug_print("[IAQ TEST] Output: predicted FUTURE IAQ (UBA 1.0-5.0)\r\n");
    debug_print("[IAQ TEST] Mode  : continuous, %u samples/cycle, %u ms between cycles\r\n",
                (unsigned)TEST_SAMPLE_COUNT, (unsigned)CYCLE_DELAY_MS);

    /* Initialize TFLite interpreter — done once, persistent across cycles */
    debug_print("[IAQ TEST] Initializing TFLite Micro...\r\n");
    bool init_ok = IAQ_Init();
    if (!init_ok) {
        debug_print("[IAQ TEST] FATAL: IAQ_Init() failed — task halted\r\n");
        debug_print("[IAQ TEST] >> Check kTensorArenaSize and model schema version\r\n");
        for (;;) { OS_Task_Delay(5000U); }
    }
    debug_print("[IAQ TEST] TFLite init: OK\r\n");

    /* Continuous inference loop */
    uint32_t cycle = 0U;
    for (;;) {
        cycle++;
        run_one_cycle(cycle);
        OS_Task_Delay(CYCLE_DELAY_MS);
    }
}

/* =========================================================================
 * Public API
 * ========================================================================= */
void test_iaq_inference_init(void)
{
    (void)OS_Task_Create(&s_iaq_test_tcb,
                         task_iaq_test,
                         (void *)0,
                         6U,        /* Priority 6 — lower than all system tasks */
                         "iaq_test");
}
