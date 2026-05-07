/*
 * fwupdate_receiver.c — UART-ISR Frame Receiver & Data Flash Writer for RA6M5.
 *
 * Architecture:
 *   ISR  → fills ring_buf[], advances ring_head
 *   main → drains ring_buf[], advances ring_tail, drives state machine
 *
 * The design keeps the ISR minimal (one byte write + head advance) to avoid
 * violating MISRA-C:2012 Rule 15.5 in interrupt context.
 *
 * UART channel: SCI2 (UART2), P301=RX, P302=TX, 115200 baud.
 * The RXI interrupt vector for SCI2 is assigned in the NVIC; its handler
 * name follows the RA6M5 vector table convention: sci2_rxi_isr.
 */

#include "fwupdate_receiver.h"
#include "drv_uart.h"
#include "drv_clk.h"
#include "drv_flash_hp.h"

#include <stdint.h>
#include <stddef.h>

/* -----------------------------------------------------------------------
 * NVIC registers for SCI2_RXI interrupt (EK-RA6M5 default vector 13)
 * RA6M5 uses the Cortex-M33 NVIC at 0xE000E000.
 * ----------------------------------------------------------------------- */
#define NVIC_BASE          0xE000E000UL
#define NVIC_ISER0         (*(volatile uint32_t *)(uintptr_t)(NVIC_BASE + 0x100U))
#define NVIC_IPR_BASE      (volatile uint8_t  *)(uintptr_t)(NVIC_BASE + 0x400U)

/* SCI2_RXI interrupt number on EK-RA6M5 (from vector table, event 228 = IRQ44) */
#define SCI2_RXI_IRQ_NUM   44U

/* SCR bit for RIE (Receive Interrupt Enable) */
#define SCR_RIE            (1U << 6)

/* -----------------------------------------------------------------------
 * Internal UART2 send helpers (blocking) — used only for ACK/NACK frames.
 * This avoids pulling in the entire drv_uart API for the TX path.
 * ----------------------------------------------------------------------- */
#define UART2_CH   2U

static void uart2_send_byte(uint8_t b)
{
    UART_SendChar((UART_t)UART2_CH, (char)b);
}

/* -----------------------------------------------------------------------
 * Static context — single instance (one active transfer at a time).
 * ----------------------------------------------------------------------- */
static fwupdate_ctx_t s_ctx;

/* -----------------------------------------------------------------------
 * crc16_ccitt — CRC-16-CCITT (XMODEM variant).
 *
 * Polynomial : 0x1021  (x^16 + x^12 + x^5 + 1)
 * Init value : 0xFFFF
 * No reflection of input or output bits.
 *
 * Algorithm: bit-by-bit with MSB-first shift, no lookup table.
 * Both sides (ESP32-C6 and RA6M5) MUST use identical constants.
 * ----------------------------------------------------------------------- */
uint16_t crc16_ccitt(uint8_t const * const p_data, uint32_t len)
{
    uint16_t crc = 0xFFFFU;
    uint32_t idx;
    uint8_t  bit;

    if (p_data == NULL)
    {
        return 0U;
    }

    for (idx = 0U; idx < len; idx++)
    {
        /* XOR next byte into MSB of CRC */
        crc ^= (uint16_t)((uint16_t)p_data[idx] << 8U);

        for (bit = 0U; bit < 8U; bit++)
        {
            if ((crc & 0x8000U) != 0U)
            {
                crc = (uint16_t)((uint16_t)(crc << 1U) ^ 0x1021U);
            }
            else
            {
                crc = (uint16_t)(crc << 1U);
            }
        }
    }

    return crc;
}

/* -----------------------------------------------------------------------
 * Internal: send an ACK frame.
 *   [STX][CMD_ACK][0x00][0x01][echo_cmd][CRC_MSB][CRC_LSB][ETX]
 * ----------------------------------------------------------------------- */
static void send_ack(uint8_t echo_cmd)
{
    uint8_t  crc_buf[4];
    uint16_t crc;

    crc_buf[0] = FWUPDATE_CMD_ACK;
    crc_buf[1] = 0x00U;
    crc_buf[2] = 0x01U;
    crc_buf[3] = echo_cmd;
    crc = crc16_ccitt(crc_buf, 4U);

    uart2_send_byte(FWUPDATE_STX);
    uart2_send_byte(FWUPDATE_CMD_ACK);
    uart2_send_byte(0x00U);                          /* LEN_MSB  */
    uart2_send_byte(0x01U);                          /* LEN_LSB  */
    uart2_send_byte(echo_cmd);                       /* DATA     */
    uart2_send_byte((uint8_t)((crc >> 8U) & 0xFFU));
    uart2_send_byte((uint8_t)(crc & 0xFFU));
    uart2_send_byte(FWUPDATE_ETX);
}

