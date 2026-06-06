#include "usbd_cdc.h"
#include <string.h>
#include <stdarg.h>
#include <stdio.h>

#if USB_MSC_ENABLE

extern "C" {
#include <STM32_USB_Device_Library/Core/inc/usbd_core.h>
#include <STM32_USB_Device_Library/Core/inc/usbd_ioreq.h>
#include <STM32_USB_Device_Library/Core/inc/usbd_usr.h>
#include <STM32_USB_OTG_Driver/inc/usb_dcd.h>
#include <VCP/usbd_desc.h>
}

extern USB_OTG_CORE_HANDLE USB_OTG_dev;

/* USB_OTG_HS 寄存器基址（与 usbd_msc.cpp 保持一致） */
#define USB_OTG_HS_BASE       0x40040000UL
#define GUSBCFG_OFFSET        0x00CU

/* ==================== CDC 全局状态 ==================== */

bool                g_cdc_ready = false;
LINE_CODING_TypeDef g_line_coding = { 115200, 0, 0, 8 };
uint8_t             g_cdc_rx_buf[CDC_RX_BUF_SIZE];
volatile uint32_t   g_cdc_rx_head = 0;
volatile uint32_t   g_cdc_rx_tail = 0;
uint8_t             g_cdc_tx_buf[CDC_TX_BUF_SIZE];
volatile uint32_t   g_cdc_tx_head = 0;
volatile uint32_t   g_cdc_tx_tail = 0;
volatile bool       g_cdc_dtr = false;
USB_OTG_CORE_HANDLE *g_cdc_pdev = NULL;

/* 专用端点缓冲区：OUT 数据先收在此，再拷贝到环形缓冲区 */
static uint8_t cdc_out_buf[CDC_DATA_PACKET_SIZE];

/* ==================== CDC 配置描述符（独立模式, 67 字节） ==================== */

static uint8_t cdc_cfg_desc[USB_CDC_CONFIG_DESC_SIZ] = {
    /* 配置描述符 (9B) */
    0x09, 0x02,
    (uint8_t)(USB_CDC_CONFIG_DESC_SIZ & 0xFF),
    (uint8_t)((USB_CDC_CONFIG_DESC_SIZ >> 8) & 0xFF),
    0x02, 0x01, 0x00, 0xC0, 0x32,

    /* ---- Interface 0: Communication Interface ---- */
    0x09, 0x04, 0x00, 0x00, 0x01, 0x02, 0x02, 0x00, 0x00,
    /* Header Func Desc */
    0x05, 0x24, 0x00, 0x10, 0x01,
    /* Call Management Func Desc */
    0x05, 0x24, 0x01, 0x00, 0x01,
    /* ACM Func Desc */
    0x04, 0x24, 0x02, 0x02,
    /* Union Func Desc */
    0x05, 0x24, 0x06, 0x00, 0x01,
    /* EP Interrupt IN (通知) */
    0x07, 0x05, CDC_NOTIF_EP, 0x03,
    (uint8_t)(CDC_NOTIF_PACKET_SIZE & 0xFF),
    (uint8_t)((CDC_NOTIF_PACKET_SIZE >> 8) & 0xFF),
    0x10,

    /* ---- Interface 1: Data Interface ---- */
    0x09, 0x04, 0x01, 0x00, 0x02, 0x0A, 0x00, 0x00, 0x00,
    /* EP Bulk OUT (数据) */
    0x07, 0x05, CDC_DATA_OUT_EP, 0x02,
    (uint8_t)(CDC_DATA_PACKET_SIZE & 0xFF),
    (uint8_t)((CDC_DATA_PACKET_SIZE >> 8) & 0xFF),
    0x00,
    /* EP Bulk IN (数据) */
    0x07, 0x05, CDC_DATA_IN_EP, 0x02,
    (uint8_t)(CDC_DATA_PACKET_SIZE & 0xFF),
    (uint8_t)((CDC_DATA_PACKET_SIZE >> 8) & 0xFF),
    0x00,
};

/* ==================== CDC 接口描述符片段（供复合描述符 memcpy） ==================== */

