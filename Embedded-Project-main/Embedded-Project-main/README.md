# Edge AI IAQ Prediction System — CK-RA6M5 + ESP32-C6

End-to-end indoor air quality (IAQ) forecasting pipeline from a Renesas
CK-RA6M5 MCU edge node to a Python server, with automatic model retraining
and over-the-air (OTA) model delivery back to the board.

The system runs the full ML inference loop on the MCU using TFLite Micro,
streams results to a FastAPI + MQTT backend over WiFi (via ESP32-C6), and
delivers retrained models to the MCU without reflashing — all while tolerating
MCU resets and WiFi dropouts through NVS persistence and FreeRTOS task design.

---

## System Architecture

```
Sensor Layer
ZMOD4410 ──I2C──► CK-RA6M5 (Cortex-M33, TFLite Micro inference)
                      |  UART 115200 8N1  GPIO16/17
                  ESP32-C6 (FreeRTOS: UART logger / MQTT / OTA tasks)
                      |  WiFi 2.4 GHz
        ┌─────────────┴────────────────────────────┐
  MQTT broker (1883)                 FastAPI (8000)
        |                                  |
  SQLite database               ai_engine/ retraining
        |                                  |
  Streamlit dashboard (8501)    updates/iaq_model.tflite
                                           |
                         /api/v1/model/version ◄─ ESP32 poll (60 s)
                         /api/v1/model/latest  ◄─ ESP32 download
                                           |  UART OTA (CRC-framed)
                                CK-RA6M5 Data Flash (0x08000000)
```

### Data flow — normal operation (every 5 s)
1. RA6M5 reads ZMOD4410 (or built-in simulator) → runs TFLite inference.
2. RA6M5 prints `"Published: TVOC=…ppb | Actual=… | Predict=…"` over UART.
3. ESP32-C6 `task_uart_logger` receives the text line, prints it to USB Serial
   **unconditionally** (regardless of WiFi/MQTT state), and enqueues it.
4. `task_net_manager` drains the queue and publishes to MQTT `iaq/node/data`.
5. Server MQTT client parses the payload and saves it to SQLite.
6. Every 500 samples the server triggers an incremental model retrain (async).
7. Retrained model is exported to `updates/iaq_model.tflite`.

### Data flow — OTA model update (every 60 s ESP32 poll)
1. `task_ota_checker` calls `GET /api/v1/model/version` → `{"version": <size>}`.
2. If version differs, it calls `GET /api/v1/model/latest` to download binary.
3. ESP32 acquires `g_serial1_mutex`, sets `g_ota_busy`, sends the model to
   RA6M5 via UART using CMD_START / CMD_DATA×N / CMD_END framing.
4. RA6M5 `fwupdate_receiver` verifies CRC, writes model to Data Flash, writes
   NVS metadata magic at `0x08001FC0`.
5. On next RA6M5 reset, `IAQ_Init()` finds the NVS magic and loads the model
   from Data Flash instead of the compiled-in fallback.

---

## Repository Layout

```
Embedded-Project-main/
├── backend/
│   ├── main.py              FastAPI server (MQTT + REST API + model serving)
│   ├── mqtt_client.py       MQTT subscriber — parse & store IAQ data
│   └── database_manager.py  SQLite helpers
├── ai_engine/
│   ├── retraining_script.py Incremental fine-tuning (every 500 samples)
│   └── model_exporter.py    H5 → TFLite → C-header converter
├── frontend/
│   └── app.py               Streamlit dashboard
├── updates/                 Auto-generated output of retraining
│   ├── iaq_model.tflite     Served by /api/v1/model/latest
│   ├── iaq_model_data.h
│   └── scaler_constants.h
└── REQUIREMENT.txt

RENESAS_DRIVER/src/
├── iaq_predictor.cpp        TFLite Micro inference + Data Flash model boot
├── scaler_constants.h       Z-score normalisation coefficients
├── iaq_model_data.h         Compiled-in fallback model
└── server_comm.c            RTOS tasks: IAQ loop + fwupdate RX

RENESAS_DRIVER/Middleware/FWUpdate/
├── fwupdate_receiver.c      UART frame receiver → Data Flash writer
└── fwupdate_receiver.h      OTA protocol constants

ESP32C6_SENDER/
└── ESP32C6_SENDER.ino       FreeRTOS UART logger + MQTT bridge + OTA updater
```

