# HW_RA6M5_USBFS

Tags: #in-progress #hardware #usb #usbfs

USB Full-Speed controller (USBFS0) on RA6M5 used in both Device and Host roles.
Firmware layer using this block: [[FW_USB_Driver]].

---

## Peripheral Map

- Base address used by current driver: `0x40090000` (USBFS0)
- Data path blocks in use:
  - Device control pipe: EP0 / DCP (`DCPMAXP`, `DCPCTR`, setup packet regs)
  - FIFO ports: `CFIFO`, `D0FIFO`, `D1FIFO`
  - Pipe window: `PIPESEL`, `PIPECFG`, `PIPEMAXP`, `PIPECTR`

---

## Core Registers Used

| Register | Purpose |
|----------|---------|
| `SYSCFG` | USB engine enable, role select (host/device), pull-up/pull-down |
| `SYSSTS0` | Line state readback (`D+`/`D-`) for attach detection |
| `DVSTCTR0` | Bus activity, reset signaling |
| `CFIFO*` / `D0FIFO*` / `D1FIFO*` | Data transfer through FIFO ports |
| `USBREQ/USBVAL/USBINDX/USBLENG` | SETUP packet staging for control transfer |
| `DCPMAXP`, `DCPCTR` | Endpoint 0 (control pipe) packet and PID control |
| `PIPECFG`, `PIPEMAXP`, `PIPECTR` | Bulk endpoint configuration and transfer control |
| `INTENB0/1`, `INTSTS0/1` | Interrupt enable/status (USBI0/USBI1 event sources) |
| `BRDY*`, `BEMP*`, `NRDY*` | Packet ready/empty/not-ready event groups |

---

## Role-Specific Hardware Notes

### Device CDC Mode

- `SYSCFG.DCFM = 0` (device role)
- `SYSCFG.DPRPU = 1` enables pull-up to announce FS device
- EP0 handles standard + class CDC requests
- Bulk IN endpoint used for logging stream

### Host CDC-ACM Mode

- `SYSCFG.DCFM = 1` (host role)
- `SYSCFG.DRPD = 1` enables host-side pull-downs for attach detection
- VBUS power switch is enabled through board pin `P500` (`USBFS_VBUS_EN`)
- Enumeration uses line-state monitoring (`SYSSTS0.LNST`) + bus reset (`DVSTCTR0`)

---

## Attach/Detach Detection (Host)

Attach state is derived from `SYSSTS0.LNST`:
- J-state or K-state: cable/device attached
- SE0/unknown for timeout window: treated as detach or no device

Bare-metal flow (implemented):
1. Wait attach (poll `LNST` with timeout)
2. Assert/reset bus through `DVSTCTR0`
3. Run EP0 control transfer sequence

---

## Error Sources to Track

- Enumeration timeout (attach wait, setup completion, descriptor read)
- FIFO not-ready (`FRDY` timeout)
- Endpoint not-ready (`NRDY`) during bulk transactions
- Short or malformed descriptor packet while parsing endpoint map

These are surfaced in firmware as `DRV_TIMEOUT` or `DRV_ERR`.

---

## Flow Map (HW Perspective)

`Role select (SYSCFG)`
→ `VBUS / pull-up / pull-down`
→ `Bus state (SYSSTS0, DVSTCTR0)`
→ `EP0 setup/control`
→ `Pipe config (PIPECFG/MAXP)`
→ `Bulk data via FIFO`
→ `Event service (INTSTS0 + BRDY/BEMP/NRDY)`
