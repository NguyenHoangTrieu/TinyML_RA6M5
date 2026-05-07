#include "drv_usb.h"

#include "GPIO.h"
#include "drv_clk.h"
#include "drv_rwp.h"

#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

/* -----------------------------------------------------------------------
 * RA6M5 USBFS register map (USBFS0)
 *
 * NOTE:
 * The project does not currently include a device-specific register header.
 * Keep this map centralized so it can be swapped with re_ra6m5.h aliases
 * without touching logic code.
 * ----------------------------------------------------------------------- */
#define USBFS0_BASE   0x40090000UL

#define USB16(off)    (*(volatile uint16_t *)(uintptr_t)(USBFS0_BASE + (uint32_t)(off)))
#define USB32(off)    (*(volatile uint32_t *)(uintptr_t)(USBFS0_BASE + (uint32_t)(off)))

#define USB_SYSCFG     USB16(0x0000U)
#define USB_BUSWAIT    USB16(0x0002U)
#define USB_SYSSTS0    USB16(0x0004U)
#define USB_DVSTCTR0   USB16(0x0008U)

#define USB_CFIFO      USB32(0x0014U)
#define USB_D0FIFO     USB32(0x0018U)
#define USB_D1FIFO     USB32(0x001CU)

#define USB_CFIFOSEL   USB16(0x0020U)
#define USB_CFIFOCTR   USB16(0x0022U)
#define USB_D0FIFOSEL  USB16(0x0028U)
#define USB_D0FIFOCTR  USB16(0x002AU)
#define USB_D1FIFOSEL  USB16(0x002CU)
#define USB_D1FIFOCTR  USB16(0x002EU)

#define USB_INTENB0    USB16(0x0030U)
#define USB_INTENB1    USB16(0x0032U)
#define USB_BRDYENB    USB16(0x0036U)
#define USB_NRDYENB    USB16(0x0038U)
#define USB_BEMPENB    USB16(0x003AU)

#define USB_INTSTS0    USB16(0x0040U)
#define USB_INTSTS1    USB16(0x0042U)
#define USB_BRDYSTS    USB16(0x0046U)
#define USB_NRDYSTS    USB16(0x0048U)
#define USB_BEMPSTS    USB16(0x004AU)

#define USB_USBADDR    USB16(0x0050U)
#define USB_USBREQ     USB16(0x0054U)
#define USB_USBVAL     USB16(0x0056U)
#define USB_USBINDX    USB16(0x0058U)
#define USB_USBLENG    USB16(0x005AU)
#define USB_DCPCFG     USB16(0x005CU)
#define USB_DCPMAXP    USB16(0x005EU)
#define USB_DCPCTR     USB16(0x0060U)

#define USB_PIPESEL    USB16(0x0064U)
#define USB_PIPECFG    USB16(0x0068U)
#define USB_PIPEBUF    USB16(0x006AU)
#define USB_PIPEMAXP   USB16(0x006CU)
#define USB_PIPEPERI   USB16(0x006EU)

#define USB_PIPECTR(n) USB16((uint32_t)(0x0070U + ((uint32_t)(n) * 2U)))

/* -----------------------------------------------------------------------
 * USBFS bit fields
 * ----------------------------------------------------------------------- */
#define USB_SYSCFG_USBE      (1U << 0)
#define USB_SYSCFG_DPRPU     (1U << 4)
#define USB_SYSCFG_DRPD      (1U << 5)
#define USB_SYSCFG_DCFM      (1U << 6)

#define USB_SYSSTS0_LNST_MASK   (0x0003U)
#define USB_SYSSTS0_LNST_J       (0x0001U)
#define USB_SYSSTS0_LNST_K       (0x0002U)

#define USB_DVSTCTR0_UACT     (1U << 4)
#define USB_DVSTCTR0_USBRST    (1U << 6)

#define USB_INT_VBINT          (1U << 15)
#define USB_INT_RESM           (1U << 12)
#define USB_INT_DVST           (1U << 11)
#define USB_INT_CTRT           (1U << 10)
#define USB_INT_BEMP           (1U << 8)
#define USB_INT_NRDY           (1U << 7)
#define USB_INT_BRDY           (1U << 6)

#define USB_CTR_BSTS           (1U << 15)
#define USB_CTR_SUREQ          (1U << 14)
#define USB_CTR_SQCLR          (1U << 8)
#define USB_CTR_SQSET          (1U << 7)
#define USB_CTR_ACLRM          (1U << 9)
#define USB_CTR_PID_BUF        (1U << 0)
#define USB_CTR_PID_NAK        (0U << 0)

#define USB_FIFOCTR_FRDY       (1U << 13)
#define USB_FIFOCTR_BCLR       (1U << 14)
#define USB_FIFOCTR_DTLN_MASK  (0x01FFU)

#define USB_CFIFOSEL_ISEL      (1U << 5)
#define USB_CFIFOSEL_CURPIPE_MASK (0x000FU)

#define USB_PIPECFG_TYPE_MASK      (0x0003U << 14)
#define USB_PIPECFG_BULK           (0x0001U << 14)
#define USB_PIPECFG_DIR            (1U << 4)
#define USB_PIPECFG_DBLB           (1U << 9)

