/**
 * ESP32C6_SENDER.ino  â€”  IAQ UART-MQTT Bridge & OTA Model Updater
 *
 * Roles:
 *   1. UART-to-MQTT Bridge: reads debug text lines from the RA6M5 UART and
 *      publishes them to the MQTT topic "iaq/node/data" for the server to
 *      store in the database and trigger retraining.
 *
 *   2. OTA Model Updater: every MODEL_POLL_INTERVAL_MS (default 60 s) the
 *      ESP32 polls the FastAPI server for a new retrained model. If a newer
 *      version is detected it downloads the binary over HTTP, then pushes it
 *      to the RA6M5 over UART using the custom framed protocol. The RA6M5
 *      writes the model to Data Flash and switches to it on the next boot.
 *
 * UART1 wiring (ESP32-C6):
 *   TX = GPIO17  â†’  RA6M5 UART2 RX (P301)   â€” sends model OTA frames
 *   RX = GPIO16  â†  RA6M5 UART2 TX (P302)   â€” receives IAQ text + ACK/NACK
 *   GND â†’ GND  (common ground REQUIRED)
 *
 * Network topology:
 *   WiFi AP  â”€â”€  ESP32-C6  â”€â”€[UART]â”€â”€  RA6M5
 *                   â”‚
 *              MQTT broker (localhost on the server PC, port 1883)
 *              FastAPI server (port 8000)
 *                   â”‚
 *              /api/v1/model/version  â†’ {"version": <uint32 file-size>}
 *              /api/v1/model/latest   â†’ binary TFLite flatbuffer
 *
 * OTA frame protocol (matches fwupdate_receiver.h on RA6M5):
 *   [STX 0x02][CMD][LEN_MSB][LEN_LSB][DATAâ€¦][CRC_MSB][CRC_LSB][ETX 0x03]
 *   CRC-16-CCITT (XMODEM): poly=0x1021, init=0xFFFF, no reflect.
 *   CMD_START(0x01) 4-byte big-endian total length
 *   CMD_DATA (0x02) up to 128 bytes per frame
 *   CMD_END  (0x03) 2-byte big-endian full-image CRC
 *
 * Required Arduino libraries (install via Library Manager):
 *   - PubSubClient  (Nick O'Leary)
 *   WiFi and HTTPClient are bundled with the ESP32 Arduino core.
 */

#include <WiFi.h>
#include <HTTPClient.h>
#include <PubSubClient.h>
#include <Arduino.h>

/* -----------------------------------------------------------------------
 * USER CONFIGURATION â€” edit before flashing
 * ----------------------------------------------------------------------- */
#define WIFI_SSID            "YOUR_WIFI_SSID"
#define WIFI_PASSWORD        "YOUR_WIFI_PASSWORD"

/* IP / hostname of the machine running FastAPI and the MQTT broker */
#define SERVER_IP            "192.168.1.100"
#define MQTT_BROKER_PORT     1883
#define SERVER_BASE_URL      "http://" SERVER_IP ":8000"

/* Unique identifier shown in MQTT client list */
#define DEVICE_ID            "esp32c6_iaq_01"

/* -----------------------------------------------------------------------
 * UART configuration
 * ----------------------------------------------------------------------- */
#define UART1_TX_PIN         17
#define UART1_RX_PIN         16
#define UART_BAUD            115200UL

/* Max length of one text line received from RA6M5 */
#define UART_LINE_BUF_LEN    256U

/* -----------------------------------------------------------------------
 * OTA protocol constants (must mirror fwupdate_receiver.h on RA6M5)
 * ----------------------------------------------------------------------- */
#define STX                  0x02U
#define ETX                  0x03U
#define CMD_START            0x01U
#define CMD_DATA             0x02U
#define CMD_END              0x03U
#define CMD_ACK              0xAAU
#define CMD_NACK             0xFFU

#define DATA_BLOCK_SIZE      128U    /* max payload bytes per CMD_DATA frame */
#define ACK_TIMEOUT_MS       500U    /* ms to wait for ACK/NACK per frame    */
#define MAX_RETRIES          3U      /* retries before aborting transfer      */
#define INTER_FRAME_DELAY    20U     /* ms between frames (flash write time)  */