/* -----------------------------------------------------------------------
 * Internal: send a NACK frame.
 *   [STX][CMD_NACK][0x00][0x01][reason][CRC_MSB][CRC_LSB][ETX]
 * ----------------------------------------------------------------------- */
static void send_nack(uint8_t reason)
{
    uint8_t  crc_buf[4];
    uint16_t crc;

    crc_buf[0] = FWUPDATE_CMD_NACK;
    crc_buf[1] = 0x00U;
    crc_buf[2] = 0x01U;
    crc_buf[3] = reason;
    crc = crc16_ccitt(crc_buf, 4U);

    uart2_send_byte(FWUPDATE_STX);
    uart2_send_byte(FWUPDATE_CMD_NACK);
    uart2_send_byte(0x00U);
    uart2_send_byte(0x01U);
    uart2_send_byte(reason);
    uart2_send_byte((uint8_t)((crc >> 8U) & 0xFFU));
    uart2_send_byte((uint8_t)(crc & 0xFFU));
    uart2_send_byte(FWUPDATE_ETX);
}

/* -----------------------------------------------------------------------
 * Internal: validate received frame CRC.
 *
 * CRC is computed over: [CMD][LEN_MSB][LEN_LSB][DATA...]
 * Returns 1 if CRC matches, 0 otherwise.
 * ----------------------------------------------------------------------- */
static uint8_t frame_crc_valid(void)
{
    /* Build crc_input = [CMD, LEN_MSB, LEN_LSB, DATA...] */
    uint8_t  crc_buf[3U + FWUPDATE_MAX_DATA_LEN + 4U];
    uint16_t crc_len;
    uint16_t computed_crc;

    crc_buf[0] = s_ctx.frame_cmd;
    crc_buf[1] = (uint8_t)((s_ctx.frame_len >> 8U) & 0xFFU);
    crc_buf[2] = (uint8_t)(s_ctx.frame_len & 0xFFU);

    /* Copy data bytes into local buffer (MISRA: no pointer arithmetic on frame_data) */
    {
        uint16_t i;
        for (i = 0U; i < s_ctx.frame_len; i++)
        {
            crc_buf[3U + i] = s_ctx.frame_data[i];
        }
    }

    crc_len = (uint16_t)(3U + s_ctx.frame_len);
    computed_crc = crc16_ccitt(crc_buf, (uint32_t)crc_len);

    return (computed_crc == s_ctx.frame_crc_received) ? 1U : 0U;
}

/* -----------------------------------------------------------------------
 * Internal: process a complete, CRC-verified frame.
 * Returns FLASH_HP_OK on success.
 * ----------------------------------------------------------------------- */
