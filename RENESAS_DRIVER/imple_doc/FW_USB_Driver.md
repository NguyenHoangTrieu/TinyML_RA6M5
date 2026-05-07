# FW_USB_Driver

Tags: #bug-fixed-pending-test #firmware #usb #cdc #host #device

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
- Apply manual USBFS pin mux before enabling the engine:
  - `P407` as `USBFS_VBUS` peripheral input
- Provide CDC descriptors:
  - Device Descriptor
  - Configuration Descriptor with IAD
  - String descriptors (`LANGID`, manufacturer, product, serial)
  - Communication Interface + Data Interface
  - Notification interrupt IN + data bulk OUT/IN endpoints
- EP0 handles:
  - Standard requests: `GET_STATUS`, `GET_DESCRIPTOR`, `SET_ADDRESS`, `GET_CONFIGURATION`, `SET_CONFIGURATION`, `GET_INTERFACE`, `SET_INTERFACE`
  - CDC class requests: `SET_LINE_CODING`, `GET_LINE_CODING`, `SET_CONTROL_LINE_STATE`, `SEND_BREAK`
- Bulk IN log stream using:
  - `USB_Dev_Write(...)`
  - `USB_Dev_Printf(...)`
- Event path supports polling and IRQ hook:
  - `USB_PollEvents()`
  - `USBI0_IRQHandler()`, `USBI1_IRQHandler()`

### Function 2: USB Host CDC-ACM (SIM Module)

- Configure USBFS in Host mode
- Apply manual USBFS pin mux before enabling the engine:
  - `P407` as `USBFS_VBUS` peripheral input
  - `P501` as `USBFS_OVRCURA` peripheral input
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

Control-stage routing in `USB_PollEvents()`:
- `CTRT` is re-sampled by reading `INTSTS0` again before decoding control stage
- Only `CTSQ = RDDS/WRDS/WRND` is treated as a new setup request
- `VALID` is cleared before reading setup regs so stale EP0 state is not re-dispatched
- `CTSQ = RDSS/WRSS` only releases EP0 status stage with `BUF + CCPL`

Dispatch rules:
- `bmRequestType` standard (`0x00/0x80`): standard handler
- `bmRequestType` class (`0x21/0xA1`): CDC class handler
- Unsupported requests: STALL on DCP

Line coding is persisted in `g_line_coding` for debug tracking.

Device-side pipe layout:
- PIPE1 = CDC data BULK IN (EP1 IN)
- PIPE2 = CDC data BULK OUT (EP2 OUT)
- PIPE6 = CDC notification INT IN (EP3 IN)

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
  - Polls USB events every 1 ms to service control/data state changes promptly
  - Sends periodic CDC log frames when the host has configured the device
- `test_usb_host_descriptor_init()`
  - Creates one one-shot enumeration task in host mode
  - Enumerates attached CDC-ACM device / SIM module
  - Logs Device Descriptor summary and parsed BULK endpoints over `debug_print`

Current ownership of the USB test selector:
- `OS_USB_TEST_MODE_*` macros live in `Config/rtos_config.h`
- `src/main.c` selects exactly one USB role-specific test task from those macros

---

## Flow Map (FW Perspective)

`USB_Init(mode)`
→ `manual USBFS pin mux bring-up (P407 common, P501 host overcurrent)`
→ `SYSCFG role + board power path`
→ `EP0 setup engine`
→ `Device: descriptors/control + log bulk IN`
→ `Host: enumerate + parse endpoints + configure pipes`
→ `AT send/read over BULK OUT/IN`
→ `USB_PollEvents or USBI0/USBI1 handlers`

---

## Known Issues / Bug History

### BUG-USB-01: Code 43 — Device Descriptor Request Failed (FIXED, pending re-test)

**Symptom:** Windows reports "Unknown USB Device (Device Descriptor Request Failed)".
Device never logs CTRT/SETUP events. DVST loops between POWR/BUS_RESET indefinitely.

**Root Cause:** `USB_PollEvents()` DVST handler only called `usb_device_bus_reset()` when
`DVSQ == USB_DS_DFLT (0x0010)`. The RA6M5 USBFS hardware first sets `DVSQ = SPD_DFLT (0x0050)`
during bus reset + speed detection. That event was silently ignored, leaving EP0 in stale
state when the host sent `GET_DESCRIPTOR`. See [[RCA_USB_CDC_NoEnumeration]].

**Fix applied in `drv_usb.c`:**
- DVST handler extended to also trigger `usb_device_bus_reset()` on `SPD_DFLT (0x0050)`
  and `SPD_POWR (0x0040)`.
- `BRDYENB[0]` now enabled at device-mode init to make SET_LINE_CODING data stage
  interrupt-driven rather than polled.

**Status:** Code change complete. Hardware re-test required.
