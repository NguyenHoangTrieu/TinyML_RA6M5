#include "drv_clk.h"
#include "drv_rwp.h"

/* -----------------------------------------------------------------------
 * CLK_Init — bring the current project to its configured 200 MHz state.
 *
 * Source of truth:
 *   configuration.xml
 *   - XTAL   = 24 MHz crystal
 *   - PLL    = XTAL / 3 * 25.0 = 200 MHz
 *   - ICLK   = 200 MHz
 *   - PCLKA  = 100 MHz
 *   - PCLKB  =  50 MHz
 *   - PCLKC  =  50 MHz
 *   - PCLKD  = 100 MHz
 *   - BCLK   = 100 MHz
 *   - FCLK   =  50 MHz
 *
 * The RA6M5 requires flash and SRAM wait states before raising ICLK.
 * Those settings are mirrored from the FSP-generated reference project.
 * ----------------------------------------------------------------------- */
void CLK_Init(void)
{
    RWP_Unlock_Clock_MSTP();

    /* 200 MHz on RA6M5 requires flash wait states before the switch. */
    FLWT = FLWT_3_WAIT;

    /* 200 MHz also requires SRAM wait states.  Unlock, update, relock. */
    __asm volatile ("dmb" ::: "memory");
    SRAMPRCR = SRAMPRCR_KEY_UNLOCK;
    __asm volatile ("dmb" ::: "memory");
    SRAMWTSC = SRAMWTSC_WAIT_1;
    __asm volatile ("dmb" ::: "memory");
    SRAMPRCR = SRAMPRCR_KEY_LOCK;
    __asm volatile ("dmb" ::: "memory");

    /* Configure main oscillator for a 24 MHz crystal and wait for stable MOSC. */
    MOMCR = 0x00U;
    MOSCWTCR = MOSCWTCR_MSTS_9;
    MOSCCR &= (uint8_t)~MOSCCR_MOSTP;
    while ((OSCSF & OSCSF_MOSCSF) == 0U)
    {
        __asm volatile ("nop");
    }

    /* XTAL / 3 * 25.0 = 200 MHz, per configuration.xml and RA6M5 PLL type 1 encoding. */
    PLLCCR = (uint16_t)(((uint16_t)PLL_MUL_X25 << PLLCCR_PLLMUL_POS)
           | ((uint16_t)PLL_SOURCE_MOSC << PLLCCR_PLSRCSEL_POS)
           | (uint16_t)PLL_DIV_3);
    PLLCR &= (uint8_t)~PLLCR_PLLSTP;
    while ((OSCSF & OSCSF_PLLSF) == 0U)
    {
        __asm volatile ("nop");
    }

    SCKDIVCR = ((uint32_t)SCKDIV_1 << SCKDIVCR_ICKPOS)   /* ICLK  = 200 MHz */
             | ((uint32_t)SCKDIV_2 << SCKDIVCR_PCKAPOS)  /* PCLKA = 100 MHz */
             | ((uint32_t)SCKDIV_4 << SCKDIVCR_PCKBPOS)  /* PCLKB =  50 MHz */
             | ((uint32_t)SCKDIV_4 << SCKDIVCR_PCKCPOS)  /* PCLKC =  50 MHz */
             | ((uint32_t)SCKDIV_2 << SCKDIVCR_PCKDPOS)  /* PCLKD = 100 MHz */
             | ((uint32_t)SCKDIV_2 << SCKDIVCR_BCKPOS)   /* BCLK  = 100 MHz */
             | ((uint32_t)SCKDIV_4 << SCKDIVCR_FCKPOS);  /* FCLK  =  50 MHz */

    SCKSCR = SCKSCR_PLL;

    RWP_Lock_Clock_MSTP();
}

/* -----------------------------------------------------------------------
 * CLK_ModuleStart_SCI — release module stop for one SCI channel.
 *
 * Replaces LPM_Unlock(). Wraps MSTPCRB write with PRCR protection.
 * MSTPCRB bit mapping: SCI0=bit31, SCI1=bit30, ..., SCI9=bit22
 * ----------------------------------------------------------------------- */
void CLK_ModuleStart_SCI(SCI_t peripheral)
{
    RWP_Unlock_Clock_MSTP();

    switch (peripheral)
    {
        case SCI0: MSTPCRB &= ~(1UL << 31U); break;
        case SCI1: MSTPCRB &= ~(1UL << 30U); break;
        case SCI2: MSTPCRB &= ~(1UL << 29U); break;
        case SCI3: MSTPCRB &= ~(1UL << 28U); break;
        case SCI4: MSTPCRB &= ~(1UL << 27U); break;
        case SCI5: MSTPCRB &= ~(1UL << 26U); break;
        case SCI6: MSTPCRB &= ~(1UL << 25U); break;
        case SCI7: MSTPCRB &= ~(1UL << 24U); break;
        case SCI8: MSTPCRB &= ~(1UL << 23U); break;
        case SCI9: MSTPCRB &= ~(1UL << 22U); break;
        default:   break;
    }

    RWP_Lock_Clock_MSTP();
}