static flash_hp_status_t process_frame(void)
{
    flash_hp_status_t status = FLASH_HP_OK;

    switch (s_ctx.frame_cmd)
    {
        /* ----------------------------------------------------------------
         * CMD_START — begin a new transfer.
         * Payload: 4-byte big-endian total model length.
         * ---------------------------------------------------------------- */
        case FWUPDATE_CMD_START:
        {
            uint32_t total_len;
            uint32_t num_blocks;

            if (s_ctx.frame_len != 4U)
            {
                send_nack(FWUPDATE_NACK_SEQUENCE);
                break;
            }

            /* Reconstruct total length from big-endian payload */
            total_len  = (uint32_t)s_ctx.frame_data[0] << 24U;
            total_len |= (uint32_t)s_ctx.frame_data[1] << 16U;
            total_len |= (uint32_t)s_ctx.frame_data[2] << 8U;
            total_len |= (uint32_t)s_ctx.frame_data[3];

            if (total_len == 0UL || total_len > FWUPDATE_MAX_MODEL_LEN)
            {
                send_nack(FWUPDATE_NACK_LENGTH);
                break;
            }

            s_ctx.model_total_len    = total_len;
            s_ctx.model_written_len  = 0UL;
            s_ctx.image_crc_running  = 0xFFFFU; /* init for CRC-CCITT */
            s_ctx.flash_write_addr   = FWUPDATE_FLASH_BASE;

            /* Initialise Flash HP P/E mode */
            status = flash_hp_init();
            if (status != FLASH_HP_OK)
            {
                send_nack(FWUPDATE_NACK_FLASH);
                break;
            }

            /* Erase required blocks — ceil(total_len / 64) */
            num_blocks = (total_len + DATA_FLASH_BLOCK_SIZE - 1UL) / DATA_FLASH_BLOCK_SIZE;
            status = flash_hp_erase(FWUPDATE_FLASH_BASE, num_blocks);
            if (status != FLASH_HP_OK)
            {
                flash_hp_exit();
                send_nack(FWUPDATE_NACK_FLASH);
                break;
            }

            s_ctx.state = FWUPDATE_STATE_IDLE;   /* ready for CMD_DATA frames */
            send_ack(FWUPDATE_CMD_START);
            break;
        }

        /* ----------------------------------------------------------------
         * CMD_DATA — write a data chunk to Data Flash.
         * Payload: 1..128 bytes of model data.
         * ---------------------------------------------------------------- */
        case FWUPDATE_CMD_DATA:
        {
            uint32_t bytes_remaining;
            uint32_t chunk_len;
            uint32_t padded_len;

            if (s_ctx.model_total_len == 0UL)
            {
                /* CMD_START not received yet */
                send_nack(FWUPDATE_NACK_SEQUENCE);
                break;
            }
            if (s_ctx.frame_len == 0U || s_ctx.frame_len > FWUPDATE_MAX_DATA_LEN)
            {
                send_nack(FWUPDATE_NACK_LENGTH);
                break;
            }

            if (s_ctx.model_written_len >= s_ctx.model_total_len)
            {
                send_nack(FWUPDATE_NACK_SEQUENCE);
                break;
            }

            chunk_len = (uint32_t)s_ctx.frame_len;
            bytes_remaining = s_ctx.model_total_len - s_ctx.model_written_len;

            if (chunk_len > bytes_remaining)
            {
                send_nack(FWUPDATE_NACK_LENGTH);
                break;
            }

            /* Update running image CRC over raw payload bytes */
            {
                uint16_t crc = s_ctx.image_crc_running;
                uint32_t i;
                uint8_t  bit;

                for (i = 0UL; i < chunk_len; i++)
                {
                    crc ^= (uint16_t)((uint16_t)s_ctx.frame_data[i] << 8U);
                    for (bit = 0U; bit < 8U; bit++)
                    {
                        if ((crc & 0x8000U) != 0U)
                        {
                            crc = (uint16_t)((uint16_t)(crc << 1U) ^ 0x1021U);
                        }
                        else
                        {
                            crc = (uint16_t)(crc << 1U);
                        }
                    }
                }
                s_ctx.image_crc_running = crc;
            }

            /* Data Flash write unit is 4 bytes on RA6M5 — pad tail bytes with 0xFF. */
            padded_len = chunk_len;
            if ((padded_len & (FWUPDATE_FLASH_WRITE_ALIGN - 1UL)) != 0UL)
            {
                uint32_t pad = FWUPDATE_FLASH_WRITE_ALIGN -
                               (padded_len & (FWUPDATE_FLASH_WRITE_ALIGN - 1UL));
                uint32_t p;
                for (p = 0UL; p < pad; p++)
                {
                    /* frame_data buffer is sized for this padding */
                    s_ctx.frame_data[padded_len + p] = 0xFFU;
                }
                padded_len += pad;
            }

            if ((s_ctx.flash_write_addr >= FWUPDATE_FLASH_END) ||
                (padded_len > (FWUPDATE_FLASH_END - s_ctx.flash_write_addr)))
            {
                send_nack(FWUPDATE_NACK_FLASH);
                break;
            }

            status = flash_hp_write(s_ctx.flash_write_addr,
                                    (uint8_t const *)s_ctx.frame_data,
                                    padded_len);
            if (status != FLASH_HP_OK)
            {
                flash_hp_exit();
                send_nack(FWUPDATE_NACK_FLASH);
                break;
            }

            s_ctx.model_written_len += chunk_len;
            s_ctx.flash_write_addr  += padded_len;

            send_ack(FWUPDATE_CMD_DATA);
            break;
        }

        /* ----------------------------------------------------------------
         * CMD_END — verify the complete image.
         * Payload: 2-byte big-endian full-image CRC.
         * ---------------------------------------------------------------- */
        case FWUPDATE_CMD_END:
        {
            uint16_t received_image_crc;

            if (s_ctx.frame_len != 2U)
            {
                send_nack(FWUPDATE_NACK_SEQUENCE);
                break;
            }

            received_image_crc  = (uint16_t)((uint16_t)s_ctx.frame_data[0] << 8U);
            received_image_crc |= (uint16_t)s_ctx.frame_data[1];

            if (s_ctx.model_written_len != s_ctx.model_total_len)
            {
                flash_hp_exit();
                send_nack(FWUPDATE_NACK_SEQUENCE);
                break;
            }

            /* Compare running image CRC */
            if (s_ctx.image_crc_running != received_image_crc)
            {
                flash_hp_exit();
                send_nack(FWUPDATE_NACK_IMAGE_CRC);
                break;
            }

            /* Exit P/E mode before read-back verify */
            flash_hp_exit();

            /* Read-back verify: compare Data Flash to the expected CRC.
             * Re-compute CRC directly from flash read-mapped region. */
            {
                /* Build a local CRC over the flash-mapped region */
                volatile uint8_t const * const p_flash =
                    (volatile uint8_t const *)(uintptr_t)FWUPDATE_FLASH_BASE;
                uint16_t verify_crc = 0xFFFFU;
                uint32_t i;
                uint8_t  bit;

                for (i = 0UL; i < s_ctx.model_written_len; i++)
                {
                    verify_crc ^= (uint16_t)((uint16_t)p_flash[i] << 8U);
                    for (bit = 0U; bit < 8U; bit++)
                    {
                        if ((verify_crc & 0x8000U) != 0U)
                        {
                            verify_crc = (uint16_t)((uint16_t)(verify_crc << 1U) ^ 0x1021U);
                        }
                        else
                        {
                            verify_crc = (uint16_t)(verify_crc << 1U);
                        }
                    }
                }

                if (verify_crc != received_image_crc)
                {
                    send_nack(FWUPDATE_NACK_VERIFY);
                    break;
                }
            }

            /* All checks passed */
            s_ctx.state = FWUPDATE_STATE_DONE;
            send_ack(FWUPDATE_CMD_END);
            break;
        }

        default:
        {
            send_nack(FWUPDATE_NACK_SEQUENCE);
            break;
        }
    }

    return status;
}