#define USB_DIR_OUT 0U
#define USB_DIR_IN  1U

/* RequestType helpers */
#define USB_REQ_DIR_IN          0x80U
#define USB_REQ_TYPE_STANDARD   0x00U
#define USB_REQ_TYPE_CLASS      0x20U
#define USB_REQ_RECIP_DEVICE    0x00U
#define USB_REQ_RECIP_INTERFACE 0x01U

/* Standard requests */
#define USB_REQ_GET_DESCRIPTOR   0x06U
#define USB_REQ_SET_ADDRESS      0x05U
#define USB_REQ_SET_CONFIGURATION 0x09U

/* CDC ACM class requests */
#define USB_CDC_REQ_SET_LINE_CODING  0x20U
#define USB_CDC_REQ_GET_LINE_CODING  0x21U

/* Descriptor types */
#define USB_DESC_DEVICE        0x01U
#define USB_DESC_CONFIGURATION 0x02U

/* USB setup packet */
typedef struct
{
    uint8_t bmRequestType;
    uint8_t bRequest;
    uint16_t wValue;
    uint16_t wIndex;
    uint16_t wLength;
} usb_setup_packet_t;

typedef enum
{
    USB_DEV_PENDING_NONE = 0,
    USB_DEV_PENDING_SET_LINE_CODING = 1
} usb_dev_pending_request_t;

/* -----------------------------------------------------------------------
 * Driver state
 * ----------------------------------------------------------------------- */
static usb_mode_t g_mode = USB_MODE_DEVICE_CDC;
static uint8_t g_usb_dev_address = 0U;
static uint8_t g_usb_dev_configured = 0U;
static usb_dev_pending_request_t g_dev_pending_request = USB_DEV_PENDING_NONE;

static usb_line_coding_t g_line_coding = {
    115200UL, 0U, 0U, 8U
};

static usb_host_cdc_device_t g_host_dev;
static usb_host_descriptor_snapshot_t g_host_desc;

/* EK-RA6M5 USBFS VBUSEN pin from configuration.xml: P500 */
#define USB_VBUS_EN_PORT GPIO_PORT5
#define USB_VBUS_EN_PIN  0U

/* MSTPCRB bit for USBFS module stop release (project-level constant). */
#define USBFS_MSTP_BIT   11U

/* Endpoint/pipe allocation for this driver */
#define USB_PIPE_CDC_DEV_BULK_IN   1U
#define USB_PIPE_CDC_HOST_BULK_OUT 1U
#define USB_PIPE_CDC_HOST_BULK_IN  2U

/* -----------------------------------------------------------------------
 * CDC descriptors (Device mode)
 * ----------------------------------------------------------------------- */
static const uint8_t g_usb_dev_desc[] = {
    18U, USB_DESC_DEVICE,
    0x00U, 0x02U,     /* USB 2.00 */
    0x02U,            /* CDC class */
    0x00U,
    0x00U,
    64U,              /* EP0 max packet */
    0x34U, 0x12U,     /* VID 0x1234 */
    0x78U, 0x56U,     /* PID 0x5678 */
    0x00U, 0x01U,     /* bcdDevice */
    1U, 2U, 3U,
    1U
};

/*
 * Configuration layout:
 * - Interface 0: CDC Communication Class
 * - Interface 1: CDC Data Class
 * - Endpoint IN  (bulk, EP1 IN)
 * - Endpoint OUT (bulk, EP2 OUT)
 */
static const uint8_t g_usb_cfg_desc[] = {
    /* Configuration descriptor */
    9U, USB_DESC_CONFIGURATION,
    0x3CU, 0x00U,   /* total length = 60 */
    2U,             /* two interfaces */
    1U,
    0U,
    0x80U,
    50U,

    /* Interface 0: CDC communication */
    9U, 0x04U,
    0U, 0U,
    0U,
    0x02U, 0x02U, 0x01U,
    0U,

    /* CDC Header functional descriptor */
    5U, 0x24U, 0x00U, 0x10U, 0x01U,
    /* CDC ACM functional descriptor */
    4U, 0x24U, 0x02U, 0x02U,
    /* CDC Union functional descriptor */
    5U, 0x24U, 0x06U, 0U, 1U,
    /* CDC Call management */
    5U, 0x24U, 0x01U, 0x00U, 1U,

    /* Interface 1: CDC Data */
    9U, 0x04U,
    1U, 0U,
    2U,
    0x0AU, 0x00U, 0x00U,
    0U,

    /* EP1 IN BULK */
    7U, 0x05U, 0x81U, 0x02U,
    64U, 0x00U,
    0U,
    /* EP2 OUT BULK */
    7U, 0x05U, 0x02U, 0x02U,
    64U, 0x00U,
    0U
};

/* -----------------------------------------------------------------------
 * Utility helpers
 * ----------------------------------------------------------------------- */