/* Maximum model binary the ESP32 heap can hold (RA6M5 Data Flash = 8 KB) */
#define MODEL_MAX_SIZE       8192UL

/* -----------------------------------------------------------------------
 * Timing
 * ----------------------------------------------------------------------- */
#define MODEL_POLL_INTERVAL_MS   (60UL * 1000UL)
#define MQTT_KEEPALIVE_S         60U

/* -----------------------------------------------------------------------
 * MQTT topic
 * ----------------------------------------------------------------------- */
#define MQTT_TOPIC_DATA      "iaq/node/data"

/* -----------------------------------------------------------------------
 * Global objects
 * ----------------------------------------------------------------------- */
static WiFiClient   s_wifi_client;
static PubSubClient s_mqtt(s_wifi_client);

/* Downloaded model buffer â€” allocated on first successful download */
static uint8_t *    s_model_buf     = nullptr;
static uint32_t     s_model_buf_len = 0UL;

/* Last model version (file-size reported by server) used to detect updates */
static uint32_t     s_last_model_version = 0UL;

/* Timer for periodic model polls */
static uint32_t     s_last_model_poll_ms = 0UL;

/* Line accumulation buffer for RA6M5 debug text output */
static char         s_line_buf[UART_LINE_BUF_LEN];
static uint16_t     s_line_idx = 0U;

/* -----------------------------------------------------------------------
 * Hardware / Protocol Constants
 * ----------------------------------------------------------------------- */
#define UART1_TX_PIN      14
#define UART1_RX_PIN      15
#define UART_BAUD         115200UL

#define STX               0x02U
#define ETX               0x03U

#define CMD_START         0x01U
#define CMD_DATA          0x02U
#define CMD_END           0x03U

#define CMD_ACK           0xAAU
#define CMD_NACK          0xFFU

/* NACK reason codes (matches RA6M5 receiver) */
#define NACK_CRC          0x01U
#define NACK_SEQUENCE     0x02U
#define NACK_FLASH        0x03U
#define NACK_LENGTH       0x04U
#define NACK_VERIFY       0x05U
#define NACK_IMAGE_CRC    0x06U

#define DATA_BLOCK_SIZE   128U          /* bytes per CMD_DATA frame           */
#define ACK_TIMEOUT_MS    500U          /* ms to wait for ACK/NACK per frame  */
#define MAX_RETRIES       3U            /* retries before giving up           */
#define INTER_FRAME_DELAY 20U           /* ms between frames                  */

/* =========================================================================
 * CRC-16-CCITT (XMODEM) â€” matches RA6M5 fwupdate_receiver exactly.
 * Polynomial : 0x1021, Init : 0xFFFF, no input/output reflection.
 * ========================================================================= */
static uint16_t crc16_ccitt(const uint8_t *data, uint32_t len)
{
    uint16_t crc = 0xFFFFU;
    for (uint32_t i = 0UL; i < len; i++) {
        crc ^= (uint16_t)((uint16_t)data[i] << 8U);
        for (uint8_t b = 0U; b < 8U; b++) {
            crc = (crc & 0x8000U)
                ? (uint16_t)((uint16_t)(crc << 1U) ^ 0x1021U)
                : (uint16_t)(crc << 1U);
        }
    }
    return crc;
}

/* =========================================================================
 * WiFi helpers
 * ========================================================================= */
static void wifi_connect(void)
{
    Serial.printf("[WiFi] Connecting to \"%s\"", WIFI_SSID);
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.printf("\n[WiFi] Connected â€” IP: %s\n", WiFi.localIP().toString().c_str());
}

static void wifi_ensure(void)
{
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[WiFi] Reconnecting...");
        wifi_connect();
    }
}

/* =========================================================================
 * MQTT helpers
 * ========================================================================= */
static void mqtt_reconnect(void)
{
    uint8_t attempts = 0U;
    while (!s_mqtt.connected() && attempts < 5U) {
        Serial.printf("[MQTT] Connecting to %s:%d ...\n", SERVER_IP, MQTT_BROKER_PORT);
        if (s_mqtt.connect(DEVICE_ID)) {
            Serial.println("[MQTT] Connected");
        } else {
            Serial.printf("[MQTT] Failed (state=%d), retry in 2 s\n", s_mqtt.state());
            delay(2000U);
            attempts++;
        }
    }
}