/* -----------------------------------------------------------------------
 * SCI2_RXI ISR — fires on each received byte from UART2.
 *
 * Reads RDR2 and stores in ring buffer.
 * ring_head is volatile; main loop reads ring_tail (non-volatile).
 * No critical section required: single byte write is atomic on CM33.
 * ----------------------------------------------------------------------- */
void sci2_rxi_isr(void);   /* forward declaration — matches vector table symbol */
void sci2_rxi_isr(void)
{
    uint8_t  byte_in;
    uint16_t next_head;

    /* Read received byte from SCI2 RDR — clears RDRF flag */
    byte_in = SCI_RDR(UART2_CH);

    next_head = (uint16_t)((s_ctx.ring_head + 1U) % FWUPDATE_RING_BUF_SIZE);

    /* Store only if buffer is not full (avoid head overtaking tail) */
    if (next_head != s_ctx.ring_tail)
    {
        s_ctx.ring_buf[s_ctx.ring_head] = byte_in;
        s_ctx.ring_head                 = next_head;
    }
    /* If buffer full, byte is silently dropped; sender will get NACK on timeout */
}

/* -----------------------------------------------------------------------
 * fwupdate_receiver_init
 * ----------------------------------------------------------------------- */
void fwupdate_receiver_init(void)
{
    uint32_t i;

    /* Zero-initialise context */
    s_ctx.ring_head           = 0U;
    s_ctx.ring_tail           = 0U;
    s_ctx.state               = FWUPDATE_STATE_IDLE;
    s_ctx.frame_cmd           = 0U;
    s_ctx.frame_len           = 0U;
    s_ctx.frame_data_idx      = 0U;
    s_ctx.frame_crc_received  = 0U;
    s_ctx.model_total_len     = 0UL;
    s_ctx.model_written_len   = 0UL;
    s_ctx.image_crc_running   = 0xFFFFU;
    s_ctx.flash_write_addr    = FWUPDATE_FLASH_BASE;

    for (i = 0U; i < FWUPDATE_RING_BUF_SIZE; i++)
    {
        s_ctx.ring_buf[i] = 0U;
    }
    for (i = 0U; i < (FWUPDATE_MAX_DATA_LEN + 4U); i++)
    {
        s_ctx.frame_data[i] = 0U;
    }

    /* Initialise UART2 at 115200 baud (TX+RX, polling TX) */
    UART_Init(UART2, 115200UL);

    /* Enable SCI2 Receive Interrupt (RIE bit in SCR) */
    SCI_SCR(UART2_CH) |= (uint8_t)SCR_RIE;

    /* Enable SCI2_RXI in NVIC (IRQ 44 → ISER1 bit 12) */
    {
        /* NVIC ISER1 is at 0xE000E104 (bits 32..63 of interrupt set-enable) */
        volatile uint32_t * const p_iser1 =
            (volatile uint32_t *)(uintptr_t)(NVIC_BASE + 0x104U);
        /* IRQ44 → bit (44 - 32) = bit 12 in ISER1 */
        *p_iser1 = (1UL << (SCI2_RXI_IRQ_NUM - 32UL));
    }

    /* Set interrupt priority 4 (0 = highest on CM33 with 4-bit priority) */
    {
        volatile uint8_t * const p_ipr =
            (volatile uint8_t *)(uintptr_t)(NVIC_BASE + 0x400UL + SCI2_RXI_IRQ_NUM);
        *p_ipr = (uint8_t)(4U << 4U);   /* Priority level 4, shifted to bits [7:4] */
    }
}