static drv_status_t usb_wait_reg16(volatile uint16_t *reg,
                                   uint16_t mask,
                                   uint16_t expected,
                                   uint32_t timeout)
{
    while (((*reg) & mask) != expected)
    {
        if (timeout == 0U)
        {
            return DRV_TIMEOUT;
        }
        timeout--;
    }
    return DRV_OK;
}

static void usb_delay_cycles(uint32_t cycles)
{
    while (cycles > 0U)
    {
        __asm volatile ("nop");
        cycles--;
    }
}

static void usb_mstp_release(void)
{
    RWP_Unlock_Clock_MSTP();
    MSTPCRB &= (uint32_t)~(1UL << USBFS_MSTP_BIT);
    RWP_Lock_Clock_MSTP();
}

static void usb_fifo_select_pipe(uint8_t pipe, uint8_t is_dcp_in)
{
    uint16_t sel = (uint16_t)(pipe & (uint8_t)USB_CFIFOSEL_CURPIPE_MASK);
    if (is_dcp_in != 0U)
    {
        sel |= USB_CFIFOSEL_ISEL;
    }
    USB_CFIFOSEL = sel;
}

static void usb_clear_brdy(uint8_t pipe)
{
    USB_BRDYSTS = (uint16_t)~(1U << pipe);
}

static drv_status_t usb_fifo_wait_ready(void)
{
    return usb_wait_reg16(&USB_CFIFOCTR, USB_FIFOCTR_FRDY, USB_FIFOCTR_FRDY, DRV_TIMEOUT_TICKS);
}

static drv_status_t usb_fifo_write(const uint8_t *data, uint16_t len)
{
    uint16_t i;
    if (usb_fifo_wait_ready() != DRV_OK)
    {
        return DRV_TIMEOUT;
    }

    for (i = 0U; i < len; i += 4U)
    {
        uint32_t v = 0U;
        uint16_t rem = (uint16_t)(len - i);
        v |= (uint32_t)data[i];
        if (rem > 1U) { v |= (uint32_t)data[i + 1U] << 8; }
        if (rem > 2U) { v |= (uint32_t)data[i + 2U] << 16; }
        if (rem > 3U) { v |= (uint32_t)data[i + 3U] << 24; }
        USB_CFIFO = v;
    }

    return DRV_OK;
}

static uint16_t usb_fifo_read(uint8_t *data, uint16_t cap)
{
    uint16_t dtln = (uint16_t)(USB_CFIFOCTR & USB_FIFOCTR_DTLN_MASK);
    uint16_t i;
    uint16_t use_len = (dtln < cap) ? dtln : cap;

    if (usb_fifo_wait_ready() != DRV_OK)
    {
        return 0U;
    }

    for (i = 0U; i < use_len; i += 4U)
    {
        uint32_t v = USB_CFIFO;
        data[i] = (uint8_t)(v & 0xFFU);
        if ((uint16_t)(i + 1U) < use_len) { data[i + 1U] = (uint8_t)((v >> 8) & 0xFFU); }
        if ((uint16_t)(i + 2U) < use_len) { data[i + 2U] = (uint8_t)((v >> 16) & 0xFFU); }
        if ((uint16_t)(i + 3U) < use_len) { data[i + 3U] = (uint8_t)((v >> 24) & 0xFFU); }
    }

    if (dtln > cap)
    {
        /* Drop remaining bytes in FIFO packet to recover FIFO state. */
        uint16_t remain = (uint16_t)(dtln - cap);
        while (remain > 0U)
        {
            (void)USB_CFIFO;
            remain = (remain > 4U) ? (uint16_t)(remain - 4U) : 0U;
        }
    }

    return use_len;
}

static void usb_pipe_set_pid(uint8_t pipe, uint16_t pid)
{
    if (pipe == 0U)
    {
        USB_DCPCTR = (uint16_t)((USB_DCPCTR & (uint16_t)~0x0003U) | (pid & 0x0003U));
    }
    else
    {
        volatile uint16_t *ctr = &USB_PIPECTR(pipe);
        *ctr = (uint16_t)((*ctr & (uint16_t)~0x0003U) | (pid & 0x0003U));
    }
}

static drv_status_t usb_device_send_control_data(const uint8_t *data, uint16_t len)
{
    usb_fifo_select_pipe(0U, 1U);
    if (usb_fifo_write(data, len) != DRV_OK)
    {
        return DRV_TIMEOUT;
    }
    usb_pipe_set_pid(0U, USB_CTR_PID_BUF);
    return DRV_OK;
}

static void usb_stall_control(void)
{
    USB_DCPCTR |= USB_CTR_SQSET;
}

static drv_status_t usb_device_complete_set_line_coding(void)
{
    uint8_t buf[7];

    if ((USB_BRDYSTS & 0x0001U) == 0U)
    {
        return DRV_ERR;
    }

    usb_fifo_select_pipe(0U, USB_DIR_OUT);
    usb_clear_brdy(0U);
    if (usb_fifo_read(buf, (uint16_t)sizeof(buf)) != (uint16_t)sizeof(buf))
    {
        return DRV_ERR;
    }

    g_line_coding.baudrate = (uint32_t)buf[0]
                           | ((uint32_t)buf[1] << 8)
                           | ((uint32_t)buf[2] << 16)
                           | ((uint32_t)buf[3] << 24);
    g_line_coding.stop_bits = buf[4];
    g_line_coding.parity = buf[5];
    g_line_coding.data_bits = buf[6];
    g_dev_pending_request = USB_DEV_PENDING_NONE;

    /* Release the status stage only after the OUT data payload was consumed. */
    USB_DCPCTR |= USB_CTR_PID_BUF;
    return DRV_OK;
}