/* 复合设备中 CDC 占用 Interface 2 (通信) + Interface 3 (数据) */
/* 总大小: 9(Comm Iface) + 5+5+4+5 = 28 + 7(Notif EP) + 9(Data Iface) + 7+7(Data EPs) = 58 */
#define CDC_IFACE_DESC_SIZE  58

static uint8_t cdc_iface_desc[CDC_IFACE_DESC_SIZE] = {
    /* Interface 2: Communication */
    0x09, 0x04, 0x02, 0x00, 0x01, 0x02, 0x02, 0x01, 0x00,
    /* Header Func Desc */
    0x05, 0x24, 0x00, 0x10, 0x01,
    /* Call Management Func Desc */
    0x05, 0x24, 0x01, 0x00, 0x01,
    /* ACM Func Desc */
    0x04, 0x24, 0x02, 0x02,
    /* Union Func Desc */
    0x05, 0x24, 0x06, 0x02, 0x03,
    /* EP Interrupt IN (通知) */
    0x07, 0x05, CDC_NOTIF_EP, 0x03,
    (uint8_t)(CDC_NOTIF_PACKET_SIZE & 0xFF),
    (uint8_t)((CDC_NOTIF_PACKET_SIZE >> 8) & 0xFF),
    0x10,

    /* Interface 3: Data */
    0x09, 0x04, 0x03, 0x00, 0x02, 0x0A, 0x00, 0x00, 0x00,
    /* EP Bulk OUT */
    0x07, 0x05, CDC_DATA_OUT_EP, 0x02,
    (uint8_t)(CDC_DATA_PACKET_SIZE & 0xFF),
    (uint8_t)((CDC_DATA_PACKET_SIZE >> 8) & 0xFF),
    0x00,
    /* EP Bulk IN */
    0x07, 0x05, CDC_DATA_IN_EP, 0x02,
    (uint8_t)(CDC_DATA_PACKET_SIZE & 0xFF),
    (uint8_t)((CDC_DATA_PACKET_SIZE >> 8) & 0xFF),
    0x00,
};

/* ==================== 调试辅助 ==================== */

#include "usb_debug.h"
#define DBG_STEP_DONE(s) do { usb_debug_set_step((s), USB_STATUS_OK); usb_debug_refresh(); delay(50); } while(0)

/* ==================== CDC 类回调 ==================== */

uint8_t cdc_class_init(void *pdev, uint8_t cfgidx)
{
    (void)cfgidx;
    USB_OTG_CORE_HANDLE *dev = (USB_OTG_CORE_HANDLE *)pdev;

    DCD_EP_Open(dev, CDC_NOTIF_EP, CDC_NOTIF_PACKET_SIZE, USB_OTG_EP_INT);
    DCD_EP_Open(dev, CDC_DATA_IN_EP, CDC_DATA_PACKET_SIZE, USB_OTG_EP_BULK);
    DCD_EP_Open(dev, CDC_DATA_OUT_EP, CDC_DATA_PACKET_SIZE, USB_OTG_EP_BULK);

    /* 准备接收第一个 OUT 包（使用专用缓冲区） */
    DCD_EP_PrepareRx(dev, CDC_DATA_OUT_EP, cdc_out_buf, CDC_DATA_PACKET_SIZE);

    g_cdc_pdev = (USB_OTG_CORE_HANDLE *)pdev;
    g_cdc_ready = true;

    return USBD_OK;
}

uint8_t cdc_class_deinit(void *pdev, uint8_t cfgidx)
{
    (void)cfgidx;
    USB_OTG_CORE_HANDLE *dev = (USB_OTG_CORE_HANDLE *)pdev;

    DCD_EP_Close(dev, CDC_NOTIF_EP);
    DCD_EP_Close(dev, CDC_DATA_IN_EP);
    DCD_EP_Close(dev, CDC_DATA_OUT_EP);

    g_cdc_ready = false;
    g_cdc_dtr = false;
    g_cdc_pdev = NULL;

    return USBD_OK;
}

