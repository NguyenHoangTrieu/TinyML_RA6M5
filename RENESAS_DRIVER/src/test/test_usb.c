#include "test_usb.h"

#include "debug_print.h"
#include "drv_usb.h"
#include "kernel.h"
#include "rtos_config.h"

#include <stdint.h>

#define USB_TEST_TASK_PRIO        6U
#define USB_CDC_POLL_PERIOD_MS   10U
#define USB_CDC_LOG_PERIOD_MS  1000U
#define USB_HOST_BOOT_DELAY_MS  500U


/* ======================================================================
 * USB Test Mode
 * ====================================================================== */

/** USBFS is single-role at runtime, so only one USB test mode can be active. */
#define OS_USB_TEST_MODE_NONE             0U
#define OS_USB_TEST_MODE_DEVICE_CDC       1U
#define OS_USB_TEST_MODE_HOST_DESCRIPTOR  2U

/**
 * Select one USB integration test mode:
 *   NONE             : USB tests disabled
 *   DEVICE_CDC       : RTOS task publishes periodic CDC log frames
 *   HOST_DESCRIPTOR  : One-shot host enumeration test logs SIM descriptors
 */
#define OS_USB_TEST_MODE             OS_USB_TEST_MODE_DEVICE_CDC

static OS_TCB_t s_usb_cdc_tcb;
static OS_TCB_t s_usb_host_tcb;

static void usb_hex_dump(const uint8_t *data, uint32_t len)
{
    for (uint32_t i = 0U; i < len; i++)
    {
        debug_print("[USB HOST TEST] desc[%u] = 0x%x\r\n",
                    (unsigned)i,
                    (unsigned)data[i]);
    }
}

static void task_usb_cdc_logger(void *arg)
{
    uint32_t elapsed_ms = 0U;
    uint32_t seq = 0U;
    uint8_t last_configured = 0U;
    drv_status_t init_st;
    (void)arg;

    init_st = USB_Init(USB_MODE_DEVICE_CDC);
    if (init_st != DRV_OK)
    {
        debug_print("[USB CDC TEST] USB_Init failed (%u)\r\n", (unsigned)init_st);
        for (;;)
        {
            OS_Task_Delay(OS_WAIT_FOREVER);
        }
    }

    debug_print("[USB CDC TEST] Device mode active. Connect USB FS to host.\r\n");

    for (;;)
    {
        uint8_t configured;

        USB_PollEvents();
        configured = USB_Dev_IsConfigured();

        if (configured != last_configured)
        {
            debug_print("[USB CDC TEST] configured=%u\r\n", (unsigned)configured);
            last_configured = configured;
        }

        if ((configured != 0U) && (elapsed_ms >= USB_CDC_LOG_PERIOD_MS))
        {
            drv_status_t write_st = USB_Dev_Printf(
                "[USB CDC TEST] seq=%u tick=%u host-ready\r\n",
                (unsigned)seq,
                (unsigned)OS_GetTick());

            if (write_st == DRV_OK)
            {
                seq++;
            }
            else
            {
                debug_print("[USB CDC TEST] write failed (%u)\r\n", (unsigned)write_st);
            }

            elapsed_ms = 0U;
        }

        OS_Task_Delay(USB_CDC_POLL_PERIOD_MS);
        elapsed_ms += USB_CDC_POLL_PERIOD_MS;
    }
}

static void task_usb_host_descriptor_test(void *arg)
{
    const usb_host_cdc_device_t *dev;
    const usb_host_descriptor_snapshot_t *snap;
    drv_status_t st;
    uint16_t vid;
    uint16_t pid;
    (void)arg;

    OS_Task_Delay(USB_HOST_BOOT_DELAY_MS);

    debug_print("[USB HOST TEST] Host mode init...\r\n");
    st = USB_Init(USB_MODE_HOST_CDC_ACM);
    if (st != DRV_OK)
    {
        debug_print("[USB HOST TEST] USB_Init failed (%u)\r\n", (unsigned)st);
        for (;;)
        {
            OS_Task_Delay(OS_WAIT_FOREVER);
        }
    }

    debug_print("[USB HOST TEST] Waiting SIM module attach and enumeration...\r\n");
    st = USB_Host_Enumerate();
    if (st != DRV_OK)
    {
        debug_print("[USB HOST TEST] Enumeration failed (%u)\r\n", (unsigned)st);
        for (;;)
        {
            OS_Task_Delay(OS_WAIT_FOREVER);
        }
    }

    dev = USB_Host_GetDeviceInfo();
    snap = USB_Host_GetDescriptorSnapshot();

    vid = (uint16_t)snap->device_desc[8] | ((uint16_t)snap->device_desc[9] << 8);
    pid = (uint16_t)snap->device_desc[10] | ((uint16_t)snap->device_desc[11] << 8);

    debug_print("[USB HOST TEST] Enumeration PASS\r\n");
    debug_print("[USB HOST TEST] VID=0x%x PID=0x%x class=0x%x sub=0x%x proto=0x%x\r\n",
                (unsigned)vid,
                (unsigned)pid,
                (unsigned)snap->device_desc[4],
                (unsigned)snap->device_desc[5],
                (unsigned)snap->device_desc[6]);
    debug_print("[USB HOST TEST] EP0_MPS=%u cfg_len=%u ifaces=%u cfg=%u\r\n",
                (unsigned)snap->max_packet_size_ep0,
                (unsigned)snap->config_total_length,
                (unsigned)snap->num_interfaces,
                (unsigned)snap->configuration_value);
    debug_print("[USB HOST TEST] BULK_OUT=EP%u/%u BULK_IN=EP%u/%u addr=%u\r\n",
                (unsigned)dev->bulk_out_ep,
                (unsigned)dev->bulk_out_mps,
                (unsigned)dev->bulk_in_ep,
                (unsigned)dev->bulk_in_mps,
                (unsigned)dev->address);

    usb_hex_dump(snap->device_desc, (uint32_t)sizeof(snap->device_desc));

    for (;;)
    {
        OS_Task_Delay(OS_WAIT_FOREVER);
    }
}

void test_usb_cdc_logger_init(void)
{
    (void)OS_Task_Create(&s_usb_cdc_tcb,
                         task_usb_cdc_logger,
                         (void *)0,
                         USB_TEST_TASK_PRIO,
                         "usb_cdc");
}

void test_usb_host_descriptor_init(void)
{
    (void)OS_Task_Create(&s_usb_host_tcb,
                         task_usb_host_descriptor_test,
                         (void *)0,
                         USB_TEST_TASK_PRIO,
                         "usb_host");
}
