# RCA_USB_CDC_NoEnumeration

Tags: #in-progress #firmware #usb #cdc #rca

Root cause analysis for USB CDC Device mode: device never completes USB enumeration.
Host (Windows) reports **Code 43 — "Device Descriptor Request Failed"**.

Related docs: [[FW_USB_Driver]], [[HW_RA6M5_USBFS]]

---

## Symptom

```
[USB CDC TRACE] state=PULLUP_ENABLED v0=0x411 v1=0xa v2=0x80
[USB CDC TRACE] DVST is0=0x10c0 SYSSTS0=0x1 DVSTCTR0=0x0 addr=0 cfg=0
[USB CDC TRACE] DVST is0=0x5090 SYSSTS0=0x0 DVSTCTR0=0x2 addr=0 cfg=0
[USB CDC TRACE] state=BUS_RESET_REARM v0=0x40 v1=0x0 v2=0x40
[USB CDC TRACE] DVST is0=0x1090 ...  state=BUS_RESET_REARM  (loops repeatedly)
[USB CDC TRACE] DVST is0=0x10d0 SYSSTS0=0x1 DVSTCTR0=0x2 addr=0 cfg=0  (rare)
```

The device never logs any `CTRT`, `SETUP`, `GET_DESCRIPTOR`, `SET_ADDRESS`, or `SET_CONFIGURATION` trace.
Windows Device Manager: **"Unknown USB Device (Device Descriptor Request Failed)" — Code 43**.

---

## Register Trace Decode

| Timestamp | is0 (INTSTS0) | SYSSTS0 | DVSTCTR0 | DVSQ decoded | Meaning |
|-----------|--------------|---------|----------|--------------|---------|
| 42 ms | `0x10c0` | `0x1` (J-state) | `0x0` | `POWR (0x0)` | VBUS detected, J-state (FS device on bus) |
| 146 ms | `0x5090` | `0x0` (SE0) | `0x2` (UACT) | `SPD_DFLT (0x5)` | Bus reset + speed detect, SE0 on lines |
| 152 ms | BUS_RESET_REARM | | | | Software re-arms EP0 after bus reset |
| 691 ms | `0x1090` | `0x0` | `0x2` | `POWR (0x0)` | Falls back to Powered, no host setup |
| 2056 ms | `0x10d0` | `0x1` | `0x2` | `DFLT (0x1)` | Another bus reset, host gave up and retried |

**Key observation:** `DVSQ` never reaches `ADDS (0x2)` (addressed) or `CNFG (0x3)` (configured).
No `CTRT` event fires even when `DVSQ = DFLT` — meaning the host sent a bus reset but
EP0 control transfers were never captured by the driver.

---

## Root Cause Analysis

### RC-1 (Primary): Missing `CTRT` interrupt after `DS_DFLT`

**Mechanism:**
The host issues a bus reset and expects to send a `GET_DESCRIPTOR(Device)` request within
~100 ms. The USBFS hardware fires `DVST` (Device State Change) with `DVSQ = SPD_DFLT (0x5)`,
then `DVST` with `DVSQ = DFLT (0x1)` after reset completes.

The firmware correctly handles `DVST` → `DS_DFLT` by calling `usb_device_bus_reset()` which
re-arms `DCPCTR = SQSET`, clears FIFOs and status registers.

However **no `CTRT` event is ever logged**, meaning either:
1. The CTRT interrupt fired but `VALID` bit was not set (no pending setup packet), OR
2. The interrupt was never raised because `BRDYENB` for PIPE0 was 0 at the time the
   host started the `GET_DESCRIPTOR` control transfer.

**Evidence:** `INTENB0 = 0x9f00` at init — this correctly includes `CTRT (0x0800)`, `DVST (0x1000)`.
But `BRDYENB = 0x0` at init. The SET_LINE_CODING completion path uses `BRDYSTS[0]` directly
without an interrupt. This is safe for that path, but the deeper issue is timing.