uint8_t cdc_class_setup(void *pdev, USB_SETUP_REQ *req)
{
    USB_OTG_CORE_HANDLE *dev = (USB_OTG_CORE_HANDLE *)pdev;

    if ((req->bmRequest & USB_REQ_TYPE_MASK) == USB_REQ_TYPE_CLASS) {
        switch (req->bRequest) {

        case CDC_SET_LINE_CODING:
            if (req->wLength == 7) {
                DCD_EP_PrepareRx(dev, 0x00,
                                 (uint8_t*)&g_line_coding, 7);
            }
            return USBD_OK;

        case CDC_GET_LINE_CODING:
            USBD_CtlSendData(dev, (uint8_t*)&g_line_coding, 7);
            return USBD_OK;

        case CDC_SET_CONTROL_LINE_STATE:
            g_cdc_dtr = (req->wValue & 0x01) ? true : false;
            return USBD_OK;

        case CDC_SEND_BREAK:
            return USBD_OK;

        default:
            break;
        }
        return USBD_OK;
    }

    if ((req->bmRequest & USB_REQ_TYPE_MASK) == USB_REQ_TYPE_STANDARD) {
        switch (req->bRequest) {
        case USB_REQ_GET_INTERFACE: {
            uint8_t zero = 0;
            USBD_CtlSendData(dev, &zero, 1);
            return USBD_OK;
        }
        case USB_REQ_SET_INTERFACE:
            return USBD_OK;
        default:
            break;
        }
    }

    return USBD_FAIL;
}

uint8_t cdc_class_data_in(void *pdev, uint8_t epnum)
{
    (void)pdev;
    (void)epnum;
    return USBD_OK;
}

uint8_t cdc_class_data_out(void *pdev, uint8_t epnum)
{
    USB_OTG_CORE_HANDLE *dev = (USB_OTG_CORE_HANDLE *)pdev;

    /* 获取接收到的字节数 */
    uint32_t cnt = dev->dev.out_ep[epnum].xfer_count;

    /* 从专用缓冲区 cdc_out_buf 拷贝到环形 RX 缓冲区 */
    for (uint32_t i = 0; i < cnt; i++) {
        uint32_t next = (g_cdc_rx_head + 1) & (CDC_RX_BUF_SIZE - 1);
        if (next == g_cdc_rx_tail) {
            break;  /* 缓冲区满，丢弃剩余数据 */
        }
        g_cdc_rx_buf[g_cdc_rx_head] = cdc_out_buf[i];
        g_cdc_rx_head = next;
    }

    /* 重新准备接收下一个 OUT 包 */
    DCD_EP_PrepareRx(dev, CDC_DATA_OUT_EP, cdc_out_buf, CDC_DATA_PACKET_SIZE);

    return USBD_OK;
}

uint8_t cdc_class_sof(void *pdev)
{
    if (!g_cdc_dtr) return USBD_OK;

    uint32_t avail = (g_cdc_tx_head - g_cdc_tx_tail) & (CDC_TX_BUF_SIZE - 1);
    if (avail == 0) return USBD_OK;

    USB_OTG_CORE_HANDLE *dev = (USB_OTG_CORE_HANDLE *)pdev;
    uint32_t epnum = CDC_DATA_IN_EP & 0x7F;

    /* 检查是否有传输正在进行 */
    if (dev->dev.in_ep[epnum].xfer_len > 0) {
        return USBD_OK;
    }

    uint32_t to_send = (avail > CDC_DATA_PACKET_SIZE) ?
                        CDC_DATA_PACKET_SIZE : avail;

    /* 使用静态缓冲区避免栈溢出 */
    static uint8_t cdc_tmp_out[CDC_DATA_PACKET_SIZE];
    for (uint32_t i = 0; i < to_send; i++) {
        cdc_tmp_out[i] = g_cdc_tx_buf[g_cdc_tx_tail];
        g_cdc_tx_tail = (g_cdc_tx_tail + 1) & (CDC_TX_BUF_SIZE - 1);
    }

    DCD_EP_Tx(dev, CDC_DATA_IN_EP, cdc_tmp_out, to_send);
    return USBD_OK;
}