static void mqtt_ensure(void)
{
    if (!s_mqtt.connected()) {
        mqtt_reconnect();
    }
}

static void mqtt_publish_line(const char *line)
{
    mqtt_ensure();
    if (s_mqtt.connected()) {
        s_mqtt.publish(MQTT_TOPIC_DATA, line);
    }
}

/* =========================================================================
 * RA6M5 UART text bridge
 *
 * RA6M5 debug_print() outputs newline-terminated strings such as:
 *   "Published: TVOC=144.0ppb | Actual=1.86 | Predict=1.80\r\n"
 *   "[sensor:263] T=31.1 C  RH=46.9%\r\n"
 * These are forwarded as-is to the MQTT topic "iaq/node/data" so the
 * server's mqtt_client.py can parse them with its regex patterns.
 * ========================================================================= */
static bool line_is_iaq_data(const char *line)
{
    return (strstr(line, "Published:") != nullptr) ||
           (strstr(line, " T=") != nullptr && strstr(line, "RH=") != nullptr);
}

static void process_uart_rx(void)
{
    while (Serial1.available() > 0) {
        char c = (char)Serial1.read();
        if (c == '\n') {
            /* Null-terminate, strip trailing \r */
            if (s_line_idx > 0U && s_line_buf[s_line_idx - 1U] == '\r') {
                s_line_buf[s_line_idx - 1U] = '\0';
            } else {
                s_line_buf[s_line_idx] = '\0';
            }
            if (s_line_idx > 0U && line_is_iaq_data(s_line_buf)) {
                Serial.printf("[Bridgeâ†’MQTT] %s\n", s_line_buf);
                mqtt_publish_line(s_line_buf);
            }
            s_line_idx = 0U;
        } else {
            if (s_line_idx < (UART_LINE_BUF_LEN - 1U)) {
                s_line_buf[s_line_idx++] = c;
            } else {
                s_line_idx = 0U;   /* buffer overrun â€” discard */
            }
        }
    }
}

/* =========================================================================
 * OTA UART protocol â€” send model binary to RA6M5
 * ========================================================================= */
static void ota_send_frame(uint8_t cmd, const uint8_t *data, uint16_t data_len)
{
    uint8_t  crc_buf[3U + DATA_BLOCK_SIZE + 4U];
    uint16_t crc_len = (uint16_t)(3U + data_len);

    crc_buf[0] = cmd;
    crc_buf[1] = (uint8_t)((data_len >> 8U) & 0xFFU);
    crc_buf[2] = (uint8_t)(data_len & 0xFFU);
    if (data != nullptr && data_len > 0U) {
        memcpy(&crc_buf[3U], data, data_len);
    }
    uint16_t crc = crc16_ccitt(crc_buf, (uint32_t)crc_len);

    Serial1.write(STX);
    Serial1.write(cmd);
    Serial1.write(crc_buf[1]);      /* LEN_MSB */
    Serial1.write(crc_buf[2]);      /* LEN_LSB */
    if (data != nullptr && data_len > 0U) {
        Serial1.write(data, data_len);
    }
    Serial1.write((uint8_t)((crc >> 8U) & 0xFFU));
    Serial1.write((uint8_t)(crc & 0xFFU));
    Serial1.write(ETX);
    Serial1.flush();
}