/* -----------------------------------------------------------------------
 * Device CDC setup packet handling
 * ----------------------------------------------------------------------- */
static void usb_device_handle_standard_request(const usb_setup_packet_t *s)
{
    if (s->bRequest == USB_REQ_SET_ADDRESS)
    {
        g_usb_dev_address = (uint8_t)(s->wValue & 0x7FU);
        USB_DCPCTR |= USB_CTR_PID_BUF;
        return;
    }

    if (s->bRequest == USB_REQ_SET_CONFIGURATION)
    {
        g_usb_dev_configured = (uint8_t)(s->wValue & 0xFFU);
        USB_DCPCTR |= USB_CTR_PID_BUF;
        return;
    }

    if (s->bRequest == USB_REQ_GET_DESCRIPTOR)
    {
        uint8_t dtype = (uint8_t)((s->wValue >> 8) & 0xFFU);
        if (dtype == USB_DESC_DEVICE)
        {
            uint16_t len = (s->wLength < (uint16_t)sizeof(g_usb_dev_desc)) ? s->wLength : (uint16_t)sizeof(g_usb_dev_desc);
            (void)usb_device_send_control_data(g_usb_dev_desc, len);
            return;
        }
        if (dtype == USB_DESC_CONFIGURATION)
        {
            uint16_t len = (s->wLength < (uint16_t)sizeof(g_usb_cfg_desc)) ? s->wLength : (uint16_t)sizeof(g_usb_cfg_desc);
            (void)usb_device_send_control_data(g_usb_cfg_desc, len);
            return;
        }
    }

    usb_stall_control();
}

static void usb_device_handle_class_request(const usb_setup_packet_t *s)
{
    if ((s->bmRequestType == (uint8_t)(USB_REQ_TYPE_CLASS | USB_REQ_RECIP_INTERFACE))
        && (s->bRequest == USB_CDC_REQ_SET_LINE_CODING)
        && (s->wLength == 7U))
    {
        g_dev_pending_request = USB_DEV_PENDING_SET_LINE_CODING;
        USB_DCPCTR |= USB_CTR_PID_BUF;
        return;
    }

    if ((s->bmRequestType == (uint8_t)(USB_REQ_DIR_IN | USB_REQ_TYPE_CLASS | USB_REQ_RECIP_INTERFACE))
        && (s->bRequest == USB_CDC_REQ_GET_LINE_CODING)
        && (s->wLength >= 7U))
    {
        uint8_t buf[7];
        buf[0] = (uint8_t)(g_line_coding.baudrate & 0xFFU);
        buf[1] = (uint8_t)((g_line_coding.baudrate >> 8) & 0xFFU);
        buf[2] = (uint8_t)((g_line_coding.baudrate >> 16) & 0xFFU);
        buf[3] = (uint8_t)((g_line_coding.baudrate >> 24) & 0xFFU);
        buf[4] = g_line_coding.stop_bits;
        buf[5] = g_line_coding.parity;
        buf[6] = g_line_coding.data_bits;
        (void)usb_device_send_control_data(buf, 7U);
        return;
    }

    usb_stall_control();
}

static void usb_device_handle_setup(void)
{
    usb_setup_packet_t s;
    uint16_t req = USB_USBREQ;

    s.bmRequestType = (uint8_t)(req & 0x00FFU);
    s.bRequest = (uint8_t)((req >> 8) & 0x00FFU);
    s.wValue = USB_USBVAL;
    s.wIndex = USB_USBINDX;
    s.wLength = USB_USBLENG;

    if ((s.bmRequestType & 0x60U) == USB_REQ_TYPE_STANDARD)
    {
        usb_device_handle_standard_request(&s);
        return;
    }

    if ((s.bmRequestType & 0x60U) == USB_REQ_TYPE_CLASS)
    {
        usb_device_handle_class_request(&s);
        return;
    }

    usb_stall_control();
}

/* -----------------------------------------------------------------------
 * Host-mode control transfer helpers
 * ----------------------------------------------------------------------- */
static drv_status_t usb_host_send_setup(const usb_setup_packet_t *s)
{
    USB_USBREQ = (uint16_t)(((uint16_t)s->bRequest << 8) | s->bmRequestType);
    USB_USBVAL = s->wValue;
    USB_USBINDX = s->wIndex;
    USB_USBLENG = s->wLength;
    USB_DCPCTR |= USB_CTR_SUREQ;
    return usb_wait_reg16(&USB_DCPCTR, USB_CTR_SUREQ, 0U, DRV_TIMEOUT_TICKS);
}