void cdc_class_prepare_rx(void *pdev)
{
    DCD_EP_PrepareRx((USB_OTG_CORE_HANDLE *)pdev, CDC_DATA_OUT_EP, cdc_out_buf, CDC_DATA_PACKET_SIZE);
}

/* ==================== 描述符回调函数 ==================== */

static uint8_t *cdc_get_cfg_desc(uint8_t speed, uint16_t *length)
{
    (void)speed;
    *length = USB_CDC_CONFIG_DESC_SIZ;
    return (uint8_t*)cdc_cfg_desc;
}

static uint8_t *cdc_get_other_cfg_desc(uint8_t speed, uint16_t *length)
{
    (void)speed;
    *length = USB_CDC_CONFIG_DESC_SIZ;
    return (uint8_t*)cdc_cfg_desc;
}

/* ==================== CDC 类回调结构体 ==================== */

static USBD_Class_cb_TypeDef cdc_class_cb = {
    NULL,  /* Init - 将在 usbd_cdc_reinit 中设置 */
};

static void cdc_setup_class_cb(void)
{
    /* 不能用 designated initializer（C++20 才支持），逐字段初始化 */
    memset(&cdc_class_cb, 0, sizeof(cdc_class_cb));
    cdc_class_cb.Init       = cdc_class_init;
    cdc_class_cb.DeInit     = cdc_class_deinit;
    cdc_class_cb.Setup      = cdc_class_setup;
    cdc_class_cb.EP0_TxSent = NULL;
    cdc_class_cb.EP0_RxReady = NULL;
    cdc_class_cb.DataIn     = cdc_class_data_in;
    cdc_class_cb.DataOut    = cdc_class_data_out;
    cdc_class_cb.SOF        = cdc_class_sof;
    cdc_class_cb.IsoINIncomplete  = NULL;
    cdc_class_cb.IsoOUTIncomplete = NULL;
    cdc_class_cb.GetConfigDescriptor     = cdc_get_cfg_desc;
    cdc_class_cb.GetOtherConfigDescriptor = cdc_get_other_cfg_desc;
}

/* ==================== 共享 BSP 函数 ==================== */
extern void hspi_gpio_init(void);
extern void hspi_clk_enable(void);
extern void hspi_phy_power_up(void);
extern void hspi_nvic_enable(void);

/* ==================== 独立 CDC 模式 reinit ==================== */

