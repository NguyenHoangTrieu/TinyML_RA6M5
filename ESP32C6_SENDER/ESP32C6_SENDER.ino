/**
 * ESP32C6_SENDER.ino
 *
 * IAQ Model Firmware Update Sender — ESP32-C6 (Arduino Framework)
 *
 * Reads g_iaq_model_data[] from iaq_model_data.h and transmits it to
 * a Renesas RA6M5 over UART1 using a custom framed protocol.
 *
 * Protocol frame format:
 *   [STX 0x02][CMD][LEN_MSB][LEN_LSB][DATA...][CRC_MSB][CRC_LSB][ETX 0x03]
 *
 * CRC-16-CCITT (XMODEM variant):
 *   Polynomial : 0x1021
 *   Init       : 0xFFFF
 *   No reflect, no final XOR
 *   Computed over: [CMD][LEN_MSB][LEN_LSB][DATA...]
 *
 * Command sequence:
 *   1. Send CMD_START (0x01) with 4-byte big-endian total length
 *   2. Send CMD_DATA  (0x02) in 128-byte blocks until all data sent
 *   3. Send CMD_END   (0x03) with 2-byte big-endian full-image CRC
 *
 * UART1 pins (ESP32-C6, default):
 *   TX = GPIO17 (UART1_TX) → connect to RA6M5 P301 (UART2 RX)
 *   RX = GPIO16 (UART1_RX) → connect to RA6M5 P302 (UART2 TX)
 *   GND → GND (common ground REQUIRED)
 *
 * Inter-frame delay: 20 ms between each block to allow RA6M5 Flash write.
 * ACK/NACK timeout: 500 ms per frame; retries up to 3 times on NACK.
 *
 * Baud rate: 115200 (matches RA6M5 UART2 configuration).
 */

#include "iaq_model_data.h"  /* declares g_iaq_model_data[], g_iaq_model_data_len */

/* -----------------------------------------------------------------------
 * Hardware / Protocol Constants
 * ----------------------------------------------------------------------- */
#define UART1_TX_PIN      17
#define UART1_RX_PIN      16
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

/* -----------------------------------------------------------------------
 * CRC-16-CCITT (XMODEM variant) — IDENTICAL algorithm to RA6M5 side.
 *
 * Polynomial : 0x1021
 * Init       : 0xFFFF
 * Input/output NOT reflected
 *
 * Verified equivalence:
 *   crc16_ccitt({0x01, 0x00, 0x04, 0x00, 0x00, 0x11, 0x68}, 7) → deterministic
 *   Both sides compute the same value for the same byte sequence.
 * ----------------------------------------------------------------------- */
uint16_t crc16_ccitt(const uint8_t* data, uint32_t len) {
    uint16_t crc = 0xFFFFU;
    for (uint32_t i = 0UL; i < len; i++) {
        crc ^= (uint16_t)((uint16_t)data[i] << 8U);
        for (uint8_t bit = 0U; bit < 8U; bit++) {
            if ((crc & 0x8000U) != 0U) {
                crc = (uint16_t)(((uint16_t)(crc << 1U)) ^ 0x1021U);
            } else {
                crc = (uint16_t)(crc << 1U);
            }
        }
    }
    return crc;
}

/* -----------------------------------------------------------------------
 * Compute full-image CRC over the entire model array.
 * Initialised to 0xFFFF — NOT continuing from a per-frame CRC.
 * The RA6M5 receiver also accumulates across all DATA payloads starting
 * from 0xFFFF and compares after CMD_END.
 * ----------------------------------------------------------------------- */
uint16_t compute_image_crc(void) {
    return crc16_ccitt(g_iaq_model_data, (uint32_t)g_iaq_model_data_len);
}