static drv_status_t usb_host_control_in(const usb_setup_packet_t *s, uint8_t *buf, uint16_t cap, uint16_t *actual)
{
    drv_status_t st = usb_host_send_setup(s);
    uint16_t total = 0U;

    if (st != DRV_OK)
    {
        return st;
    }

    usb_fifo_select_pipe(0U, 1U);
    usb_pipe_set_pid(0U, USB_CTR_PID_BUF);

    while (total < s->wLength)
    {
        if (usb_wait_reg16(&USB_BRDYSTS, 0x0001U, 0x0001U, DRV_TIMEOUT_TICKS) != DRV_OK)
        {
            return DRV_TIMEOUT;
        }
        USB_BRDYSTS = (uint16_t)~0x0001U;

        {
            uint16_t step = usb_fifo_read(&buf[total], (uint16_t)(cap - total));
            total = (uint16_t)(total + step);
            if (step < 64U)
            {
                break;
            }
        }
    }

    usb_pipe_set_pid(0U, USB_CTR_PID_NAK);
    if (actual != NULL)
    {
        *actual = total;
    }
    return DRV_OK;
}

static drv_status_t usb_host_control_no_data(const usb_setup_packet_t *s)
{
    return usb_host_send_setup(s);
}

static drv_status_t usb_host_reset_bus(void)
{
    USB_DVSTCTR0 |= USB_DVSTCTR0_USBRST;
    for (volatile uint32_t d = 0U; d < 10000U; d++)
    {
        __asm volatile ("nop");
    }
    USB_DVSTCTR0 &= (uint16_t)~USB_DVSTCTR0_USBRST;
    for (volatile uint32_t d = 0U; d < 10000U; d++)
    {
        __asm volatile ("nop");
    }
    return DRV_OK;
}

static uint8_t usb_host_is_attach_detected(void)
{
    uint16_t lnst = (uint16_t)(USB_SYSSTS0 & USB_SYSSTS0_LNST_MASK);
    return (uint8_t)((lnst == USB_SYSSTS0_LNST_J) || (lnst == USB_SYSSTS0_LNST_K));
}

static drv_status_t usb_host_wait_attach(void)
{
    uint32_t to = DRV_TIMEOUT_TICKS;
    while (to > 0U)
    {
        if (usb_host_is_attach_detected() != 0U)
        {
            return DRV_OK;
        }
        to--;
    }
    return DRV_TIMEOUT;
}

static void usb_host_parse_cdc_endpoints(const uint8_t *cfg, uint16_t len)
{
    uint16_t idx = 0U;
    g_host_dev.bulk_in_ep = 0U;
    g_host_dev.bulk_out_ep = 0U;
    g_host_dev.bulk_in_mps = 64U;
    g_host_dev.bulk_out_mps = 64U;

    while ((uint16_t)(idx + 1U) < len)
    {
        uint8_t dlen = cfg[idx + 0U];
        uint8_t dtype = cfg[idx + 1U];
        if ((dlen < 2U) || ((uint16_t)(idx + dlen) > len))
        {
            break;
        }

        if ((dtype == 0x05U) && (dlen >= 7U))
        {
            uint8_t ep_addr = cfg[idx + 2U];
            uint8_t attr = cfg[idx + 3U];
            uint16_t mps = (uint16_t)cfg[idx + 4U] | ((uint16_t)cfg[idx + 5U] << 8);
            if ((attr & 0x03U) == 0x02U)
            {
                if ((ep_addr & 0x80U) != 0U)
                {
                    g_host_dev.bulk_in_ep = (uint8_t)(ep_addr & 0x0FU);
                    g_host_dev.bulk_in_mps = mps;
                }
                else
                {
                    g_host_dev.bulk_out_ep = (uint8_t)(ep_addr & 0x0FU);
                    g_host_dev.bulk_out_mps = mps;
                }
            }
        }

        idx = (uint16_t)(idx + dlen);
    }
}

static void usb_host_configure_bulk_pipes(void)
{
    /* PIPE1: host BULK OUT */
    USB_PIPESEL = USB_PIPE_CDC_HOST_BULK_OUT;
    USB_PIPECFG = (uint16_t)(USB_PIPECFG_BULK | (uint16_t)(g_host_dev.bulk_out_ep & 0x0FU));
    USB_PIPEMAXP = (uint16_t)(((uint16_t)g_host_dev.address << 12) | (g_host_dev.bulk_out_mps & 0x07FFU));

    /* PIPE2: host BULK IN */
    USB_PIPESEL = USB_PIPE_CDC_HOST_BULK_IN;
    USB_PIPECFG = (uint16_t)(USB_PIPECFG_BULK | USB_PIPECFG_DIR | (uint16_t)(g_host_dev.bulk_in_ep & 0x0FU));
    USB_PIPEMAXP = (uint16_t)(((uint16_t)g_host_dev.address << 12) | (g_host_dev.bulk_in_mps & 0x07FFU));

    /* Restore PIPESEL to DCP-safe default */
    USB_PIPESEL = 0U;
}

/* -----------------------------------------------------------------------
 * Public API
 * ----------------------------------------------------------------------- */