void usbd_cdc_reinit()
{
    /* ──── 阶段 0：时钟使能 + GPIO ──── */
    hspi_gpio_init();
    hspi_clk_enable();
    delay(10);
    DBG_STEP_DONE(USB_STEP_CLOCK);
    DBG_STEP_DONE(USB_STEP_GPIO);

    /* ──── 阶段 1：PHY 上电 ──── */
    hspi_phy_power_up();
    DBG_STEP_DONE(USB_STEP_PHY);

    /* 重置 CDC 全局状态 */
    g_cdc_ready = false;
    g_cdc_dtr = false;
    g_cdc_rx_head = 0;
    g_cdc_rx_tail = 0;
    g_cdc_tx_head = 0;
    g_cdc_tx_tail = 0;

    cdc_setup_class_cb();

    USB_OTG_dev.cfg.phy_itface = 1;
    USB_OTG_dev.dev.class_cb   = &cdc_class_cb;
    USB_OTG_dev.dev.usr_cb     = &USR_cb;
    USB_OTG_dev.dev.usr_device = &USR_desc;

    USB_OTG_dev.cfg.dma_enable = 0;
    DCD_Init(&USB_OTG_dev, USB_OTG_HS_CORE_ID);

    {
        volatile uint32_t *dctl = (volatile uint32_t *)(USB_OTG_HS_BASE + 0x804);
        *dctl |= (1u << 1);
        __asm volatile ("dsb");
    }
    DBG_STEP_DONE(USB_STEP_DCD);

    /* ──── 阶段 3：EP0 + 中断 ──── */
    DCD_EP_Open(&USB_OTG_dev, 0x00, 64, USB_OTG_EP_CONTROL);
    DCD_EP_Open(&USB_OTG_dev, 0x80, 64, USB_OTG_EP_CONTROL);
    DCD_EP_PrepareRx(&USB_OTG_dev, 0x00,
                     USB_OTG_dev.dev.setup_packet, 8);
    {
        volatile uint32_t *doepmsk  = (volatile uint32_t *)(USB_OTG_HS_BASE + 0x80C);
        volatile uint32_t *daintmsk = (volatile uint32_t *)(USB_OTG_HS_BASE + 0x818);
        *doepmsk  |= (1u << 3);
        *daintmsk |= (1u << 0) | (1u << 16);
        __asm volatile ("dsb");
    }

    {
        volatile uint32_t *gahbcfg = (volatile uint32_t *)(USB_OTG_HS_BASE + 0x008);
        *gahbcfg &= ~((1u << 5) | (0xFu << 1));
        *gahbcfg |=  (2u << 1);
        __asm volatile ("dsb");
    }

    /* ──── 阶段 4：GCCFG (PHY 唤醒 + VBUS 旁路) ──── */
    {
        volatile uint32_t *gccfg = (volatile uint32_t *)(USB_OTG_HS_BASE + 0x038);
        *gccfg &= ~(1u << 16);
        *gccfg |=  (1u << 21);
        *gccfg &= ~((1u << 19) | (1u << 18));
        __asm volatile ("dsb");
        delay(5);
    }
    DBG_STEP_DONE(USB_STEP_VBUS);

    /* ──── 阶段 5：速度 + FIFO ──── */
    USB_OTG_InitDevSpeed(&USB_OTG_dev, USB_OTG_SPEED_PARAM_HIGH);
    USB_OTG_dev.cfg.speed = USB_OTG_SPEED_HIGH;
    USB_OTG_dev.cfg.mps   = 512;

    /* FIFO: 128+64 = 192 words for EP0, then 64 for EP3 IN (CDC Notif), 128 for EP4 IN (CDC Data)
     * DIEPTXF[n] → IN EP (n+1): DIEPTXF[0]=EP1, [1]=EP2, [2]=EP3, [3]=EP4 */
    #define CDC_RX_FIFO_SIZE   128
    #define CDC_TX0_FIFO_SIZE  64
    #define CDC_TX1_FIFO_SIZE  64   /* EP3 IN 通知 (8B) */
    #define CDC_TX2_FIFO_SIZE  128  /* EP4 IN Bulk 数据 */

    USB_OTG_WRITE_REG32(&USB_OTG_dev.regs.GREGS->GRXFSIZ, CDC_RX_FIFO_SIZE);

    USB_OTG_FSIZ_TypeDef fifo;
    fifo.d32 = 0;
    fifo.b.depth     = CDC_TX0_FIFO_SIZE;
    fifo.b.startaddr = CDC_RX_FIFO_SIZE;
    USB_OTG_WRITE_REG32(&USB_OTG_dev.regs.GREGS->DIEPTXF0_HNPTXFSIZ, fifo.d32);  /* EP0 IN */

    /* EP1 IN (DIEPTXF[0]) — 未使用 */
    fifo.b.startaddr += fifo.b.depth;
    fifo.b.depth      = 0;
    USB_OTG_WRITE_REG32(&USB_OTG_dev.regs.GREGS->DIEPTXF[0], fifo.d32);

    /* EP2 IN (DIEPTXF[1]) — 未使用 */
    fifo.b.startaddr += fifo.b.depth;
    fifo.b.depth      = 0;
    USB_OTG_WRITE_REG32(&USB_OTG_dev.regs.GREGS->DIEPTXF[1], fifo.d32);

    /* EP3 IN (DIEPTXF[2]) — CDC Notif */
    fifo.b.startaddr += fifo.b.depth;
    fifo.b.depth      = CDC_TX1_FIFO_SIZE;
    USB_OTG_WRITE_REG32(&USB_OTG_dev.regs.GREGS->DIEPTXF[2], fifo.d32);

    /* EP4 IN (DIEPTXF[3]) — CDC Data */
    fifo.b.startaddr += fifo.b.depth;
    fifo.b.depth      = CDC_TX2_FIFO_SIZE;
    USB_OTG_WRITE_REG32(&USB_OTG_dev.regs.GREGS->DIEPTXF[3], fifo.d32);

    /* EP5+ (DIEPTXF[4+]) — 未使用 (EP5 是 OUT only) */
    fifo.b.startaddr += fifo.b.depth;
    fifo.b.depth = 0;
    USB_OTG_WRITE_REG32(&USB_OTG_dev.regs.GREGS->DIEPTXF[4], fifo.d32);
    DBG_STEP_DONE(USB_STEP_SPEED);

    /* ──── 阶段 6：GUSBCFG ──── */
    {
        volatile uint32_t *gusbcfg = (volatile uint32_t *)(USB_OTG_HS_BASE + GUSBCFG_OFFSET);
        *gusbcfg &= ~((1u << 6) | (1u << 17) | (1u << 22));
        *gusbcfg &= ~((1u << 21) | (1u << 20));
        *gusbcfg &= ~(1u << 30);
        __asm volatile ("dsb");
    }

    /* ──── 阶段 7：连接 ──── */
    USB_OTG_dev.dev.usr_cb->Init();
    hspi_nvic_enable();
    DCD_DevConnect(&USB_OTG_dev);
    delay(200);
    DBG_STEP_DONE(USB_STEP_CONNECT);
}