static uint8_t ota_wait_ack(uint32_t timeout_ms)
{
    uint8_t  buf[16];
    uint8_t  idx     = 0U;
    bool     got_stx = false;
    uint32_t start   = millis();

    while ((millis() - start) < timeout_ms) {
        if (Serial1.available() > 0) {
            uint8_t b = (uint8_t)Serial1.read();
            if (!got_stx) {
                if (b == STX) { got_stx = true; idx = 0U; }
                continue;
            }
            if (idx < (uint8_t)sizeof(buf)) { buf[idx++] = b; }

            /* Minimum ACK/NACK frame: CMD+LEN_MSB+LEN_LSB+DATA+CRC_MSB+CRC_LSB+ETX = 7 */
            if (idx >= 7U && buf[idx - 1U] == ETX) {
                uint8_t rc = buf[0];
                if (idx == 7U && (rc == CMD_ACK || rc == CMD_NACK)) {
                    /* Validate response CRC */
                    uint8_t  rcrc[4] = { buf[0], buf[1], buf[2], buf[3] };
                    uint16_t exp_crc = crc16_ccitt(rcrc, 4U);
                    uint16_t got_crc = (uint16_t)((uint16_t)buf[4] << 8U) | buf[5];
                    if (exp_crc == got_crc) { return rc; }
                }
                got_stx = false; idx = 0U;
            }
        }
        yield();
    }
    return 0U;   /* timeout */
}

static bool ota_send_model(const uint8_t *model, uint32_t model_len)
{
    uint16_t image_crc = crc16_ccitt(model, model_len);

    /* ---- CMD_START ---- */
    {
        uint8_t payload[4] = {
            (uint8_t)(model_len >> 24U), (uint8_t)(model_len >> 16U),
            (uint8_t)(model_len >> 8U),  (uint8_t)(model_len)
        };
        bool ok = false;
        for (uint8_t t = 0U; t < MAX_RETRIES; t++) {
            ota_send_frame(CMD_START, payload, 4U);
            if (ota_wait_ack(ACK_TIMEOUT_MS) == CMD_ACK) { ok = true; break; }
            delay(100U);
        }
        if (!ok) { Serial.println("[OTA] CMD_START failed"); return false; }
    }

    /* ---- CMD_DATA blocks ---- */
    uint32_t offset = 0UL;
    while (offset < model_len) {
        uint32_t chunk = model_len - offset;
        if (chunk > DATA_BLOCK_SIZE) { chunk = DATA_BLOCK_SIZE; }

        bool ok = false;
        for (uint8_t t = 0U; t < MAX_RETRIES; t++) {
            ota_send_frame(CMD_DATA, model + offset, (uint16_t)chunk);
            if (ota_wait_ack(ACK_TIMEOUT_MS) == CMD_ACK) { ok = true; break; }
            delay(50U);
        }
        if (!ok) {
            Serial.printf("[OTA] CMD_DATA failed at offset %lu\n", offset);
            return false;
        }
        offset += chunk;
        Serial.printf("[OTA] Progress: %lu / %lu bytes\n", offset, model_len);
        delay(INTER_FRAME_DELAY);
    }

    /* ---- CMD_END ---- */
    {
        uint8_t payload[2] = {
            (uint8_t)((image_crc >> 8U) & 0xFFU),
            (uint8_t)(image_crc & 0xFFU)
        };
        bool ok = false;
        for (uint8_t t = 0U; t < MAX_RETRIES; t++) {
            ota_send_frame(CMD_END, payload, 2U);
            /* Allow extra time for flash erase + verify on RA6M5 */
            if (ota_wait_ack(2000U) == CMD_ACK) { ok = true; break; }
            delay(200U);
        }
        if (!ok) { Serial.println("[OTA] CMD_END verify failed"); return false; }
    }

    Serial.println("[OTA] Transfer complete â€” RA6M5 verified and written to Data Flash");
    return true;
}

/* =========================================================================
 * Server model version check & binary download
 * ========================================================================= */

/**
 * Poll GET /api/v1/model/version â†’ {"version": <uint32>}
 * The server returns the TFLite file size as the version token.
 * Returns 0 on error or if no model exists yet.
 */
static uint32_t fetch_model_version(void)
{
    HTTPClient http;
    String url = String(SERVER_BASE_URL) + "/api/v1/model/version";
    http.begin(url);
    http.setTimeout(5000U);
    int code = http.GET();

    uint32_t version = 0UL;
    if (code == 200) {
        String body = http.getString();
        int key_pos = body.indexOf("\"version\"");
        if (key_pos >= 0) {
            int colon = body.indexOf(':', key_pos);
            if (colon >= 0) {
                version = (uint32_t)body.substring(colon + 1).toInt();
            }
        }
    } else {
        Serial.printf("[OTA] Version check HTTP %d\n", code);
    }
    http.end();
    return version;
}