drv_status_t USB_Init(usb_mode_t mode)
{
    g_mode = mode;
    g_usb_dev_address = 0U;
    g_usb_dev_configured = 0U;
    g_dev_pending_request = USB_DEV_PENDING_NONE;
    memset(&g_host_dev, 0, sizeof(g_host_dev));
    memset(&g_host_desc, 0, sizeof(g_host_desc));

    usb_mstp_release();

    USB_INTENB0 = 0U;
    USB_INTENB1 = 0U;
    USB_BRDYENB = 0U;
    USB_NRDYENB = 0U;
    USB_BEMPENB = 0U;

    /* Clear stale statuses before enabling USB engine. */
    USB_INTSTS0 = 0U;
    USB_INTSTS1 = 0U;
    USB_BRDYSTS = 0U;
    USB_NRDYSTS = 0U;
    USB_BEMPSTS = 0U;

    USB_BUSWAIT = 0x0005U;

    if (mode == USB_MODE_DEVICE_CDC)
    {
        /* Device mode: DCFM=0, pull-down off, pull-up on. */
        USB_SYSCFG = (uint16_t)(USB_SYSCFG_USBE | USB_SYSCFG_DPRPU);

        USB_DCPMAXP = 64U;

        /* PIPE1 = BULK IN endpoint 1 for logging stream */
        USB_PIPESEL = USB_PIPE_CDC_DEV_BULK_IN;
        USB_PIPECFG = (uint16_t)(USB_PIPECFG_BULK | USB_PIPECFG_DIR | 0x0001U | USB_PIPECFG_DBLB);
        USB_PIPEMAXP = 64U;
        USB_PIPESEL = 0U;

        USB_INTENB0 = (uint16_t)(USB_INT_DVST | USB_INT_CTRT | USB_INT_BRDY | USB_INT_NRDY | USB_INT_BEMP);
        USB_DVSTCTR0 |= USB_DVSTCTR0_UACT;
    }
    else
    {
        /* Host mode: DCFM=1, pull-down enabled, pull-up disabled. */
        USB_SYSCFG = (uint16_t)(USB_SYSCFG_USBE | USB_SYSCFG_DCFM | USB_SYSCFG_DRPD);

        /* Enable board-side VBUS switch (P500 = USBFS_VBUS_EN). */
        GPIO_Config(USB_VBUS_EN_PORT, USB_VBUS_EN_PIN, GPIO_CNF_OUT_PP, GPIO_MODE_OUTPUT);
        GPIO_Write_Pin(USB_VBUS_EN_PORT, USB_VBUS_EN_PIN, GPIO_PIN_SET);

        USB_INTENB0 = (uint16_t)(USB_INT_VBINT | USB_INT_CTRT | USB_INT_BRDY | USB_INT_NRDY | USB_INT_BEMP);
        USB_DVSTCTR0 |= USB_DVSTCTR0_UACT;
    }

    return DRV_OK;
}

void USB_Deinit(void)
{
    USB_INTENB0 = 0U;
    USB_INTENB1 = 0U;
    USB_BRDYENB = 0U;
    USB_NRDYENB = 0U;
    USB_BEMPENB = 0U;
    USB_DVSTCTR0 &= (uint16_t)~USB_DVSTCTR0_UACT;
    USB_SYSCFG = 0U;

    if (g_mode == USB_MODE_HOST_CDC_ACM)
    {
        GPIO_Write_Pin(USB_VBUS_EN_PORT, USB_VBUS_EN_PIN, GPIO_PIN_RESET);
    }
}

void USB_Dev_SetConfigured(uint8_t configured)
{
    g_usb_dev_configured = configured;
}

uint8_t USB_Dev_IsConfigured(void)
{
    return g_usb_dev_configured;
}

drv_status_t USB_Dev_Write(const uint8_t *data, uint32_t len)
{
    uint32_t sent = 0U;

    if ((g_mode != USB_MODE_DEVICE_CDC) || (g_usb_dev_configured == 0U) || (data == NULL))
    {
        return DRV_ERR;
    }

    usb_fifo_select_pipe(USB_PIPE_CDC_DEV_BULK_IN, USB_DIR_IN);

    while (sent < len)
    {
        uint16_t chunk = (uint16_t)((len - sent) > 64U ? 64U : (len - sent));
        if (usb_fifo_write(&data[sent], chunk) != DRV_OK)
        {
            return DRV_TIMEOUT;
        }
        USB_BEMPSTS = (uint16_t)~(1U << USB_PIPE_CDC_DEV_BULK_IN);
        usb_pipe_set_pid(USB_PIPE_CDC_DEV_BULK_IN, USB_CTR_PID_BUF);

        if (usb_wait_reg16(&USB_BEMPSTS,
                           (uint16_t)(1U << USB_PIPE_CDC_DEV_BULK_IN),
                           (uint16_t)(1U << USB_PIPE_CDC_DEV_BULK_IN),
                           DRV_TIMEOUT_TICKS) != DRV_OK)
        {
            return DRV_TIMEOUT;
        }

        sent += chunk;
    }

    return DRV_OK;
}