/* ==================== CDC 复合设备辅助函数 ==================== */

/*
 * 返回 CDC 接口描述符指针（供复合设备 memcpy 使用）
 */
const uint8_t* cdc_get_iface_desc(void)
{
    return cdc_iface_desc;
}

uint16_t cdc_get_iface_desc_size(void)
{
    return CDC_IFACE_DESC_SIZE;
}

/*
 * 设置复合模式下 CDC 类回调（在 usbd_msc.cpp 的 composite_cb 中使用）
 */
void cdc_set_class_callbacks(USBD_Class_cb_TypeDef *cb)
{
    /* 这些回调需要被复合设备的 composite_cb 调用 */
    /* 实际调用通过 CDC 类回调结构体转发 */
    /* 当前 CDC 类回调已通过 cdc_setup_class_cb 设置 */
    (void)cb;
}

/* ==================== CDC 公共 API ==================== */

bool usbd_cdc_is_ready(void)
{
    return g_cdc_ready && g_cdc_dtr
        && (USB_OTG_dev.dev.device_status == USB_OTG_CONFIGURED);
}

uint32_t cdc_write(const uint8_t *buf, uint32_t len)
{
    if (!buf || len == 0) return 0;

    uint32_t written = 0;
    for (uint32_t i = 0; i < len; i++) {
        uint32_t next = (g_cdc_tx_head + 1) & (CDC_TX_BUF_SIZE - 1);
        if (next == g_cdc_tx_tail) break;
        g_cdc_tx_buf[g_cdc_tx_head] = buf[i];
        g_cdc_tx_head = next;
        written++;
    }
    return written;
}

uint32_t cdc_write_byte(uint8_t c)
{
    return cdc_write(&c, 1);
}

uint32_t cdc_read(uint8_t *buf, uint32_t len)
{
    if (!buf || len == 0) return 0;

    uint32_t cnt = 0;
    while (cnt < len && g_cdc_rx_tail != g_cdc_rx_head) {
        buf[cnt++] = g_cdc_rx_buf[g_cdc_rx_tail];
        g_cdc_rx_tail = (g_cdc_rx_tail + 1) & (CDC_RX_BUF_SIZE - 1);
    }
    return cnt;
}

uint32_t cdc_available(void)
{
    return (g_cdc_rx_head - g_cdc_rx_tail) & (CDC_RX_BUF_SIZE - 1);
}

bool cdc_is_connected(void)
{
    return g_cdc_dtr;
}

void cdc_printf(const char *fmt, ...)
{
    char buf[256];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    cdc_write((uint8_t*)buf, strlen(buf));
}

#endif /* USB_MSC_ENABLE */
