#ifndef DRV_FLASH_HP_H
#define DRV_FLASH_HP_H

/*
 * drv_flash_hp.h — Flash HP Peripheral Driver for RA6M5 (Data Flash).
 *
 * Target region : Data Flash — 0x08000000 .. 0x08001FFF (8 KB)
 * Erase unit    : 64 bytes per block  (RA6M5 Data Flash, §44)
 * Write unit    : 4-byte aligned minimum, up to 256 bytes per operation
 *
 * MISRA-C:2012 compliance notes:
 *   - All integer types from <stdint.h>
 *   - No complex pointer arithmetic (only base + constant offset)
 *   - All pointer parameters validated against NULL before use (Rule 14.5 / D4.1)
 *   - Return status checked at every call site in the implementation
 *   - No dynamic memory allocation
 *
 * Hardware manual references: RA6M5 Group User Manual §44 (Flash).
 */

#include <stdint.h>
#include "drv_common.h"

/* -----------------------------------------------------------------------
 * Flash HP peripheral base address (RA6M5 HW manual §44.3.1)
 * ----------------------------------------------------------------------- */
#define FLASH_HP_BASE   0x407EC000UL

/* -----------------------------------------------------------------------
 * Flash HP register offsets (all 32-bit unless noted)
 * ----------------------------------------------------------------------- */
#define FLASH_HP_FPMCR_OFF   0x000U  /* Flash P/E Mode Control Register          */
#define FLASH_HP_FSTATR_OFF  0x010U  /* Flash Status Register                    */
#define FLASH_HP_FENTRYR_OFF 0x084U  /* Flash P/E Entry Register  (16-bit)       */
#define FLASH_HP_FSUINITR_OFF 0x08CU /* Flash Sequencer Set-up Initialization    */
#define FLASH_HP_FCMDR_OFF   0x0A0U  /* Flash Command Register                   */
#define FLASH_HP_FSADDR_OFF  0x030U  /* Flash Start Address Register             */
#define FLASH_HP_FEADDR_OFF  0x034U  /* Flash End   Address Register             */
#define FLASH_HP_FCPSR_OFF   0x0E4U  /* Flash Sequencer Processing Switching     */
#define FLASH_HP_FRESETR_OFF 0x008U  /* Flash Reset Register                     */
#define FLASH_HP_FWBL0_OFF   0x280U  /* Flash Write Buffer L-word 0              */
#define FLASH_HP_FWBH0_OFF   0x284U  /* Flash Write Buffer H-word 0              */

/* -----------------------------------------------------------------------
 * Register accessors
 * ----------------------------------------------------------------------- */
#define FLASH_HP_REG32(off) \
    (*(volatile uint32_t *)(uintptr_t)(FLASH_HP_BASE + (uint32_t)(off)))
#define FLASH_HP_REG16(off) \
    (*(volatile uint16_t *)(uintptr_t)(FLASH_HP_BASE + (uint32_t)(off)))
#define FLASH_HP_REG8(off) \
    (*(volatile uint8_t  *)(uintptr_t)(FLASH_HP_BASE + (uint32_t)(off)))

#define FPMCR    FLASH_HP_REG8 (FLASH_HP_FPMCR_OFF)
#define FSTATR   FLASH_HP_REG32(FLASH_HP_FSTATR_OFF)
#define FENTRYR  FLASH_HP_REG16(FLASH_HP_FENTRYR_OFF)
#define FSUINITR FLASH_HP_REG16(FLASH_HP_FSUINITR_OFF)
#define FCMDR    FLASH_HP_REG8 (FLASH_HP_FCMDR_OFF)
#define FSADDR   FLASH_HP_REG32(FLASH_HP_FSADDR_OFF)
#define FEADDR   FLASH_HP_REG32(FLASH_HP_FEADDR_OFF)
#define FCPSR    FLASH_HP_REG16(FLASH_HP_FCPSR_OFF)
#define FRESETR  FLASH_HP_REG16(FLASH_HP_FRESETR_OFF)
#define FWBL0    FLASH_HP_REG32(FLASH_HP_FWBL0_OFF)
#define FWBH0    FLASH_HP_REG32(FLASH_HP_FWBH0_OFF)

/* -----------------------------------------------------------------------
 * FPMCR bits
 * ----------------------------------------------------------------------- */
#define FPMCR_DFLR   (1U << 6)   /* Data Flash Read mode          */
#define FPMCR_FMS1   (1U << 4)   /* Flash Mode Select bit 1       */
#define FPMCR_FMS0   (1U << 1)   /* Flash Mode Select bit 0       */
#define FPMCR_MS     (1U << 3)   /* Mode Select for Data Flash PE */

/* FPMCR sequences for entering Data Flash P/E mode (§44.3.6):
 *   Step 1: write 0x10 (set FMS1)
 *   Step 2: write 0x12 (set FMS1 | FMS0)
 *   Step 3: write 0x13 (set FMS1 | FMS0 | DFLR) — not needed per Renesas app note */

/* -----------------------------------------------------------------------
 * FSTATR bits (§44.3.13)
 * ----------------------------------------------------------------------- */
#define FSTATR_FRDY    (1UL << 15)  /* Flash Ready: 1 = idle, can accept command */
#define FSTATR_ILGLERR (1UL << 4)   /* Illegal Command Error                     */
#define FSTATR_PRGERR  (1UL << 8)   /* Programming Error                         */
#define FSTATR_ERSERR  (1UL << 9)   /* Erase Error                               */
#define FSTATR_OTERR   (1UL << 5)   /* Over-cycle error                          */

#define FSTATR_ERROR_MASK  (FSTATR_ILGLERR | FSTATR_PRGERR | FSTATR_ERSERR | FSTATR_OTERR)