drv_status_t USB_Dev_Printf(const char *fmt, ...)
{
    char buf[192];
    int n;
    va_list ap;

    if (fmt == NULL)
    {
        return DRV_ERR;
    }

    va_start(ap, fmt);
    n = vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    if (n < 0)
    {
        return DRV_ERR;
    }

    if ((size_t)n >= sizeof(buf))
    {
        n = (int)(sizeof(buf) - 1U);
    }

    return USB_Dev_Write((const uint8_t *)buf, (uint32_t)n);
}

drv_status_t USB_Host_Enumerate(void)
{
    uint8_t dev_desc[18];
    uint8_t cfg_hdr[9];
    uint8_t cfg_desc[256];
    uint16_t actual = 0U;
    uint16_t cfg_len;
    usb_setup_packet_t s;

    if (g_mode != USB_MODE_HOST_CDC_ACM)
    {
        return DRV_ERR;
    }

    /* 1) Attach detect */
    if (usb_host_wait_attach() != DRV_OK)
    {
        return DRV_TIMEOUT;
    }

    /* 2) Bus reset */
    if (usb_host_reset_bus() != DRV_OK)
    {
        return DRV_ERR;
    }

    /* 3) Read Device Descriptor (first 18 bytes). */
    s.bmRequestType = (uint8_t)(USB_REQ_DIR_IN | USB_REQ_TYPE_STANDARD | USB_REQ_RECIP_DEVICE);
    s.bRequest = USB_REQ_GET_DESCRIPTOR;
    s.wValue = (uint16_t)((uint16_t)USB_DESC_DEVICE << 8);
    s.wIndex = 0U;
    s.wLength = 18U;
    if (usb_host_control_in(&s, dev_desc, (uint16_t)sizeof(dev_desc), &actual) != DRV_OK)
    {
        return DRV_TIMEOUT;
    }
    if (actual < 8U)
    {
        return DRV_ERR;
    }

    memcpy(g_host_desc.device_desc, dev_desc, sizeof(dev_desc));
    g_host_desc.max_packet_size_ep0 = dev_desc[7];

    USB_DCPMAXP = dev_desc[7];

    /* 4) SET_ADDRESS = 1 */
    s.bmRequestType = (uint8_t)(USB_REQ_TYPE_STANDARD | USB_REQ_RECIP_DEVICE);
    s.bRequest = USB_REQ_SET_ADDRESS;
    s.wValue = 1U;
    s.wIndex = 0U;
    s.wLength = 0U;
    if (usb_host_control_no_data(&s) != DRV_OK)
    {
        return DRV_TIMEOUT;
    }

    usb_delay_cycles(10000U);

    g_host_dev.address = 1U;
    USB_USBADDR = (uint16_t)((uint16_t)g_host_dev.address << 12);

    /* 5) Read configuration header first to obtain wTotalLength. */
    s.bmRequestType = (uint8_t)(USB_REQ_DIR_IN | USB_REQ_TYPE_STANDARD | USB_REQ_RECIP_DEVICE);
    s.bRequest = USB_REQ_GET_DESCRIPTOR;
    s.wValue = (uint16_t)((uint16_t)USB_DESC_CONFIGURATION << 8);
    s.wIndex = 0U;
    s.wLength = (uint16_t)sizeof(cfg_hdr);
    if (usb_host_control_in(&s, cfg_hdr, (uint16_t)sizeof(cfg_hdr), &actual) != DRV_OK)
    {
        return DRV_TIMEOUT;
    }
    if (actual < 9U)
    {
        return DRV_ERR;
    }

    g_host_desc.config_total_length = (uint16_t)cfg_hdr[2] | ((uint16_t)cfg_hdr[3] << 8);
    g_host_desc.num_interfaces = cfg_hdr[4];
    g_host_desc.configuration_value = cfg_hdr[5];

    cfg_len = g_host_desc.config_total_length;
    if (cfg_len > (uint16_t)sizeof(cfg_desc))
    {
        cfg_len = (uint16_t)sizeof(cfg_desc);
    }

    s.wLength = cfg_len;
    if (usb_host_control_in(&s, cfg_desc, cfg_len, &actual) != DRV_OK)
    {
        return DRV_TIMEOUT;
    }
    if (actual < 9U)
    {
        return DRV_ERR;
    }

    usb_host_parse_cdc_endpoints(cfg_desc, actual);
    if ((g_host_dev.bulk_in_ep == 0U) || (g_host_dev.bulk_out_ep == 0U))
    {
        return DRV_ERR;
    }

    /* 6) SET_CONFIGURATION */
    g_host_dev.config_value = g_host_desc.configuration_value;
    s.bmRequestType = (uint8_t)(USB_REQ_TYPE_STANDARD | USB_REQ_RECIP_DEVICE);
    s.bRequest = USB_REQ_SET_CONFIGURATION;
    s.wValue = g_host_dev.config_value;
    s.wIndex = 0U;
    s.wLength = 0U;
    if (usb_host_control_no_data(&s) != DRV_OK)
    {
        return DRV_TIMEOUT;
    }

    usb_host_configure_bulk_pipes();
    g_host_dev.configured = 1U;
    return DRV_OK;
}

