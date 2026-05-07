# FW_USB_Driver

Tags: #in-progress #firmware #usb #cdc #host #device

Dual-mode USBFS firmware driver for RA6M5.

Implemented files:
- `Driver/Include/drv_usb.h`
- `Driver/Source/drv_usb.c`
- `src/test/test_usb.h`
- `src/test/test_usb.c`

Hardware dependency: [[HW_RA6M5_USBFS]]

---

## Scope

### Function 1: USB Device CDC (Logging)

- Configure USBFS in Device mode
- Provide CDC descriptors:
  - Device Descriptor
  - Configuration Descriptor
  - Communication Interface + Data Interface
  - Endpoint descriptors
- EP0 handles:
  - Standard requests: `SET_ADDRESS`, `SET_CONFIGURATION`, `GET_DESCRIPTOR`
  - CDC class requests: `SET_LINE_CODING`, `GET_LINE_CODING`
- Bulk IN log stream using:
  - `USB_Dev_Write(...)`
  - `USB_Dev_Printf(...)`
- Event path supports polling and IRQ hook:
  - `USB_PollEvents()`
  - `USBI0_IRQHandler()`, `USBI1_IRQHandler()`

### Function 2: USB Host CDC-ACM (SIM Module)

- Configure USBFS in Host mode
- Enable VBUS switch (`P500`, symbolic `USBFS_VBUS_EN`)
- Bare-metal enumeration flow:
  1. Attach detect by line state (`SYSSTS0.LNST`)
  2. Bus reset (`DVSTCTR0.USBRST`)
  3. Read Device Descriptor (EP0 control transfer)
  4. `SET_ADDRESS`
  5. Read full Configuration Descriptor and parse endpoints
  6. `SET_CONFIGURATION`
- Data API:
  - `USB_Host_Send_AT_Command(char *cmd)` via Bulk OUT
  - `USB_Host_Read_Response(uint8_t *buffer, uint32_t len)` via Bulk IN

---

## API

```c
drv_status_t USB_Init(usb_mode_t mode);
void USB_Deinit(void);

void USB_PollEvents(void);
void USBI0_IRQHandler(void);
void USBI1_IRQHandler(void);

drv_status_t USB_Dev_Write(const uint8_t *data, uint32_t len);
drv_status_t USB_Dev_Printf(const char *fmt, ...);

drv_status_t USB_Host_Enumerate(void);
drv_status_t USB_Host_Send_AT_Command(char *cmd);
int32_t USB_Host_Read_Response(uint8_t *buffer, uint32_t len);
const usb_host_cdc_device_t *USB_Host_GetDeviceInfo(void);
```

Return conventions:
- `DRV_OK`: transaction completed
- `DRV_TIMEOUT`: timeout on attach, setup, FIFO ready, BRDY/BEMP wait
- `DRV_ERR`: invalid mode/state, descriptor parse failure, malformed transfer

---

## Driver Internals

### EP0 Control Handling (Device)

Setup packet is read from:
- `USBREQ`, `USBVAL`, `USBINDX`, `USBLENG`

Dispatch rules:
- `bmRequestType` standard (`0x00/0x80`): standard handler
- `bmRequestType` class (`0x21/0xA1`): CDC class handler
- Unsupported requests: STALL on DCP

Line coding is persisted in `g_line_coding` for debug tracking.

### Host Enumeration Implementation

Control transfer helper stages:
1. Program setup packet regs
2. Trigger setup (`DCPCTR.SUREQ`)
3. Wait setup completion (timeout-protected)
4. For IN data stage: poll `BRDYSTS`, read `CFIFO`

Endpoint discovery:
- Parse configuration descriptor blocks
- Filter endpoint descriptors with `bmAttributes == BULK`
- Record first BULK IN and BULK OUT endpoints + max packet sizes

Pipe setup:
- PIPE1 = host BULK OUT
- PIPE2 = host BULK IN

---

## Error Handling and Debug Tracking

Implemented protections:
- Global timeout loops using `DRV_TIMEOUT_TICKS`
- FIFO ready timeout (`CFIFOCTR.FRDY`)
- BRDY/BEMP wait timeout during TX/RX
- Enumeration sanity checks (descriptor length and endpoint presence)

Recommended debug probes:
- `INTSTS0`, `BRDYSTS`, `BEMPSTS`, `NRDYSTS`
- `SYSSTS0.LNST` in attach/detach scenarios
- `DCPCTR` when setup stage stalls

---

## RTOS Test Integration

Integration point: `src/main.c`

Build-time selector in `Config/rtos_config.h`:
- `OS_USB_TEST_MODE_NONE`
- `OS_USB_TEST_MODE_DEVICE_CDC`
- `OS_USB_TEST_MODE_HOST_DESCRIPTOR`

Important constraint:
- USBFS is single-role at runtime, so Device CDC test and Host descriptor test are alternate modes, not simultaneous modes.

Provided test hooks:
- `test_usb_cdc_logger_init()`
  - Creates one RTOS task
  - Polls USB events periodically
  - Sends periodic CDC log frames when the host has configured the device
- `test_usb_host_descriptor_init()`
  - Creates one one-shot enumeration task in host mode
  - Enumerates attached CDC-ACM device / SIM module
  - Logs Device Descriptor summary and parsed BULK endpoints over `debug_print`

---

## Flow Map (FW Perspective)

`USB_Init(mode)`
→ `SYSCFG role + board power path`
→ `EP0 setup engine`
→ `Device: descriptors/control + log bulk IN`
→ `Host: enumerate + parse endpoints + configure pipes`
→ `AT send/read over BULK OUT/IN`
→ `USB_PollEvents or USBI0/USBI1 handlers`
