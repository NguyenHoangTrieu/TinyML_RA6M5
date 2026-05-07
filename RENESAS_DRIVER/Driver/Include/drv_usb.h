#ifndef DRV_USB_H
#define DRV_USB_H

#include <stdint.h>
#include "drv_common.h"

/*
 * drv_usb.h — RA6M5 USBFS bare-metal dual-mode driver.
 *
 * Supported modes:
 *   1) Device CDC (logging over BULK IN)
 *   2) Host CDC-ACM (AT command transport over BULK OUT/BULK IN)
 *
 * This module accesses USBFS registers directly and does not use FSP USB APIs.
 */

typedef enum
{
    USB_MODE_DEVICE_CDC = 0,
    USB_MODE_HOST_CDC_ACM = 1
} usb_mode_t;

typedef struct
{
    uint32_t baudrate;
    uint8_t stop_bits;
    uint8_t parity;
    uint8_t data_bits;
} usb_line_coding_t;

typedef struct
{
    uint8_t address;
    uint8_t config_value;
    uint8_t bulk_in_ep;
    uint8_t bulk_out_ep;
    uint16_t bulk_in_mps;
    uint16_t bulk_out_mps;
    uint8_t configured;
} usb_host_cdc_device_t;

typedef struct
{
    uint8_t device_desc[18];
    uint8_t max_packet_size_ep0;
    uint16_t config_total_length;
    uint8_t num_interfaces;
    uint8_t configuration_value;
} usb_host_descriptor_snapshot_t;

/* -----------------------------------------------------------------------
 * Driver lifecycle
 * ----------------------------------------------------------------------- */
drv_status_t USB_Init(usb_mode_t mode);
void USB_Deinit(void);

/* -----------------------------------------------------------------------
 * Event handling
 * ----------------------------------------------------------------------- */
void USB_PollEvents(void);
void USBI0_IRQHandler(void);
void USBI1_IRQHandler(void);

/* -----------------------------------------------------------------------
 * Device CDC API (logging)
 * ----------------------------------------------------------------------- */
drv_status_t USB_Dev_Write(const uint8_t *data, uint32_t len);
drv_status_t USB_Dev_Printf(const char *fmt, ...);
void USB_Dev_SetConfigured(uint8_t configured);
uint8_t USB_Dev_IsConfigured(void);

/* -----------------------------------------------------------------------
 * Host CDC-ACM API (SIM module)
 * ----------------------------------------------------------------------- */
drv_status_t USB_Host_Enumerate(void);
drv_status_t USB_Host_Send_AT_Command(char *cmd);
int32_t USB_Host_Read_Response(uint8_t *buffer, uint32_t len);
const usb_host_cdc_device_t *USB_Host_GetDeviceInfo(void);
const usb_host_descriptor_snapshot_t *USB_Host_GetDescriptorSnapshot(void);

#endif /* DRV_USB_H */