/* -----------------------------------------------------------------------
 * Send a framed packet over UART1.
 *
 * Constructs frame: [STX][cmd][len_msb][len_lsb][data...][crc_msb][crc_lsb][ETX]
 * CRC is computed over [cmd][len_msb][len_lsb][data...]
 *
 * Parameters:
 *   cmd      : command byte
 *   data     : pointer to payload (may be NULL if data_len == 0)
 *   data_len : number of payload bytes (0..128)
 * ----------------------------------------------------------------------- */
void send_frame(uint8_t cmd, const uint8_t* data, uint16_t data_len) {
    /* Build CRC input buffer: [cmd, len_msb, len_lsb, data...] */
    uint8_t  crc_buf[3U + DATA_BLOCK_SIZE + 4U];   /* +4 for CMD_START/END payloads */
    uint16_t crc_len = (uint16_t)(3U + data_len);
    uint16_t crc;

    uint8_t len_msb = (uint8_t)((data_len >> 8U) & 0xFFU);
    uint8_t len_lsb = (uint8_t)(data_len & 0xFFU);

    crc_buf[0] = cmd;
    crc_buf[1] = len_msb;
    crc_buf[2] = len_lsb;

    if (data != nullptr && data_len > 0U) {
        memcpy(&crc_buf[3], data, data_len);
    }

    crc = crc16_ccitt(crc_buf, (uint32_t)crc_len);

    /* Transmit the frame */
    Serial1.write(STX);
    Serial1.write(cmd);
    Serial1.write(len_msb);
    Serial1.write(len_lsb);

    if (data != nullptr && data_len > 0U) {
        Serial1.write(data, data_len);
    }

    Serial1.write((uint8_t)((crc >> 8U) & 0xFFU));
    Serial1.write((uint8_t)(crc & 0xFFU));
    Serial1.write(ETX);

    Serial1.flush();   /* ensure all bytes are transmitted before returning */
}

/* -----------------------------------------------------------------------
 * Wait for ACK or NACK from the RA6M5 receiver.
 *
 * Expected response frame: [STX][CMD_ACK/CMD_NACK][0x00][0x01][data][CRC_MSB][CRC_LSB][ETX]
 * Parses the response and returns the command byte (CMD_ACK or CMD_NACK).
 * Returns 0 on timeout.
 * ----------------------------------------------------------------------- */
uint8_t wait_for_response(uint32_t timeout_ms) {
    uint8_t  resp_buf[16];
    uint8_t  resp_idx   = 0U;
    bool     got_stx    = false;
    uint32_t start_time = millis();

    while ((millis() - start_time) < timeout_ms) {
        if (Serial1.available() > 0) {
            uint8_t b = (uint8_t)Serial1.read();

            if (!got_stx) {
                if (b == STX) {
                    got_stx  = true;
                    resp_idx = 0U;
                }
                continue;
            }

            if (resp_idx < (uint8_t)(sizeof(resp_buf))) {
                resp_buf[resp_idx++] = b;
            }

            /* Minimum response length: CMD(1)+LEN_MSB(1)+LEN_LSB(1)+DATA(1)+CRC(2)+ETX(1)=7 */
            if (resp_idx >= 7U && resp_buf[resp_idx - 1U] == ETX) {
                /* Extract command from response */
                uint8_t resp_cmd = resp_buf[0];

                if (resp_idx != 7U) {
                    got_stx  = false;
                    resp_idx = 0U;
                    continue;
                }

                if ((resp_cmd != CMD_ACK) && (resp_cmd != CMD_NACK)) {
                    got_stx  = false;
                    resp_idx = 0U;
                    continue;
                }

                if ((resp_buf[1] != 0x00U) || (resp_buf[2] != 0x01U)) {
                    got_stx  = false;
                    resp_idx = 0U;
                    continue;
                }

                /* Validate response frame CRC */
                uint8_t  rcrc_buf[4];
                uint16_t received_crc;
                uint16_t computed_crc;

                rcrc_buf[0] = resp_buf[0];   /* cmd      */
                rcrc_buf[1] = resp_buf[1];   /* len_msb  */
                rcrc_buf[2] = resp_buf[2];   /* len_lsb  */
                rcrc_buf[3] = resp_buf[3];   /* data[0]  */

                received_crc  = (uint16_t)((uint16_t)resp_buf[4] << 8U);
                received_crc |= (uint16_t)resp_buf[5];
                computed_crc  = crc16_ccitt(rcrc_buf, 4U);

                if (computed_crc == received_crc) {
                    return resp_cmd;
                } else {
                    /* CRC invalid — restart scan */
                    got_stx  = false;
                    resp_idx = 0U;
                }
            }
        }
        yield();  /* allow Arduino background tasks */
    }

    return 0U;   /* timeout */
}