### RC-2 (Contributing): `PULLUP_ENABLED` debounce — pullup applied too late

**Mechanism:**
`usb_device_update_pullup()` waits for `g_usb_device_attach_debounce >= 10` before setting
`SYSCFG.DPRPU`. The debounce counter increments once per `USB_PollEvents()` call, and the task
polls every 1 ms. This means **the D+ pullup is asserted ~10 ms after VBUS is first stable**.

From the log: VBUS detected at t=42 ms (`is0=0x10c0`), PULLUP_ENABLED at t=42 ms trace
(same poll cycle) suggests debounce happened to complete. **BUT**: the SYSCFG value when
PULLUP is enabled shows `v0=0x411`:
- `0x411 = SCKE(10) | DPRPU(4) | USBE(0)` → bits: `0b10000010001` → `SCKE=1, DPRPU=1, USBE=1`

This looks correct. However `v1=0xa` (debounce count) means the pullup was asserted at
debounce count=10, which is borderline.

### RC-3 (Primary Bug): `DS_SPD_DFLT` mapped to `DVST` bus reset trigger but `DS_DFLT (0x10)` does not match

**Mechanism — CRITICAL:**

```c
// In USB_PollEvents():
if ((g_mode == USB_MODE_DEVICE_CDC) && (dvsq == USB_DS_DFLT))
{
    usb_device_bus_reset();
}
```

```c
#define USB_DS_DFLT   0x0010U
#define USB_INT_DVSQ_MASK  0x0070U
```

The code extracts `dvsq = is0 & 0x0070`:
- `is0=0x10c0` → `dvsq = 0x0040` → `USB_DS_SPD_POWR` (not handled)
- `is0=0x5090` → `dvsq = 0x0050` → `USB_DS_SPD_DFLT`  ← **this is the actual bus reset event**
- `is0=0x1090` → `dvsq = 0x0010` → `USB_DS_DFLT` ← triggers bus reset again

**The problem:** When host resets the bus and speed is detected, USBFS sets
`DVSQ = SPD_DFLT (0x0050)`. The firmware only handles `dvsq == USB_DS_DFLT (0x0010)`.

**The first bus reset (`is0=0x5090`, `dvsq=0x0050=SPD_DFLT`) is completely IGNORED.**
The host expects the device to be ready for EP0 traffic immediately after bus reset + speed detect.
Because the firmware doesn't call `usb_device_bus_reset()` on `SPD_DFLT`, EP0 is left in a
stale state. The host sends `GET_DESCRIPTOR` but the device either NACKs or ignores it.

Eventually the USBFS hardware auto-transitions to `DFLT (0x0010)` (after the host times out and
retries). The firmware then calls `usb_device_bus_reset()` but by this time the host has already
given up and issued another bus reset — hence the infinite loop.

### RC-4 (Secondary): `BRDYENB[0]` not set — blocks SET_LINE_CODING data stage detection

`BRDYENB = 0x0` at init means no BRDY interrupt for DCP (pipe 0). The `usb_device_complete_set_line_coding()`
function is called only when `USB_BRDYSTS & 0x0001` is already set (polled in `USB_PollEvents`).
This is fragile — if the poll cycle misses the BRDY window, the SET_LINE_CODING data is lost
and enumeration stalls. Enabling `BRDYENB[0]` would make this robust.

---

## Fix Plan

### Fix 1 (Critical): Handle `USB_DS_SPD_DFLT` in DVST handler

**File:** `Driver/Source/drv_usb.c` — `USB_PollEvents()` around line 1708

```c
// BEFORE (BROKEN):
if ((g_mode == USB_MODE_DEVICE_CDC) && (dvsq == USB_DS_DFLT))
{
    usb_device_bus_reset();
}

// AFTER (FIXED):
if (g_mode == USB_MODE_DEVICE_CDC)
{
    if ((dvsq == USB_DS_DFLT) || (dvsq == USB_DS_SPD_DFLT))
    {
        usb_device_bus_reset();
    }
}
```