/* -----------------------------------------------------------------------
 * fwupdate_receiver_run — call from main loop.
 * ----------------------------------------------------------------------- */
fwupdate_state_t fwupdate_receiver_run(void)
{
    /* Drain the ring buffer one byte at a time */
    while (s_ctx.ring_tail != s_ctx.ring_head)
    {
        uint8_t b = s_ctx.ring_buf[s_ctx.ring_tail];
        s_ctx.ring_tail = (uint16_t)((s_ctx.ring_tail + 1U) % FWUPDATE_RING_BUF_SIZE);

        /* Feed byte into frame state machine */
        switch (s_ctx.state)
        {
            case FWUPDATE_STATE_IDLE:
                if (b == FWUPDATE_STX)
                {
                    /* Reset frame context for new frame */
                    s_ctx.frame_cmd      = 0U;
                    s_ctx.frame_len      = 0U;
                    s_ctx.frame_data_idx = 0U;
                    s_ctx.state          = FWUPDATE_STATE_CMD;
                }
                break;

            case FWUPDATE_STATE_CMD:
                s_ctx.frame_cmd = b;
                s_ctx.state     = FWUPDATE_STATE_LEN_MSB;
                break;

            case FWUPDATE_STATE_LEN_MSB:
                s_ctx.frame_len = (uint16_t)((uint16_t)b << 8U);
                s_ctx.state     = FWUPDATE_STATE_LEN_LSB;
                break;

            case FWUPDATE_STATE_LEN_LSB:
                s_ctx.frame_len |= (uint16_t)b;
                s_ctx.frame_data_idx = 0U;

                if (s_ctx.frame_len == 0U)
                {
                    /* No data — go straight to CRC */
                    s_ctx.state = FWUPDATE_STATE_CRC_MSB;
                }
                else if (s_ctx.frame_len > (FWUPDATE_MAX_DATA_LEN + 4U))
                {
                    /* Oversized — abort and wait for next STX */
                    s_ctx.state = FWUPDATE_STATE_IDLE;
                }
                else
                {
                    s_ctx.state = FWUPDATE_STATE_DATA;
                }
                break;

            case FWUPDATE_STATE_DATA:
                s_ctx.frame_data[s_ctx.frame_data_idx] = b;
                s_ctx.frame_data_idx++;

                if (s_ctx.frame_data_idx >= s_ctx.frame_len)
                {
                    s_ctx.state = FWUPDATE_STATE_CRC_MSB;
                }
                break;

            case FWUPDATE_STATE_CRC_MSB:
                s_ctx.frame_crc_received = (uint16_t)((uint16_t)b << 8U);
                s_ctx.state = FWUPDATE_STATE_CRC_LSB;
                break;

            case FWUPDATE_STATE_CRC_LSB:
                s_ctx.frame_crc_received |= (uint16_t)b;
                s_ctx.state = FWUPDATE_STATE_ETX;
                break;

            case FWUPDATE_STATE_ETX:
                if (b == FWUPDATE_ETX)
                {
                    /* Validate CRC */
                    if (frame_crc_valid() != 0U)
                    {
                        /* Process the valid frame */
                        (void)process_frame();
                    }
                    else
                    {
                        send_nack(FWUPDATE_NACK_CRC);
                    }
                }
                /* Return to idle regardless — wait for next STX */
                if (s_ctx.state != FWUPDATE_STATE_DONE)
                {
                    s_ctx.state = FWUPDATE_STATE_IDLE;
                }
                break;

            case FWUPDATE_STATE_PROCESS:
            case FWUPDATE_STATE_DONE:
            default:
                /* No action — absorb bytes */
                break;
        }
    }

    return s_ctx.state;
}