/* -----------------------------------------------------------------------
 * Transfer status tracking
 * ----------------------------------------------------------------------- */
typedef enum {
    TRANSFER_IDLE    = 0,
    TRANSFER_RUNNING = 1,
    TRANSFER_DONE    = 2,
    TRANSFER_FAILED  = 3
} transfer_status_t;

static transfer_status_t g_transfer_status = TRANSFER_IDLE;

/* -----------------------------------------------------------------------
 * setup() — Arduino entry point
 * ----------------------------------------------------------------------- */
void setup() {
    /* Debug UART (USB CDC) */
    Serial.begin(115200);
    delay(500);
    Serial.println("[SENDER] IAQ Model Firmware Update — ESP32-C6");
    Serial.print("[SENDER] Model size: ");
    Serial.print(g_iaq_model_data_len);
    Serial.println(" bytes");

    /* Data UART (to RA6M5) */
    Serial1.begin(UART_BAUD, SERIAL_8N1, UART1_RX_PIN, UART1_TX_PIN);
    delay(100);

    Serial.println("[SENDER] UART1 ready. Starting transfer in 2 s...");
    delay(2000);
}

/* -----------------------------------------------------------------------
 * loop() — runs the transfer state machine once, then stops.
 * ----------------------------------------------------------------------- */
