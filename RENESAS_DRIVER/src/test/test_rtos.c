#include "test_rtos.h"
#include "kernel.h"
#include "debug_print.h"

static OS_TCB_t g_test_low_tcb;
static OS_TCB_t g_test_high_tcb;

static volatile int g_preempt_step = 0;

static void test_high_prio_task(void *arg)
{
    (void)arg;
    
    /* 
     * Nếu preemption chạy đúng, task này phải được thực thi NGAY SAU KHI nó 
     * được tạo ra từ low_prio_task, trước khi low_prio_task chạy lệnh tiếp theo.
     */
    if (g_preempt_step == 1) {
        g_preempt_step = 2;
    }
    
    /* Suspend task vô tận để nhường CPU lại cho các task khác */
    for (;;) {
        OS_Task_Delay(0xFFFFFFFFU); 
    }
}

static void test_low_prio_task(void *arg)
{
    (void)arg;
    int32_t status;
    
    debug_print("\r\n[RTOS TEST] Starting Preemptive Test...\r\n");
    
    g_preempt_step = 1;
    
    /* 
     * Tạo task ưu tiên cao hơn. Ở đây low_prio có ưu tiên 5, ta tạo high_prio ưu tiên 4.
     * (0 là ưu tiên cao nhất, 31 là thấp nhất).
     */
    status = OS_Task_Create(&g_test_high_tcb,
                            test_high_prio_task,
                            (void *)0,
                            4U,
                            "test_high");
                            
    if (status != OS_OK) {
        debug_print("[RTOS TEST] OS_Task_Create failed!\r\n");
        for (;;) { OS_Task_Delay(0xFFFFFFFFU); }
    }
    
    /* 
     * Ngay sau lệnh OS_Task_Create, CPU đáng lý phải bị preempt bởi test_high_prio_task.
     * Do đó khi code chạy đến dòng này, test_high_prio_task ĐÃ chạy xong và set step = 2.
     */
    if (g_preempt_step == 2) {
        debug_print("[RTOS TEST] Preemptive Scheduling: PASS\r\n");
    } else {
        debug_print("[RTOS TEST] Preemptive Scheduling: FAIL (step = %d)\r\n", g_preempt_step);
    }
    
    /* Kết thúc bài test, suspend task này vô tận để hệ thống bình thường tiếp tục chạy */
    for (;;) {
        OS_Task_Delay(0xFFFFFFFFU);
    }
}

void test_rtos_preemptive_init(void)
{
    /* Tạo Low Prio Task. Priority 5 là đủ thấp để không tranh chấp với hệ thống chính. */
    OS_Task_Create(&g_test_low_tcb,
                   test_low_prio_task,
                   (void *)0,
                   5U,
                   "test_low");
}