drv_status_t USB_Host_Send_AT_Command(char *cmd)
{
    uint32_t len;
    uint32_t sent = 0U;

    if ((g_mode != USB_MODE_HOST_CDC_ACM) || (g_host_dev.configured == 0U) || (cmd == NULL))
    {
        return DRV_ERR;
    }

    len = (uint32_t)strlen(cmd);
    usb_fifo_select_pipe(USB_PIPE_CDC_HOST_BULK_OUT, USB_DIR_OUT);

    while (sent < len)
    {
        uint16_t chunk = (uint16_t)((len - sent) > g_host_dev.bulk_out_mps
                              ? g_host_dev.bulk_out_mps : (len - sent));
        if (usb_fifo_write((const uint8_t *)&cmd[sent], chunk) != DRV_OK)
        {
            return DRV_TIMEOUT;
        }

        USB_BEMPSTS = (uint16_t)~(1U << USB_PIPE_CDC_HOST_BULK_OUT);
        usb_pipe_set_pid(USB_PIPE_CDC_HOST_BULK_OUT, USB_CTR_PID_BUF);

        if (usb_wait_reg16(&USB_BEMPSTS,
                           (uint16_t)(1U << USB_PIPE_CDC_HOST_BULK_OUT),
                           (uint16_t)(1U << USB_PIPE_CDC_HOST_BULK_OUT),
                           DRV_TIMEOUT_TICKS) != DRV_OK)
        {
            return DRV_TIMEOUT;
        }
        sent += chunk;
    }

    return DRV_OK;
}

int32_t USB_Host_Read_Response(uint8_t *buffer, uint32_t len)
{
    uint32_t recved = 0U;

    if ((g_mode != USB_MODE_HOST_CDC_ACM) || (g_host_dev.configured == 0U) || (buffer == NULL))
    {
        return -1;
    }

    usb_fifo_select_pipe(USB_PIPE_CDC_HOST_BULK_IN, USB_DIR_IN);
    usb_pipe_set_pid(USB_PIPE_CDC_HOST_BULK_IN, USB_CTR_PID_BUF);

    while (recved < len)
    {
        if (usb_wait_reg16(&USB_BRDYSTS,
                           (uint16_t)(1U << USB_PIPE_CDC_HOST_BULK_IN),
                           (uint16_t)(1U << USB_PIPE_CDC_HOST_BULK_IN),
                           DRV_TIMEOUT_TICKS) != DRV_OK)
        {
            break;
        }

        USB_BRDYSTS = (uint16_t)~(1U << USB_PIPE_CDC_HOST_BULK_IN);

        {
            uint16_t step = usb_fifo_read(&buffer[recved], (uint16_t)(len - recved));
            recved += step;
            if ((step == 0U) || (step < g_host_dev.bulk_in_mps))
            {
                break;
            }
        }
    }

    if (recved == 0U)
    {
        return -1;
    }

    return (int32_t)recved;
}

const usb_host_cdc_device_t *USB_Host_GetDeviceInfo(void)
{
    return &g_host_dev;
}

const usb_host_descriptor_snapshot_t *USB_Host_GetDescriptorSnapshot(void)
{
    return &g_host_desc;
}

void USB_PollEvents(void)
{
    uint16_t is0 = USB_INTSTS0;

    if ((g_mode == USB_MODE_DEVICE_CDC)
        && (g_dev_pending_request == USB_DEV_PENDING_SET_LINE_CODING)
        && ((USB_BRDYSTS & 0x0001U) != 0U))
    {
        if (usb_device_complete_set_line_coding() != DRV_OK)
        {
            usb_stall_control();
            g_dev_pending_request = USB_DEV_PENDING_NONE;
        }
    }

    if ((is0 & USB_INT_DVST) != 0U)
    {
        USB_INTSTS0 = (uint16_t)~USB_INT_DVST;
    }

    if ((is0 & USB_INT_CTRT) != 0U)
    {
        if (g_mode == USB_MODE_DEVICE_CDC)
        {
            usb_device_handle_setup();
        }
        USB_INTSTS0 = (uint16_t)~USB_INT_CTRT;
    }

    if ((is0 & USB_INT_BRDY) != 0U)
    {
        /* BRDY is consumed by data path polling helpers; clear latched bit. */
        USB_INTSTS0 = (uint16_t)~USB_INT_BRDY;
    }

    if ((is0 & USB_INT_BEMP) != 0U)
    {
        /* BEMP indicates TX empty for selected pipe. */
        USB_INTSTS0 = (uint16_t)~USB_INT_BEMP;
    }

    if ((is0 & USB_INT_NRDY) != 0U)
    {
        /* NRDY is treated as a recoverable endpoint-level error. */
        USB_INTSTS0 = (uint16_t)~USB_INT_NRDY;
    }

    if ((is0 & USB_INT_VBINT) != 0U)
    {
        USB_INTSTS0 = (uint16_t)~USB_INT_VBINT;
    }
}

void USBI0_IRQHandler(void)
{
    USB_PollEvents();
}

void USBI1_IRQHandler(void)
{
    USB_PollEvents();
}