/* -----------------------------------------------------------------------
 * FENTRYR — Flash P/E Entry keys (§44.3.14)
 * Upper byte = key. Lower byte = enable bits.
 * 0xAA80 = enter data flash P/E mode
 * 0xAA00 = exit P/E mode (return to read mode)
 * ----------------------------------------------------------------------- */
#define FENTRYR_KEY_DATA_PE   0xAA80U
#define FENTRYR_KEY_READ      0xAA00U

/* -----------------------------------------------------------------------
 * Flash commands (§44.4)
 * ----------------------------------------------------------------------- */
#define FLASH_CMD_PROGRAM     0xE8U   /* Program data (followed by word count)    */
#define FLASH_CMD_PROGRAM_CFM 0xD0U   /* Program confirm                          */
#define FLASH_CMD_ERASE       0x20U   /* Block erase                              */
#define FLASH_CMD_ERASE_CFM   0xD0U   /* Erase confirm                            */
#define FLASH_CMD_FORCED_STOP 0xB3U   /* Forced stop                              */
#define FLASH_CMD_STATUS_CLR  0x50U   /* Status clear                             */

/* -----------------------------------------------------------------------
 * Data Flash geometry (RA6M5 §44.2)
 * ----------------------------------------------------------------------- */
#define DATA_FLASH_BASE            0x08000000UL
#define DATA_FLASH_BLOCK_SIZE      64UL     /* bytes per erasable block            */
#define DATA_FLASH_SIZE            0x2000UL /* 8 KB total                         */
#define DATA_FLASH_END             (DATA_FLASH_BASE + DATA_FLASH_SIZE)
#define DATA_FLASH_BLOCK_COUNT     (DATA_FLASH_SIZE / DATA_FLASH_BLOCK_SIZE)
#define DATA_FLASH_LAST_BLOCK_ADDR (DATA_FLASH_END - DATA_FLASH_BLOCK_SIZE)

/* Write minimum word = 4 bytes. Buffer register = 8 bytes (two 32-bit regs). */
#define DATA_FLASH_WRITE_WORD_SIZE  4UL   /* bytes per write word                 */
#define DATA_FLASH_WRITE_PAGE_SIZE  256UL /* max bytes per program command        */

/* -----------------------------------------------------------------------
 * Flash HP driver status codes (extend drv_status_t with flash specifics)
 * ----------------------------------------------------------------------- */
typedef enum {
    FLASH_HP_OK          = 0,   /* Operation completed successfully              */
    FLASH_HP_ERR_PARAM   = 1,   /* Invalid parameter (NULL, misaligned, OOB)     */
    FLASH_HP_ERR_TIMEOUT = 2,   /* FSTATR.FRDY never asserted within timeout     */
    FLASH_HP_ERR_HW      = 3,   /* Hardware error bit set in FSTATR              */
    FLASH_HP_ERR_VERIFY  = 4    /* Read-back mismatch after write                */
} flash_hp_status_t;

/* -----------------------------------------------------------------------
 * Public API
 * ----------------------------------------------------------------------- */

/*
 * flash_hp_init — Enter Data Flash P/E mode.
 *
 * Must be called once before any erase/write operation.
 * Transitions FPMCR to Data Flash P/E mode and sets FENTRYR.
 *
 * Returns: FLASH_HP_OK on success, FLASH_HP_ERR_TIMEOUT if FRDY never set.
 */
flash_hp_status_t flash_hp_init(void);

/*
 * flash_hp_exit — Exit P/E mode and return to read mode.
 *
 * Must be called after all erase/write operations are complete,
 * before reading back Data Flash contents from application code.
 */
void flash_hp_exit(void);

/*
 * flash_hp_erase — Erase one or more 64-byte Data Flash blocks.
 *
 * Parameters:
 *   addr       : Start address (must be DATA_FLASH_BASE-aligned to 64-byte boundary).
 *   num_blocks : Number of 64-byte blocks to erase (≥1).
 *
 * MISRA: addr and num_blocks validated before use.
 * Returns: FLASH_HP_OK, FLASH_HP_ERR_PARAM, FLASH_HP_ERR_TIMEOUT, or FLASH_HP_ERR_HW.
 */
flash_hp_status_t flash_hp_erase(uint32_t addr, uint32_t num_blocks);

/*
 * flash_hp_write — Write a buffer to Data Flash.
 *
 * Parameters:
 *   addr  : Destination start address (must be 4-byte aligned, within Data Flash).
 *   p_src : Pointer to source data (must not be NULL).
 *   len   : Number of bytes to write (must be multiple of 4, ≤ data flash size).
 *
 * Handles splitting into ≤256-byte page program commands internally.
 * MISRA: p_src validated; addr checked for alignment and bounds.
 * Returns: FLASH_HP_OK, FLASH_HP_ERR_PARAM, FLASH_HP_ERR_TIMEOUT, or FLASH_HP_ERR_HW.
 */
flash_hp_status_t flash_hp_write(uint32_t addr, uint8_t const * const p_src, uint32_t len);

/*
 * flash_hp_verify — Read-back verify: compare flash contents to source buffer.
 *
 * Parameters:
 *   addr  : Flash start address to verify.
 *   p_ref : Reference data buffer (must not be NULL).
 *   len   : Number of bytes to compare.
 *
 * Note: Call flash_hp_exit() before calling this — Data Flash is read-mapped
 * at 0x08000000 only in read mode.
 * Returns: FLASH_HP_OK on match, FLASH_HP_ERR_PARAM or FLASH_HP_ERR_VERIFY on mismatch.
 */
flash_hp_status_t flash_hp_verify(uint32_t addr, uint8_t const * const p_ref, uint32_t len);

#endif /* DRV_FLASH_HP_H */