/**
 * Download GET /api/v1/model/latest into s_model_buf, then push to RA6M5.
 * Returns true on full end-to-end success.
 */
static bool download_and_push_model(void)
{
    HTTPClient http;
    String url = String(SERVER_BASE_URL) + "/api/v1/model/latest";

    Serial.println("[OTA] Downloading model binary from server...");
    http.begin(url);
    http.setTimeout(30000U);
    int code = http.GET();

    if (code != 200) {
        Serial.printf("[OTA] Download HTTP %d\n", code);
        http.end();
        return false;
    }

    int content_len = http.getSize();
    if (content_len <= 0 || (uint32_t)content_len > MODEL_MAX_SIZE) {
        Serial.printf("[OTA] Bad content-length: %d\n", content_len);
        http.end();
        return false;
    }
    uint32_t model_len = (uint32_t)content_len;

    /* Reallocate buffer */
    if (s_model_buf != nullptr) { free(s_model_buf); s_model_buf = nullptr; }
    s_model_buf = (uint8_t *)malloc(model_len);
    if (s_model_buf == nullptr) {
        Serial.println("[OTA] malloc failed");
        http.end();
        return false;
    }
    s_model_buf_len = model_len;

    /* Stream body into buffer */
    WiFiClient *stream = http.getStreamPtr();
    uint32_t read_total  = 0UL;
    uint32_t dl_deadline = millis() + 30000UL;
    while (read_total < model_len && millis() < dl_deadline) {
        if (stream->available() > 0) {
            int n = stream->readBytes(
                (char *)(s_model_buf + read_total),
                (size_t)(model_len - read_total));
            if (n > 0) { read_total += (uint32_t)n; dl_deadline = millis() + 5000UL; }
        } else {
            delay(5U);
        }
    }
    http.end();

    if (read_total != model_len) {
        Serial.printf("[OTA] Incomplete download %lu / %lu\n", read_total, model_len);
        return false;
    }
    Serial.printf("[OTA] Downloaded %lu bytes\n", model_len);
    return ota_send_model(s_model_buf, model_len);
}

static void check_and_update_model(void)
{
    wifi_ensure();
    uint32_t server_ver = fetch_model_version();
    if (server_ver == 0UL) { return; }   /* server unreachable / no model yet */

    if (server_ver != s_last_model_version) {
        Serial.printf("[OTA] New model detected (server=%lu, local=%lu)\n",
                      server_ver, s_last_model_version);
        if (download_and_push_model()) {
            s_last_model_version = server_ver;
        }
    } else {
        Serial.println("[OTA] Model up to date");
    }
}

/* =========================================================================
 * Arduino setup / loop
 * ========================================================================= */
void setup(void)
{
    Serial.begin(115200U);
    while (!Serial && millis() < 3000U) {}
    Serial.println("\n[BOOT] ESP32-C6 IAQ Bridge & OTA Updater");

    /* UART1 â†’ RA6M5 */
    Serial1.begin(UART_BAUD, SERIAL_8N1, UART1_RX_PIN, UART1_TX_PIN);

    /* WiFi */
    wifi_connect();

    /* MQTT */
    s_mqtt.setServer(SERVER_IP, MQTT_BROKER_PORT);
    s_mqtt.setKeepAlive(MQTT_KEEPALIVE_S);
    mqtt_reconnect();

    /* Trigger an immediate model version check on boot */
    s_last_model_poll_ms = millis() - MODEL_POLL_INTERVAL_MS;

    Serial.println("[BOOT] Ready â€” bridging RA6M5 UART to MQTT");
}

void loop(void)
{
    /* Keep MQTT connection alive and process incoming messages */
    s_mqtt.loop();

    /* Bridge RA6M5 debug text to MQTT */
    process_uart_rx();

    /* Periodic OTA model version check */
    if ((millis() - s_last_model_poll_ms) >= MODEL_POLL_INTERVAL_MS) {
        s_last_model_poll_ms = millis();
        check_and_update_model();
    }
}