---

## Prerequisites

### For a Raspberry Pi server (recommended)
| Requirement | Notes |
|---|---|
| Raspberry Pi 3 / 4 / 5 | Any model with 2 GB+ RAM recommended |
| Raspberry Pi OS (64-bit) | Bookworm or Bullseye |
| Python 3.10+ | `python3 --version` |
| pip3 | `sudo apt install python3-pip` |
| Mosquitto MQTT | installed as shown in Step 1 |
| Static LAN IP | set in router DHCP or via `/etc/dhcpcd.conf` |

### For a regular Linux / macOS / Windows PC
| Tool | Version |
|---|---|
| Python | >= 3.10 |
| Mosquitto MQTT broker | >= 2.0 |
| pip packages | see `REQUIREMENT.txt` |

### RA6M5 Board
| Tool | Version |
|---|---|
| CMake | >= 3.20 |
| GCC ARM Embedded | arm-none-eabi-gcc 12+ |
| J-Link or E2 Lite debugger | for initial flash |

### ESP32-C6
| Tool | Version |
|---|---|
| Arduino IDE | >= 2.x or PlatformIO |
| Arduino-ESP32 core | >= 3.3.8 |
| PubSubClient library | >= 2.8 (Nick O'Leary) |

---

## Step 1 — Server Setup

This section covers both **Raspberry Pi** and regular Linux/PC setups.

---

### Option A — Raspberry Pi (recommended for always-on deployment)

#### 1a.1 First-time Raspberry Pi preparation

```bash
# Update the system
sudo apt update && sudo apt upgrade -y

# Install required system packages
sudo apt install -y python3-pip python3-venv git mosquitto mosquitto-clients

# Enable and start Mosquitto
sudo systemctl enable mosquitto
sudo systemctl start mosquitto
sudo systemctl status mosquitto   # should show "active (running)"
```

#### 1a.2 Configure Mosquitto to accept external connections

By default Mosquitto 2.x binds to localhost only. Edit the config:

```bash
sudo nano /etc/mosquitto/conf.d/local.conf
```

Add the following lines and save:
```
listener 1883
allow_anonymous true
```

Restart Mosquitto:
```bash
sudo systemctl restart mosquitto
```

Verify the broker is listening on all interfaces:
```bash
sudo ss -tlnp | grep 1883
# Should show: 0.0.0.0:1883
```

#### 1a.3 Clone and set up the project

```bash
git clone <YOUR_REPO_URL> ~/iaq-system
cd ~/iaq-system/Embedded-Project-main

# Create a virtual environment (keeps system Python clean)
python3 -m venv .venv
source .venv/bin/activate

# Install Python dependencies
pip install -r REQUIREMENT.txt
```

#### 1a.4 Find the Raspberry Pi's IP address

```bash
hostname -I
# Example output: 192.168.1.101 ...
```

Use this IP as `SERVER_IP` in `ESP32C6_SENDER.ino`.

#### 1a.5 Start the FastAPI backend

```bash
cd ~/iaq-system/Embedded-Project-main
source .venv/bin/activate
python -m uvicorn backend.main:app --host 0.0.0.0 --port 8000
```

#### 1a.6 (Optional) Run as systemd services for auto-start

Create a service file for the FastAPI backend:
```bash
sudo nano /etc/systemd/system/iaq-backend.service
```

Paste (replace `/home/pi` with your actual home directory):
```ini
[Unit]
Description=IAQ FastAPI Backend
After=network.target mosquitto.service

[Service]
User=pi
WorkingDirectory=/home/pi/iaq-system/Embedded-Project-main
ExecStart=/home/pi/iaq-system/Embedded-Project-main/.venv/bin/python \
          -m uvicorn backend.main:app --host 0.0.0.0 --port 8000
Restart=always
RestartSec=5

[Install]
WantedBy=multi-user.target
```

Enable and start it:
```bash
sudo systemctl daemon-reload
sudo systemctl enable iaq-backend
sudo systemctl start iaq-backend
sudo systemctl status iaq-backend
```

Create a service for the Streamlit dashboard:
```bash
sudo nano /etc/systemd/system/iaq-dashboard.service
```
```ini
[Unit]
Description=IAQ Streamlit Dashboard
After=network.target iaq-backend.service

[Service]
User=pi
WorkingDirectory=/home/pi/iaq-system/Embedded-Project-main
ExecStart=/home/pi/iaq-system/Embedded-Project-main/.venv/bin/streamlit \
          run frontend/app.py --server.port 8501 --server.address 0.0.0.0
Restart=always
RestartSec=5

[Install]
WantedBy=multi-user.target
```
```bash
sudo systemctl daemon-reload
sudo systemctl enable iaq-dashboard
sudo systemctl start iaq-dashboard
```

#### 1a.7 Open firewall ports (if ufw is active)

```bash
sudo ufw allow 1883/tcp   # MQTT broker
sudo ufw allow 8000/tcp   # FastAPI
sudo ufw allow 8501/tcp   # Streamlit dashboard
sudo ufw reload
sudo ufw status
```

#### 1a.8 Verify services are reachable from another machine

```bash
# From any machine on the same LAN (replace 192.168.1.101 with RPi IP)
curl http://192.168.1.101:8000/
# Expected: {"status":"ok"} or similar

mosquitto_pub -h 192.168.1.101 -t test -m "hello"
# No error = broker reachable
```

---

### Option B — Linux or macOS PC

```bash
# Install Mosquitto
sudo apt install mosquitto mosquitto-clients   # Debian/Ubuntu
brew install mosquitto                         # macOS

# Allow external connections (same conf edit as 1a.2 above)

# Install Python deps
pip install -r REQUIREMENT.txt

# Start backend
python -m uvicorn backend.main:app --host 0.0.0.0 --port 8000 --reload
```

### Option C — Windows PC

1. Download Mosquitto from https://mosquitto.org/download/ and install.
2. Edit `C:\Program Files\mosquitto\mosquitto.conf`: add `listener 1883` and
   `allow_anonymous true`.
3. Run `net start mosquitto` (or start as a Windows service).
4. Run `pip install -r REQUIREMENT.txt`.
5. Run `python -m uvicorn backend.main:app --host 0.0.0.0 --port 8000 --reload`.

### Server API endpoints

| Endpoint | Method | Purpose |
|---|---|---|
| `/` | GET | Health check |
| `/api/v1/latest` | GET | Latest IAQ record |
| `/api/v1/history?limit=N` | GET | Last N records |
| `/api/v1/retrain` | POST | Manually trigger retraining |
| `/api/v1/model/version` | GET | Model version token (file size) |
| `/api/v1/model/latest` | GET | Download TFLite binary |

---

## Step 2 — Flash the RA6M5 Firmware

### 2.1 Build
```bash
cd RENESAS_DRIVER
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

### 2.2 Flash
Use the provided script (requires J-Link):
```bash
build_and_flash.bat      # Windows
./build_and_flash.sh     # Linux / macOS
```

The compiled-in model (`src/iaq_model_data.h`) is the fallback used on first
boot. No separate model flash step is needed — the OTA pipeline replaces it.

### 2.3 Debug output

By default the firmware uses USB CDC for debug output (`OS_DEBUG_BACKEND_USB_CDC=1`
in `Config/rtos_config.h`). Connect the CK-RA6M5 USB port and open a serial
monitor at any baud rate (USB CDC auto-negotiates).

To switch to J-Link VCOM UART (SCI1, P709 TX / P708 RX), set
`OS_DEBUG_BACKEND_UART=1` and `OS_DEBUG_BACKEND_USB_CDC=0` in `rtos_config.h`.

### 2.4 UART wiring — RA6M5 to ESP32-C6

| Signal | RA6M5 pin | ESP32-C6 GPIO |
|---|---|---|
| UART TX (IAQ data out) | P302 (UART2 TX) | GPIO16 (RX) |
| UART RX (OTA model in) | P301 (UART2 RX) | GPIO17 (TX) |
| GND | GND | GND |

Baud: **115200**, 8N1. Both boards must share a common GND.

### 2.5 Safe Reset and crash logging

The firmware includes a crash-recovery module (`src/safe_reset.c`).
On power-up the boot log will show if a previous crash was detected:

```
[SAFE_RESET] Previous crash detected: reason=0x00000002  count=1
```

Crash reason codes:
| Code | Meaning |
|---|---|
| 0x00000001 | Stack overflow (canary corrupted) |
| 0x00000002 | Hardware fault (HardFault / BusFault / UsageFault) |
| 0x00000003 | Watchdog timeout |
| 0x00000004 | Application-requested reset (e.g. self-test) |

---

## Step 3 — Configure and Flash the ESP32-C6

### 3.1 Edit credentials

Open `ESP32C6_SENDER/ESP32C6_SENDER.ino` and edit the user configuration
block near the top:

```cpp
#define WIFI_SSID    "YOUR_WIFI_SSID"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"
#define SERVER_IP    "192.168.1.101"   // IP of the Raspberry Pi or PC
```

`SERVER_IP` must be reachable by the ESP32 on the same LAN.

### 3.2 Install required library

Arduino IDE: **Sketch → Include Library → Manage Libraries** →
search `PubSubClient` by Nick O'Leary → Install.

### 3.3 Flash the ESP32-C6

- Board: **ESP32C6 Dev Module**
- Partition scheme: default
- Upload via USB-CDC or JTAG

### 3.4 FreeRTOS task design

The sketch runs three independent FreeRTOS tasks:

| Task | Stack | Priority | Responsibility |
|---|---|---|---|
| `task_uart_logger` | 4 KB | 3 (high) | Read RA6M5 UART; print ALL lines to Serial regardless of network state |
| `task_net_manager` | 6 KB | 2 | WiFi + MQTT keepalive; publish queued IAQ lines |
| `task_ota_checker` | 8 KB | 1 (low) | Poll server every 60 s; download and push new model |

**UART logging is unconditional** — `task_uart_logger` prints every received
line to USB Serial even when WiFi is down or MQTT is disconnected. This
ensures RA6M5 debug output is always visible in the Arduino Serial Monitor.

During an OTA transfer, `g_ota_busy` is set and `task_uart_logger` pauses
to prevent its reads from interfering with the ACK/NACK exchange.

### 3.5 Expected Serial Monitor output

Normal operation:
```
[BOOT] ESP32-C6 IAQ Bridge & OTA Updater (FreeRTOS)
[BOOT] UART1 RX=GPIO16 TX=GPIO17 @ 115200 baud
[BOOT] Serial1 open -- RA6M5 output will appear below:
[BOOT] All tasks started
[WiFi] Connecting to "MyNetwork"...
[WiFi] Connected -- IP: 192.168.1.105
[MQTT] Connecting to 192.168.1.101:1883 ...
[MQTT] Connected
[RA6M5] [IAQ Task] Starting TFLite initialization...
[RA6M5] [IAQ Task] Init OK. Starting 5s forecast loop...
[RA6M5] Published: TVOC=144.0ppb | Actual=1.86 | Predict=1.80
[MQTT->] Published: TVOC=144.0ppb | Actual=1.86 | Predict=1.80
```

When WiFi drops temporarily:
```
[RA6M5] Published: TVOC=210.5ppb | Actual=2.11 | Predict=2.09
[MQTT offline] dropped: Published: TVOC=210.5ppb ...
```
Note: RA6M5 lines are **always** printed even while MQTT is offline.

When a retrain completes and a new model is detected:
```
[OTA] New model (server=4612, local=4456)
[OTA] Downloading model binary from server...
[OTA] Downloaded 4612 bytes -- pushing to RA6M5
[OTA] Progress: 128 / 4612 bytes
...
[OTA] Transfer complete -- RA6M5 verified and written to Data Flash
[OTA] Update successful
```

---

## Step 4 — Verify End-to-End Operation

### 4.1 Check data is reaching the database

```bash
curl http://192.168.1.101:8000/api/v1/latest
```

Expected:
```json
{
  "id": 42,
  "timestamp": "2026-05-11 09:15:00",
  "tvoc": 144.0,
  "iaq_actual": 1.86,
  "iaq_forecast": 1.80,
  "temperature": 31.1,
  "humidity": 46.9
}
```

### 4.2 Manually trigger retraining

```bash
curl -X POST http://192.168.1.101:8000/api/v1/retrain
```

### 4.3 Check model version

```bash
curl http://192.168.1.101:8000/api/v1/model/version
# {"version": 4612}
```

### 4.4 Confirm RA6M5 loaded the OTA model

After the ESP32 completes the OTA push and the RA6M5 resets, the debug
output will show:
```
IAQ: using OTA model from Data Flash (4612 bytes)
[IAQ Task] Init OK. Starting 5s forecast loop...
```
If no OTA has been received:
```
IAQ: using built-in model (4456 bytes)
```

### 4.5 Open the Streamlit dashboard

Navigate to `http://192.168.1.101:8501` in a browser.

---

## OTA Model Persistence (NVS Layout)

The RA6M5 Data Flash (8 KB at `0x08000000`) is laid out as follows:

```
0x08000000  +--------------------------+
            |  OTA model binary        |  up to 8128 bytes
            |  (written by fwupdate)   |
0x08001FC0  +--------------------------+
            |  NVS metadata (64 bytes) |
            |  [0..3]   0xDEADBEEF    |  OTA valid magic
            |  [4..7]   model_len BE  |
            |  [8..9]   model_crc16   |
            |  [10..11] 0xFF          |
            |  [12..15] crash magic   |  written by safe_reset
            |  [16..19] crash reason  |
            |  [20..23] reset count   |
            |  [24..63] 0xFF          |
0x08002000  +--------------------------+
```

---

## MQTT Data Processing

The MQTT client (`backend/mqtt_client.py`) subscribes to `iaq/node/data`
and parses lines using regex patterns:

| Pattern | Extracted field |
|---|---|
| `TVOC=(\d+\.?\d*)\s*ppb` | tvoc |
| `Actual=(\d+\.?\d*)` | iaq_actual |
| `Predict=(\d+\.?\d*)` | iaq_forecast |
| `T=(\d+\.?\d*)\s*C` | temperature |
| `RH=(\d+\.?\d*)%` | humidity |

---

## Troubleshooting

| Symptom | Likely cause | Fix |
|---|---|---|
| ESP32 `[OTA] Version check HTTP -1` | Server not reachable | Check `SERVER_IP`; verify firewall allows port 8000 |
| ESP32 `[MQTT] Failed (state=-2)` | Broker down or wrong IP | Start Mosquitto; check `SERVER_IP` |
| ESP32 shows `[RA6M5]` lines but no `[MQTT->]` | MQTT disconnected | `task_net_manager` will reconnect; lines are logged but dropped from MQTT |
| RA6M5 `AllocateTensors() failed` | Retrained model exceeds arena | Increase `kTensorArenaSize` in `iaq_predictor.cpp` |
| OTA `CMD_END NACK reason=0x06` | Image CRC mismatch | WiFi drop during download; ESP32 retries on next 60 s poll |
| RA6M5 halts completely (no LED) | HardFault caught by handler | Check boot log for `[SAFE_RESET] Previous crash detected` |
| Dashboard shows no data | Parser regex mismatch | Confirm RA6M5 output contains `"Published:"` |
| Raspberry Pi service crashes | Missing Python dep | Run `pip install -r REQUIREMENT.txt` inside `.venv` |
| `mosquitto: bind: Address already in use` | Another broker instance | `sudo systemctl stop mosquitto; sudo systemctl start mosquitto` |

---

## Hardware Summary

| Component | Part | Role |
|---|---|---|
| MCU board | CK-RA6M5 (R7FA6M5BH3CFC) | Edge inference, OTA receiver |
| Gas sensor | ZMOD4410 | TVOC / CO2 measurement |
| WiFi bridge | ESP32-C6 | MQTT publisher, OTA downloader |
| Server | Raspberry Pi 4 (or PC) | MQTT broker, FastAPI, SQLite, Streamlit |

---

*For implementation details see `RENESAS_DRIVER/imple_doc/INDEX.md`.*