void loop() {
    if (g_transfer_status != TRANSFER_IDLE) {
        delay(1000);
        return;   /* transfer already done or failed — do nothing */
    }

    g_transfer_status = TRANSFER_RUNNING;

    Serial.println("[SENDER] === Starting IAQ model transfer ===");

    /* ----------------------------------------------------------------
     * Step 1: Send CMD_START with 4-byte big-endian total length
     * ---------------------------------------------------------------- */
    {
        uint32_t total_len = (uint32_t)g_iaq_model_data_len;
        uint8_t  payload[4];
        uint8_t  response;
        uint8_t  retries = 0U;

        payload[0] = (uint8_t)((total_len >> 24U) & 0xFFU);
        payload[1] = (uint8_t)((total_len >> 16U) & 0xFFU);
        payload[2] = (uint8_t)((total_len >>  8U) & 0xFFU);
        payload[3] = (uint8_t)(total_len & 0xFFU);

        Serial.print("[SENDER] CMD_START: len=");
        Serial.println(total_len);

        do {
            send_frame(CMD_START, payload, 4U);
            response = wait_for_response(ACK_TIMEOUT_MS);
            retries++;

            if (response == CMD_ACK) {
                Serial.println("[SENDER] CMD_START → ACK");
                break;
            } else if (response == CMD_NACK) {
                Serial.println("[SENDER] CMD_START → NACK, retrying...");
            } else {
                Serial.println("[SENDER] CMD_START → TIMEOUT, retrying...");
            }
        } while (retries < MAX_RETRIES);

        if (response != CMD_ACK) {
            Serial.println("[ERROR] CMD_START failed after max retries. Aborting.");
            g_transfer_status = TRANSFER_FAILED;
            return;
        }

        delay(INTER_FRAME_DELAY);
    }

    /* ----------------------------------------------------------------
     * Compute full-image CRC before sending any data
     * (saves recomputing at CMD_END)
     * ---------------------------------------------------------------- */
    uint16_t image_crc = compute_image_crc();
    Serial.print("[SENDER] Full-image CRC-16-CCITT = 0x");
    Serial.println(image_crc, HEX);

    /* ----------------------------------------------------------------
     * Step 2: Send CMD_DATA frames in 128-byte blocks
     * ---------------------------------------------------------------- */
    {
        uint32_t bytes_sent = 0UL;
        uint32_t total_len  = (uint32_t)g_iaq_model_data_len;
        uint32_t block_num  = 0UL;

        while (bytes_sent < total_len) {
            uint32_t chunk;
            uint8_t  response;
            uint8_t  retries = 0U;

            /* Determine block size (last block may be smaller) */
            chunk = total_len - bytes_sent;
            if (chunk > DATA_BLOCK_SIZE) {
                chunk = DATA_BLOCK_SIZE;
            }

            do {
                send_frame(CMD_DATA,
                           &g_iaq_model_data[bytes_sent],
                           (uint16_t)chunk);
                response = wait_for_response(ACK_TIMEOUT_MS);
                retries++;

                if (response == CMD_ACK) {
                    break;
                } else if (response == CMD_NACK) {
                    Serial.print("[SENDER] Block ");
                    Serial.print(block_num);
                    Serial.println(" → NACK, retrying...");
                } else {
                    Serial.print("[SENDER] Block ");
                    Serial.print(block_num);
                    Serial.println(" → TIMEOUT, retrying...");
                }
            } while (retries < MAX_RETRIES);

            if (response != CMD_ACK) {
                Serial.print("[ERROR] Block ");
                Serial.print(block_num);
                Serial.println(" failed. Aborting.");
                g_transfer_status = TRANSFER_FAILED;
                return;
            }

            bytes_sent += chunk;
            block_num++;

            /* Progress report every 8 blocks */
            if ((block_num % 8UL) == 0UL) {
                Serial.print("[SENDER] Progress: ");
                Serial.print(bytes_sent);
                Serial.print(" / ");
                Serial.print(total_len);
                Serial.println(" bytes");
            }

            delay(INTER_FRAME_DELAY);
        }

        Serial.print("[SENDER] All ");
        Serial.print(block_num);
        Serial.println(" data blocks sent.");
    }

    /* ----------------------------------------------------------------
     * Step 3: Send CMD_END with 2-byte big-endian full-image CRC
     * ---------------------------------------------------------------- */
    {
        uint8_t  payload[2];
        uint8_t  response;
        uint8_t  retries = 0U;

        payload[0] = (uint8_t)((image_crc >> 8U) & 0xFFU);
        payload[1] = (uint8_t)(image_crc & 0xFFU);

        Serial.println("[SENDER] CMD_END: sending full-image CRC");

        do {
            send_frame(CMD_END, payload, 2U);
            response = wait_for_response(ACK_TIMEOUT_MS * 2U);  /* longer timeout: flash verify */
            retries++;

            if (response == CMD_ACK) {
                Serial.println("[SENDER] CMD_END → ACK");
                break;
            } else if (response == CMD_NACK) {
                Serial.println("[SENDER] CMD_END → NACK (verify failed on RA6M5)");
            } else {
                Serial.println("[SENDER] CMD_END → TIMEOUT");
            }
        } while (retries < MAX_RETRIES);

        if (response != CMD_ACK) {
            Serial.println("[ERROR] CMD_END failed. Transfer failed.");
            g_transfer_status = TRANSFER_FAILED;
            return;
        }
    }

    /* ----------------------------------------------------------------
     * Transfer complete
     * ---------------------------------------------------------------- */
    Serial.println("===========================================");
    Serial.println("[SENDER] IAQ model transfer SUCCESSFUL.");
    Serial.print("[SENDER] Model is now at RA6M5 Data Flash 0x08000000, ");
    Serial.print(g_iaq_model_data_len);
    Serial.println(" bytes.");
    Serial.println("===========================================");

    g_transfer_status = TRANSFER_DONE;
}