**Rationale:** RA6M5 USBFS sets `DVSQ = SPD_DFLT (0x0050)` first during bus reset + speed detection.
The device must be re-armed at this point so EP0 is ready when the host sends `GET_DESCRIPTOR`.
If we only react to `DS_DFLT (0x0010)`, we miss the primary event window.

### Fix 2 (Robustness): Enable BRDY for DCP (pipe 0) in device mode

**File:** `Driver/Source/drv_usb.c` — `USB_Init()` around line 1313

```c
// BEFORE:
USB_INTENB0 = (uint16_t)(USB_INT_VBINT | USB_INT_DVST | USB_INT_CTRT | USB_INT_BRDY | USB_INT_NRDY | USB_INT_BEMP);

// AFTER (also set BRDYENB[0]):
USB_BRDYENB = USB_PIPE_MASK(0U);  /* Enable BRDY for DCP to catch SET_LINE_CODING data stage */
USB_INTENB0 = (uint16_t)(USB_INT_VBINT | USB_INT_DVST | USB_INT_CTRT | USB_INT_BRDY | USB_INT_NRDY | USB_INT_BEMP);
```

### Fix 3 (Correctness): Also handle `USB_DS_SPD_POWR` and `USB_DS_SPD_CNFG`

Add handling for all speed-detect variants in the DVST handler so VBUS/detach scenarios
are robust:

```c
if (g_mode == USB_MODE_DEVICE_CDC)
{
    /* React to any bus reset event, including FS speed-detect variants */
    if ((dvsq == USB_DS_DFLT)
        || (dvsq == USB_DS_SPD_DFLT)
        || (dvsq == USB_DS_SPD_POWR))
    {
        usb_device_bus_reset();
    }
}
```

---

## Status

| Item | Status |
|------|--------|
| Root cause identified | ✅ |
| Fix 1 (SPD_DFLT handling) | 🔲 Pending implementation |
| Fix 2 (BRDYENB[0]) | 🔲 Pending implementation |
| Fix 3 (SPD_POWR handling) | 🔲 Pending implementation |
| Hardware re-test | 🔲 Pending |

---

## Expected Behavior After Fix

```
[USB CDC TRACE] state=PULLUP_ENABLED ...
[USB CDC TRACE] DVST is0=0x10c0 ...              ← VBUS detected, POWR
[USB CDC TRACE] DVST is0=0x5090 ...              ← SPD_DFLT → usb_device_bus_reset() fires HERE
[USB CDC TRACE] state=BUS_RESET_REARM ...        ← EP0 re-armed immediately
[USB CDC TRACE] CTRT is0=... CTSQ=RDDS(1) ...   ← GET_DESCRIPTOR arrives ← THIS should appear
[USB CDC TRACE] SETUP GET_DESCRIPTOR ...
[USB CDC TRACE] state=GET_DESCRIPTOR ...
[USB CDC TRACE] SETUP SET_ADDRESS ...
[USB CDC TRACE] SETUP SET_CONFIGURATION ...
[USB CDC TRACE] state=SW_CONFIGURED ...
[USB CDC TEST] configured=1
[USB CDC TEST] seq=0 tick=... host-ready         ← CDC data flowing
```

---

## References

- RA6M5 User Manual Hardware, Section 37 (USB 2.0 Full-Speed Module): DVSQ field encoding
  - `0x00` = Powered, `0x10` = Default, `0x20` = Address, `0x30` = Configured
  - `0x40` = Powered+Suspend, `0x50` = Default+Suspend, `0x60` = Address+Suspend, `0x70` = Configured+Suspend
  - **`DVSQ[6:4]` not `[3:0]`** — the lower bits map to CTSQ, upper bits to DVSQ
- USB 2.0 Spec §9.1.1: Device speed detection occurs at end of bus reset, host must see J-state
  within 2.5 µs after reset ends
- [[HW_RA6M5_USBFS]] — register map and event source breakdown
- [[FW_USB_Driver]] — driver scope, pipe layout, test integration
