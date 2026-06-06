/*
 * USB MSC (Mass Storage Class) BOT + SCSI 设备实现
 *
 * 硬件：STM32F405RGT6 + USB3300-EZK-TR (ULPI HS PHY) + USB Type-C
 * 使用 USB_OTG_HS 内核，通过 ULPI 8-bit 接口连接外部 PHY
 *
 * 存储后端：W25Q512JVEIQ SPI NOR Flash (64MB)
 *
 * 架构：
 *   W25Q512 → usb_manager (storage callbacks) → 本文件 (BOT引擎+SCSI处理器)
 *   USB3300 ← ULPI引脚 (本文件BSP) ← USB_OTG_HS内核 ← DCD层 ← 本文件 (MSC类回调)
 *
 * 关键实现细节：
 * - 核心库以FS模式编译，USB_OTG_CoreInit()中的ULPI初始化是运行时判断
 * - 在DCD_Init前预设phy_itface=1（USB_OTG_ULPI_PHY），核心即走ULPI路径
 * - BSP/NVIC/IRQ由本文件自行提供（因核心库中只有FS版本）
 */

#include "usbd_msc.h"
#include <string.h>
#include "usb_debug.h"

#if USB_MSC_ENABLE

#if HID_ENABLE
#include "usbd_hid.h"
#endif

#include "usbd_cdc.h"

/* ==================== 核心库头文件 ==================== */
extern "C" {
#include <STM32_USB_Device_Library/Core/inc/usbd_core.h>
#include <STM32_USB_Device_Library/Core/inc/usbd_ioreq.h>
#include <STM32_USB_Device_Library/Core/inc/usbd_usr.h>
#include <STM32_USB_OTG_Driver/inc/usb_dcd_int.h>
#include <STM32_USB_OTG_Driver/inc/usb_dcd.h>
#include <VCP/usbd_desc.h>
}

/* ==================== 端点定义（HS: 512B包） ==================== */
#define MSC_EP_IN   0x81   /* EP1 IN  — 批量传输 */
#define MSC_EP_OUT  0x01   /* EP1 OUT — 批量传输 */
#define MSC_EP_SIZE 512    /* HS最大包大小 */

/* ==================== BOT 协议常量 ==================== */
#define BOT_CBW_SIGNATURE  0x43425355UL
#define BOT_CSW_SIGNATURE  0x53425355UL
#define BOT_CBW_LENGTH     31
#define BOT_CSW_LENGTH     13

#define BOT_STATE_IDLE        0
#define BOT_STATE_DATA_OUT    1
#define BOT_STATE_DATA_IN     2
#define BOT_STATE_LAST_DATA_IN 3
#define BOT_STATE_SEND_CSW    4
#define BOT_STATE_ERROR       5

#define BOT_DIR_IN   0x80
#define BOT_DIR_OUT  0x00

#define CSW_CMD_PASSED   0x00
#define CSW_CMD_FAILED   0x01
#define CSW_PHASE_ERROR  0x02

/* ==================== BOT 数据结构 ==================== */

#pragma pack(push, 1)
typedef struct {
    uint32_t dSignature;
    uint32_t dTag;
    uint32_t dDataLength;
    uint8_t  bmFlags;
    uint8_t  bLUN;
    uint8_t  bCBLength;
    uint8_t  CB[16];
} MSC_BOT_CBW_TypeDef;

typedef struct {
    uint32_t dSignature;
    uint32_t dTag;
    uint32_t dDataResidue;
    uint8_t  bStatus;
} MSC_BOT_CSW_TypeDef;
#pragma pack(pop)

/* ==================== MSC 全局状态 ==================== */

static MSC_BOT_CBW_TypeDef bot_cbw;
static MSC_BOT_CSW_TypeDef bot_csw;
static uint8_t bot_state = BOT_STATE_IDLE;

static uint8_t scsi_sense_key  = 0;
static uint8_t scsi_sense_asc  = 0;
static uint8_t scsi_sense_ascq = 0;
static bool    scsi_cmd_failed = false;

/* 读写缓冲区（512B — 一个扇区） */
static uint8_t scsi_buf[512];
static uint32_t scsi_blk_addr;
static uint32_t scsi_blk_len;
static uint32_t scsi_data_len;

/* 默认INQUIRY数据 */
static uint8_t default_inquiry[36] = {
    0x00,                   /* 直接访问块设备 */
    0x80,                   /* 可移动介质 */
    0x04,                   /* SPC-2 */
    0x02,                   /* 响应格式 */
    36 - 5,                 /* 附加长度 */
    0x00, 0x00, 0x00,       /* 特性标志 */
    'W','o','u','o','U','I',' ',' ',              /* Vendor 8B */
    'W','2','5','Q','5','1','2',' ',' ',' ',' ',' ',' ',' ',' ',' ',  /* Product 16B */
    '1','.','0','0'                               /* Revision 4B */
};

/* 存储回调指针（用户代码赋值） */
USBD_STORAGE_cb_TypeDef *USBD_STORAGE_fops = NULL;

/* USB设备句柄（从核心extern获取） */
extern USB_OTG_CORE_HANDLE USB_OTG_dev;

/* ==================== 前向声明 ==================== */

static void bot_process_cbw(USB_OTG_CORE_HANDLE *pdev);
static void bot_abort(USB_OTG_CORE_HANDLE *pdev);
static void scsi_process_cmd(uint8_t lun, const uint8_t *cb, uint8_t cb_len, uint32_t *data_len);

/* ==================== BOT 引擎 ==================== */

static void bot_abort(USB_OTG_CORE_HANDLE *pdev)
{
    bot_state = BOT_STATE_ERROR;
    DCD_EP_Stall(pdev, MSC_EP_OUT);
    DCD_EP_Stall(pdev, MSC_EP_IN);
}

static void bot_send_csw(USB_OTG_CORE_HANDLE *pdev, uint8_t status)
{
    bot_csw.dSignature = BOT_CSW_SIGNATURE;
    bot_csw.dTag = bot_cbw.dTag;
    bot_csw.dDataResidue = bot_cbw.dDataLength;
    bot_csw.bStatus = status;
    bot_state = BOT_STATE_SEND_CSW;
    DCD_EP_Tx(pdev, MSC_EP_IN, (uint8_t *)&bot_csw, BOT_CSW_LENGTH);
}

static void bot_process_cbw(USB_OTG_CORE_HANDLE *pdev)
{
    uint32_t data_len = 0;

    /* 验证CBW */
    if (bot_cbw.dSignature != BOT_CBW_SIGNATURE ||
        bot_cbw.bCBLength > 16 ||
        bot_cbw.bLUN > 0)
    {
        bot_abort(pdev);
        return;
    }

    /* 处理SCSI命令 */
    scsi_cmd_failed = false;
    scsi_process_cmd(bot_cbw.bLUN, bot_cbw.CB, bot_cbw.bCBLength, &data_len);

    if (scsi_cmd_failed) {
        bot_send_csw(pdev, CSW_CMD_FAILED);
        return;
    }

    if (bot_state == BOT_STATE_ERROR) {
        bot_abort(pdev);
        return;
    }

    /* 判断数据传输方向 */
    if (data_len > 0 && bot_cbw.dDataLength > 0) {
        if (bot_cbw.bmFlags & BOT_DIR_IN) {
            uint32_t send_len = 512;
            if (data_len < send_len) send_len = data_len;
            if (send_len > bot_cbw.dDataLength) send_len = bot_cbw.dDataLength;

            if (scsi_blk_len > 0) {
                scsi_blk_len--;
                scsi_blk_addr++;
            }

            bot_cbw.dDataLength -= send_len;
            bot_state = BOT_STATE_DATA_IN;
            DCD_EP_Tx(pdev, MSC_EP_IN, scsi_buf, send_len);
        } else {
            bot_state = BOT_STATE_DATA_OUT;
            scsi_data_len = data_len;
            scsi_blk_len = (data_len + 511) / 512;
            DCD_EP_PrepareRx(pdev, MSC_EP_OUT, scsi_buf,
                (data_len > 512) ? 512 : data_len);
        }
    } else {
        bot_send_csw(pdev, CSW_CMD_PASSED);
    }
}

/* ==================== SCSI 命令处理 ==================== */

static void scsi_sense_save(uint8_t key, uint8_t asc, uint8_t ascq)
{
    scsi_sense_key  = key;
    scsi_sense_asc  = asc;
    scsi_sense_ascq = ascq;
    scsi_cmd_failed = true;
}

static void scsi_sense_fail(uint8_t key, uint8_t asc, uint8_t ascq)
{
    scsi_sense_save(key, asc, ascq);
    bot_state = BOT_STATE_ERROR;
}

static void scsi_process_cmd(uint8_t lun, const uint8_t *cb, uint8_t cb_len, uint32_t *data_len)
{
    (void)cb_len;
    *data_len = 0;

    if (USBD_STORAGE_fops == NULL) {
        scsi_sense_fail(0x02, 0x3A, 0x00);
        return;
    }

    switch (cb[0]) {

    case SCSI_TEST_UNIT_READY:
        if (USBD_STORAGE_fops->IsReady(lun) != 0) {
            scsi_sense_save(0x02, 0x3A, 0x00);
        }
        break;

    case SCSI_REQUEST_SENSE:
        memset(scsi_buf, 0, 18);
        scsi_buf[0] = 0x70;
        scsi_buf[2] = scsi_sense_key  ? scsi_sense_key  : 0x02;
        scsi_buf[7] = 10;
        scsi_buf[12] = scsi_sense_asc  ? scsi_sense_asc  : 0x3A;
        scsi_buf[13] = scsi_sense_ascq ? scsi_sense_ascq : 0x00;
        scsi_cmd_failed = false;
        *data_len = 18;
        break;

    case SCSI_INQUIRY: {
        uint8_t *inq = (USBD_STORAGE_fops->pInquiry) ?
                        (uint8_t *)USBD_STORAGE_fops->pInquiry : default_inquiry;
        uint32_t len = (cb[4] < 36) ? cb[4] : 36;
        memcpy(scsi_buf, inq, len);
        *data_len = len;
        break;
    }

    case SCSI_MODE_SENSE6:
        memset(scsi_buf, 0, 4);
        scsi_buf[0] = 3;
        if (USBD_STORAGE_fops->IsWriteProtected(lun)) {
            scsi_buf[2] = 0x80;
        }
        *data_len = 4;
        break;

    case SCSI_MODE_SENSE10:
        memset(scsi_buf, 0, 8);
        scsi_buf[0] = 0; scsi_buf[1] = 6;
        if (USBD_STORAGE_fops->IsWriteProtected(lun)) {
            scsi_buf[3] = 0x80;
        }
        *data_len = 8;
        break;

    case SCSI_READ_FORMAT_CAP: {
        uint32_t blk_num = 0, blk_size_val = 512;
        USBD_STORAGE_fops->GetCapacity(lun, &blk_num, &blk_size_val);
        uint32_t last_blk = blk_num - 1;
        memset(scsi_buf, 0, 12);
        scsi_buf[3] = 8;
        scsi_buf[4] = (uint8_t)(last_blk >> 24);
        scsi_buf[5] = (uint8_t)(last_blk >> 16);
        scsi_buf[6] = (uint8_t)(last_blk >> 8);
        scsi_buf[7] = (uint8_t)(last_blk);
        scsi_buf[8] = 0x02;
        scsi_buf[9]  = (uint8_t)(blk_size_val >> 16);
        scsi_buf[10] = (uint8_t)(blk_size_val >> 8);
        scsi_buf[11] = (uint8_t)(blk_size_val);
        *data_len = 12;
        break;
    }

    case SCSI_READ_CAPACITY10: {
        uint32_t blk_num = 0, blk_size = 512;
        if (USBD_STORAGE_fops->GetCapacity(lun, &blk_num, &blk_size) != 0) {
            scsi_sense_fail(0x02, 0x3A, 0x00);
            break;
        }
        uint32_t last_blk = blk_num - 1;
        scsi_buf[0] = (uint8_t)(last_blk >> 24);
        scsi_buf[1] = (uint8_t)(last_blk >> 16);
        scsi_buf[2] = (uint8_t)(last_blk >> 8);
        scsi_buf[3] = (uint8_t)(last_blk);
        scsi_buf[4] = (uint8_t)(blk_size >> 24);
        scsi_buf[5] = (uint8_t)(blk_size >> 16);
        scsi_buf[6] = (uint8_t)(blk_size >> 8);
        scsi_buf[7] = (uint8_t)(blk_size);
        *data_len = 8;
        break;
    }

    case SCSI_READ10: {
        uint32_t lba = ((uint32_t)cb[2] << 24) | ((uint32_t)cb[3] << 16) |
                        ((uint32_t)cb[4] << 8) | cb[5];
        uint16_t count = ((uint16_t)cb[7] << 8) | cb[8];
        if (count == 0) count = 1;

        scsi_blk_addr = lba;
        scsi_blk_len = count;

        if (USBD_STORAGE_fops->Read(lun, scsi_buf, lba, 1) != 0) {
            scsi_sense_fail(0x03, 0x11, 0x00);
            break;
        }
        *data_len = (uint32_t)count * 512;
        break;
    }

    case SCSI_WRITE10: {
        uint32_t lba = ((uint32_t)cb[2] << 24) | ((uint32_t)cb[3] << 16) |
                        ((uint32_t)cb[4] << 8) | cb[5];
        uint16_t count = ((uint16_t)cb[7] << 8) | cb[8];
        if (count == 0) count = 1;
        scsi_blk_addr = lba;
        *data_len = count * 512;
        break;
    }

    case SCSI_VERIFY10:
        break;

    case SCSI_PREVENT_ALLOW:
        break;

    case SCSI_START_STOP_UNIT:
        break;

    default:
        scsi_sense_fail(0x05, 0x20, 0x00);
        break;
    }
}

/* ==================== MSC 配置描述符（HS: 512B包大小） ==================== */

static uint8_t msc_cfg_desc[32] = {
    /* 配置描述符 */
    0x09,
    0x02,
    (uint8_t)32, 0x00,
    0x01,
    0x01,
    0x00,
    0xC0,
    0x32,

    /* 接口描述符 */
    0x09,
    0x04,
    0x00,
    0x00,
    0x02,
    0x08,
    0x06,
    0x50,
    0x00,

    /* Endpoint OUT */
    0x07,
    0x05,
    MSC_EP_OUT,
    0x02,
    (uint8_t)(MSC_EP_SIZE & 0xFF), (uint8_t)((MSC_EP_SIZE >> 8) & 0xFF),
    0x00,

    /* Endpoint IN */
    0x07,
    0x05,
    MSC_EP_IN,
    0x02,
    (uint8_t)(MSC_EP_SIZE & 0xFF), (uint8_t)((MSC_EP_SIZE >> 8) & 0xFF),
    0x00,
};

/* ==================== MSC 类回调 ==================== */

static uint8_t msc_init(void *pdev, uint8_t cfgidx)
{
    (void)cfgidx;
    USB_OTG_CORE_HANDLE *dev = (USB_OTG_CORE_HANDLE *)pdev;

    DCD_EP_Open(dev, MSC_EP_IN,  MSC_EP_SIZE, USB_OTG_EP_BULK);
    DCD_EP_Open(dev, MSC_EP_OUT, MSC_EP_SIZE, USB_OTG_EP_BULK);

    DCD_EP_PrepareRx(dev, MSC_EP_OUT, (uint8_t *)&bot_cbw, BOT_CBW_LENGTH);
    bot_state = BOT_STATE_IDLE;

    if (USBD_STORAGE_fops && USBD_STORAGE_fops->Init) {
        USBD_STORAGE_fops->Init(0);
    }

    return 0;
}

static uint8_t msc_deinit(void *pdev, uint8_t cfgidx)
{
    (void)cfgidx;
    USB_OTG_CORE_HANDLE *dev = (USB_OTG_CORE_HANDLE *)pdev;

    DCD_EP_Close(dev, MSC_EP_IN);
    DCD_EP_Close(dev, MSC_EP_OUT);
    bot_state = BOT_STATE_IDLE;

    return 0;
}

static uint8_t msc_setup(void *pdev, USB_SETUP_REQ *req)
{
    USB_OTG_CORE_HANDLE *dev = (USB_OTG_CORE_HANDLE *)pdev;

    switch (req->bmRequest & USB_REQ_TYPE_MASK) {
    case USB_REQ_TYPE_CLASS | USB_REQ_RECIPIENT_INTERFACE:
        if (req->wValue == 0xFF && req->wLength == 0) {
            /* BOT Reset */
            bot_state = BOT_STATE_IDLE;
            DCD_EP_Close(dev, MSC_EP_OUT);
            DCD_EP_Close(dev, MSC_EP_IN);
            DCD_EP_Open(dev, MSC_EP_OUT, MSC_EP_SIZE, USB_OTG_EP_BULK);
            DCD_EP_Open(dev, MSC_EP_IN, MSC_EP_SIZE, USB_OTG_EP_BULK);
            DCD_EP_PrepareRx(dev, MSC_EP_OUT, (uint8_t *)&bot_cbw, BOT_CBW_LENGTH);
            return USBD_OK;
        }
        if (req->wValue == 0xFE && req->wLength == 1) {
            /* Get Max LUN */
            uint8_t max_lun = 0;
            if (USBD_STORAGE_fops) {
                max_lun = (uint8_t)USBD_STORAGE_fops->GetMaxLun();
            }
            USBD_CtlSendData(dev, &max_lun, 1);
            return USBD_OK;
        }
        break;
    }
    return USBD_FAIL;
}

static uint8_t msc_data_in(void *pdev, uint8_t epnum)
{
    (void)epnum;
    USB_OTG_CORE_HANDLE *dev = (USB_OTG_CORE_HANDLE *)pdev;

    switch (bot_state) {
    case BOT_STATE_SEND_CSW:
    case BOT_STATE_ERROR:
        bot_state = BOT_STATE_IDLE;
        DCD_EP_PrepareRx(dev, MSC_EP_OUT, (uint8_t *)&bot_cbw, BOT_CBW_LENGTH);
        break;

    case BOT_STATE_DATA_IN:
    case BOT_STATE_LAST_DATA_IN:
        if (bot_state == BOT_STATE_LAST_DATA_IN) {
            bot_send_csw(dev, CSW_CMD_PASSED);
        } else {
            uint32_t remaining = (scsi_blk_len > 0) ? scsi_blk_len * 512UL : 0;
            if (remaining > 0 && USBD_STORAGE_fops) {
                USBD_STORAGE_fops->Read(0, scsi_buf, scsi_blk_addr, 1);
                scsi_blk_addr++;
                scsi_blk_len--;
                DCD_EP_Tx(dev, MSC_EP_IN, scsi_buf, 512);
            } else {
                bot_send_csw(dev, CSW_CMD_PASSED);
            }
        }
        break;

    default:
        break;
    }
    return 0;
}

static uint8_t msc_data_out(void *pdev, uint8_t epnum)
{
    (void)epnum;
    USB_OTG_CORE_HANDLE *dev = (USB_OTG_CORE_HANDLE *)pdev;

    switch (bot_state) {
    case BOT_STATE_IDLE:
        bot_process_cbw(dev);
        break;

    case BOT_STATE_DATA_OUT:
        if (USBD_STORAGE_fops && USBD_STORAGE_fops->Write) {
            USBD_STORAGE_fops->Write(0, scsi_buf, scsi_blk_addr, 1);
            scsi_blk_addr++;
            scsi_blk_len--;
        }
        if (scsi_blk_len > 0) {
            uint16_t len = (scsi_blk_len > 1) ? 512 : (scsi_data_len % 512);
            if (len == 0) len = 512;
            DCD_EP_PrepareRx(dev, MSC_EP_OUT, scsi_buf, len);
        } else {
            bot_send_csw(dev, CSW_CMD_PASSED);
        }
        break;

    default:
        break;
    }
    return 0;
}

static uint8_t *msc_get_cfg_desc(uint8_t speed, uint16_t *length)
{
    (void)speed;
    *length = sizeof(msc_cfg_desc);
    return msc_cfg_desc;
}

static uint8_t *msc_get_other_cfg_desc(uint8_t speed, uint16_t *length)
{
    (void)speed;
    *length = sizeof(msc_cfg_desc);
    return msc_cfg_desc;
}

/* ==================== MSC 类回调结构体（12成员，用于HS） ==================== */

USBD_Class_cb_TypeDef MSC_cb = {
    msc_init,
    msc_deinit,
    msc_setup,
    NULL,
    NULL,
    msc_data_in,
    msc_data_out,
    NULL,
    NULL,
    NULL,
    msc_get_cfg_desc,
    msc_get_other_cfg_desc,
};

/* ==================== W25Q512 SPI Flash 驱动 ==================== */

/* 上次检测失败的 JEDEC ID（调试用） */
static char w25q_last_id_str[32] = "";

/* SPI1 寄存器基址（APB2，STM32F405） */
#define SPI1_BASE       0x40013000UL
#define SPI1_CR1        (*(volatile uint32_t *)(SPI1_BASE + 0x00))
#define SPI1_SR         (*(volatile uint32_t *)(SPI1_BASE + 0x08))
#define SPI1_DR         (*(volatile uint32_t *)(SPI1_BASE + 0x0C))
#define SPI1_CRCPR      (*(volatile uint32_t *)(SPI1_BASE + 0x10))

#define SPI_SR_TXE      (1u << 1)
#define SPI_SR_RXNE     (1u << 0)
#define SPI_SR_BSY      (1u << 7)
#define SPI_CR1_SPE     (1u << 6)
#define SPI_CR1_MSTR    (1u << 2)
#define SPI_CR1_SSM     (1u << 9)
#define SPI_CR1_SSI     (1u << 8)
#define SPI_CR1_BR_SHIFT 3u
#define SPI_CR1_BR_MASK  (7u << 3)

/* RCC 外设时钟使能 */
#define RCC_APB2ENR       (*(volatile uint32_t *)(RCC_BASE + 0x44))
#define RCC_APB2ENR_SPI1  (1u << 12)

/* 单字节 SPI1 收发（轮询，阻塞） */
static uint8_t spi1_xfer(uint8_t tx)
{
    while (!(SPI1_SR & SPI_SR_TXE)) {}
    SPI1_DR = (uint32_t)tx;
    while (!(SPI1_SR & SPI_SR_RXNE)) {}
    return (uint8_t)(SPI1_DR & 0xFF);
}

static uint8_t  sector_cache[4096];
static int32_t  cached_sector = -1;

#define W25Q_CMD_WRITE_ENABLE  0x06
#define W25Q_CMD_WRITE_DISABLE 0x04
#define W25Q_CMD_READ_STATUS   0x05
#define W25Q_CMD_READ_DATA     0x03
#define W25Q_CMD_PAGE_PROGRAM  0x02
#define W25Q_CMD_SECTOR_ERASE  0x20
#define W25Q_CMD_JEDEC_ID      0x9F

static void w25q_cs_low(void)  { digitalWrite(W25Q_CS_PIN, LOW); }
static void w25q_cs_high(void) { digitalWrite(W25Q_CS_PIN, HIGH); }

static void w25q_spi_tx(const uint8_t *data, uint32_t len)
{
    for (uint32_t i = 0; i < len; i++) { spi1_xfer(data[i]); }
}

static void w25q_spi_rx(uint8_t *data, uint32_t len)
{
    for (uint32_t i = 0; i < len; i++) { data[i] = spi1_xfer(0xFF); }
}

static uint8_t w25q_read_status(void)
{
    uint8_t cmd = W25Q_CMD_READ_STATUS, status;
    w25q_cs_low();
    spi1_xfer(cmd);
    status = spi1_xfer(0xFF);
    w25q_cs_high();
    return status;
}

static void w25q_wait_busy(void)
{
    while (w25q_read_status() & 0x01) {}
}

static void w25q_write_enable(void)
{
    uint8_t cmd = W25Q_CMD_WRITE_ENABLE;
    w25q_cs_low();
    spi1_xfer(cmd);
    w25q_cs_high();
}

bool w25q_init(void)
{
    /* 本地常量（避免依赖后续 BSP 段的宏定义） */
    #define _REG32(addr)    (*(volatile uint32_t *)(addr))
    #define _RCC_AHB1ENR    _REG32(0x40023800UL + 0x30)
    #define _RCC_APB2ENR    _REG32(0x40023800UL + 0x44)
    #define _GPIOA_BASE     0x40020000UL
    #define _GPIOB_BASE     0x40020400UL
    #define _GPIO_MODER(g)  _REG32((g) + 0x00u)
    #define _GPIO_OSPEEDR(g) _REG32((g) + 0x08u)
    #define _GPIO_PUPDR(g)  _REG32((g) + 0x0Cu)
    #define _GPIO_AFRL(g)   _REG32((g) + 0x20u)

    uint32_t cr1;

    /* ── 1. 时钟使能 ── */
    _RCC_AHB1ENR |= (1u << 0) | (1u << 1);     /* GPIOA + GPIOB 时钟 */
    _RCC_APB2ENR |= (1u << 12);                  /* SPI1 时钟 (APB2 bit12) */
    __asm volatile ("dsb");

    /* ── 2. CS 引脚 (PA4) ── */
    _GPIO_MODER(_GPIOA_BASE)   &= ~(0x3u << 8);    /* PA4 通用输出 */
    _GPIO_MODER(_GPIOA_BASE)   |=  (0x1u << 8);
    _GPIO_OSPEEDR(_GPIOA_BASE) &= ~(0x3u << 8);
    _GPIO_OSPEEDR(_GPIOA_BASE) |=  (0x2u << 8);    /* 50MHz */
    _GPIO_PUPDR(_GPIOA_BASE)   &= ~(0x3u << 8);    /* 无上拉 */
    digitalWrite(W25Q_CS_PIN, HIGH);

    /* ── 3. SPI 引脚：PB3(SCK), PA6(MISO), PA7(MOSI) → AF5 ──
     * 注意：PA5 已经是 ULPI_CLK(AF10)，不触碰 */
    /* PB3=SPI1_SCK */
    _GPIO_MODER(_GPIOB_BASE)   &= ~(0x3u << 6);
    _GPIO_MODER(_GPIOB_BASE)   |=  (0x2u << 6);     /* MODER=AF */
    _GPIO_OSPEEDR(_GPIOB_BASE) &= ~(0x3u << 6);
    _GPIO_OSPEEDR(_GPIOB_BASE) |=  (0x2u << 6);     /* 50MHz */
    _GPIO_PUPDR(_GPIOB_BASE)   &= ~(0x3u << 6);      /* 无上拉 */
    _GPIO_AFRL(_GPIOB_BASE)    &= ~(0xFu << 12);     /* PB3 AFRL位[15:12] */
    _GPIO_AFRL(_GPIOB_BASE)    |=  (5u << 12);       /* AF5 */

    /* PA6=SPI1_MISO */
    _GPIO_MODER(_GPIOA_BASE)   &= ~(0x3u << 12);
    _GPIO_MODER(_GPIOA_BASE)   |=  (0x2u << 12);
    _GPIO_OSPEEDR(_GPIOA_BASE) &= ~(0x3u << 12);
    _GPIO_OSPEEDR(_GPIOA_BASE) |=  (0x2u << 12);
    _GPIO_PUPDR(_GPIOA_BASE)   &= ~(0x3u << 12);
    _GPIO_AFRL(_GPIOA_BASE)    &= ~(0xFu << 24);     /* PA6 AFRL位[27:24] */
    _GPIO_AFRL(_GPIOA_BASE)    |=  (5u << 24);       /* AF5 */

    /* PA7=SPI1_MOSI */
    _GPIO_MODER(_GPIOA_BASE)   &= ~(0x3u << 14);
    _GPIO_MODER(_GPIOA_BASE)   |=  (0x2u << 14);
    _GPIO_OSPEEDR(_GPIOA_BASE) &= ~(0x3u << 14);
    _GPIO_OSPEEDR(_GPIOA_BASE) |=  (0x2u << 14);
    _GPIO_PUPDR(_GPIOA_BASE)   &= ~(0x3u << 14);
    _GPIO_AFRL(_GPIOA_BASE)    &= ~(0xFu << 28);     /* PA7 AFRL位[31:28] */
    _GPIO_AFRL(_GPIOA_BASE)    |=  (5u << 28);       /* AF5 */

    __asm volatile ("dsb");

    /* ── 4. 配置 SPI1 寄存器 ── */
    SPI1_CR1 = 0;                                         /* 先关 SPE */
    cr1  = SPI_CR1_MSTR;                                  /* 主机模式 */
    cr1 |= (3u << SPI_CR1_BR_SHIFT);                      /* BR=011 → fPCLK/16 ≈ 5.25MHz */
    cr1 |= SPI_CR1_SSM | SPI_CR1_SSI;                     /* 软件 NSS */
    /* CPOL=0, CPHA=0 → SPI mode 0 */
    cr1 |= SPI_CR1_SPE;                                   /* SPI 使能 */
    SPI1_CR1 = cr1;
    __asm volatile ("dsb");
    delay(1);

    /* ── 5. 读取 JEDEC ID ── */
    uint8_t cmd = W25Q_CMD_JEDEC_ID;
    uint8_t id[3];
    uint32_t jedec_id;

    /* 先试 mode 0 */
    w25q_cs_low();
    w25q_spi_tx(&cmd, 1);
    w25q_spi_rx(id, 3);
    w25q_cs_high();

    jedec_id = ((uint32_t)id[0] << 16) | ((uint32_t)id[1] << 8) | id[2];

    /* 接受 W25Q512 的两种常见 JEDEC ID */
    if (id[0] == 0xEF && (
        (id[1] == 0x40 && id[2] == 0x1A) ||   /* W25Q512JV: 0xEF401A */
        (id[1] == 0x40 && id[2] == 0x20)       /* W25Q512  variant: 0xEF4020 */
    )) {
        cached_sector = -1;
        return true;
    }

    /* 若失败，试 mode 3 (CPOL=1, CPHA=1) */
    cr1 = SPI1_CR1;
    cr1 |= (1u << 0) | (1u << 1);   /* CPHA=1, CPOL=1 */
    SPI1_CR1 = cr1;
    __asm volatile ("dsb");
    delay(1);

    w25q_cs_low();
    w25q_spi_tx(&cmd, 1);
    w25q_spi_rx(id, 3);
    w25q_cs_high();

    jedec_id = ((uint32_t)id[0] << 16) | ((uint32_t)id[1] << 8) | id[2];

    if (id[0] == 0xEF && (
        (id[1] == 0x40 && id[2] == 0x1A) ||
        (id[1] == 0x40 && id[2] == 0x20)
    )) {
        cached_sector = -1;
        return true;
    }

    /* 在屏幕底部显示实际读到的 ID，方便调试 */
    {
        char msg[32];
        static const char hex[] = "0123456789ABCDEF";
        msg[0] = 'I'; msg[1] = 'D'; msg[2] = ':'; msg[3] = ' ';
        msg[4] = '0'; msg[5] = 'x';
        msg[6] = hex[(jedec_id >> 20) & 0x0F];
        msg[7] = hex[(jedec_id >> 16) & 0x0F];
        msg[8] = hex[(jedec_id >> 12) & 0x0F];
        msg[9] = hex[(jedec_id >> 8) & 0x0F];
        msg[10] = hex[(jedec_id >> 4) & 0x0F];
        msg[11] = hex[jedec_id & 0x0F];
        msg[12] = '\0';
        /* 保存到静态变量，供最终错误页使用 */
        strncpy(w25q_last_id_str, msg, sizeof(w25q_last_id_str) - 1);
        w25q_last_id_str[sizeof(w25q_last_id_str) - 1] = '\0';
        usb_debug_set_msg(msg);
        usb_debug_refresh();
        delay(500);
    }

    return false;  /* W25Q512 未检测到 */
}

const char* w25q_get_last_id_str(void)
{
    return w25q_last_id_str;
}

void w25q_read(uint32_t addr, uint8_t *buf, uint32_t len)
{
    uint8_t cmd[4];
    cmd[0] = W25Q_CMD_READ_DATA;
    cmd[1] = (uint8_t)(addr >> 16);
    cmd[2] = (uint8_t)(addr >> 8);
    cmd[3] = (uint8_t)(addr);

    w25q_cs_low();
    w25q_spi_tx(cmd, 4);
    w25q_spi_rx(buf, len);
    w25q_cs_high();
}

void w25q_read_block(uint32_t blk_addr, uint8_t *buf)
{
    w25q_read((uint32_t)blk_addr * 512, buf, 512);
}

static void w25q_flush_sector(int32_t sector)
{
    if (sector < 0) return;

    uint32_t addr = (uint32_t)sector * 4096;

    /* 擦除4KB */
    w25q_write_enable();
    uint8_t erase_cmd[4];
    erase_cmd[0] = W25Q_CMD_SECTOR_ERASE;
    erase_cmd[1] = (uint8_t)(addr >> 16);
    erase_cmd[2] = (uint8_t)(addr >> 8);
    erase_cmd[3] = (uint8_t)(addr);
    w25q_cs_low();
    w25q_spi_tx(erase_cmd, 4);
    w25q_cs_high();
    w25q_wait_busy();

    /* 逐页写入（16页 × 256B = 4096B） */
    for (int page = 0; page < 16; page++) {
        w25q_write_enable();
        uint32_t page_addr = addr + page * 256;
        uint8_t prog_cmd[4];
        prog_cmd[0] = W25Q_CMD_PAGE_PROGRAM;
        prog_cmd[1] = (uint8_t)(page_addr >> 16);
        prog_cmd[2] = (uint8_t)(page_addr >> 8);
        prog_cmd[3] = (uint8_t)(page_addr);
        w25q_cs_low();
        w25q_spi_tx(prog_cmd, 4);
        w25q_spi_tx(&sector_cache[page * 256], 256);
        w25q_cs_high();
        w25q_wait_busy();
    }
}

int8_t w25q_write_block(uint32_t blk_addr, const uint8_t *buf)
{
    uint32_t byte_addr = (uint32_t)blk_addr * 512;
    int32_t  sector = (int32_t)(byte_addr / 4096);
    uint32_t offset = byte_addr % 4096;

    if (cached_sector != sector) {
        if (cached_sector >= 0) {
            w25q_flush_sector(cached_sector);
        }
        w25q_read((uint32_t)sector * 4096, sector_cache, 4096);
        cached_sector = sector;
    }

    memcpy(&sector_cache[offset], buf, 512);
    w25q_flush_sector(sector);
    cached_sector = -1;

    return 0;
}

/* ==================== HS ULPI BSP 初始化 ==================== */

/* 通用 GPIO 寄存器访问宏 */
#define REG32(addr)     (*(volatile uint32_t *)(addr))
#define GPIO_MODER(g)   REG32((g) + 0x00u)
#define GPIO_OTYPER(g)  REG32((g) + 0x04u)
#define GPIO_OSPEEDR(g) REG32((g) + 0x08u)
#define GPIO_PUPDR(g)   REG32((g) + 0x0Cu)
#define GPIO_AFRL(g)    REG32((g) + 0x20u)
#define GPIO_AFRH(g)    REG32((g) + 0x24u)

#define RCC_BASE              0x40023800UL
#define RCC_AHB1ENR           (*(volatile uint32_t *)(RCC_BASE + 0x30))
#define RCC_AHB1ENR_OTGHSEN   (1UL << 29)
#define RCC_AHB1ENR_OTGHSULPIEN (1UL << 30)
#define RCC_AHB1LPENR         (*(volatile uint32_t *)(RCC_BASE + 0x50))

/* USB_OTG_HS 寄存器基址 */
#define USB_OTG_HS_BASE       0x40040000UL
#define GUSBCFG_OFFSET        0x00CU

#define HSPI_AF  GPIO_AFMODE_OTG_FS  /* =10 = AF10 = OTG_HS ULPI (与 hardware_early_init 中 ULPI_AF_VAL 保持一致) */

void hspi_gpio_init(void)
{
    const gpio_pin_mode af_mode = (gpio_pin_mode)(GPIO_MODE_AF | GPIO_OTYPE_PP | GPIO_OSPEED_100MHZ);

    gpio_set_mode(ULPI_D0_PIN, af_mode);
    gpio_set_mode(ULPI_D1_PIN, af_mode);
    gpio_set_mode(ULPI_D2_PIN, af_mode);
    gpio_set_mode(ULPI_D3_PIN, af_mode);
    gpio_set_mode(ULPI_D4_PIN, af_mode);
    gpio_set_mode(ULPI_D5_PIN, af_mode);
    gpio_set_mode(ULPI_D6_PIN, af_mode);
    gpio_set_mode(ULPI_D7_PIN, af_mode);
    gpio_set_mode(ULPI_CLK_PIN, af_mode);
    gpio_set_mode(ULPI_STP_PIN, af_mode);
    gpio_set_mode(ULPI_DIR_PIN, af_mode);
    gpio_set_mode(ULPI_NXT_PIN, af_mode);

    gpio_set_af_mode(ULPI_D0_PIN,  HSPI_AF);
    gpio_set_af_mode(ULPI_D1_PIN,  HSPI_AF);
    gpio_set_af_mode(ULPI_D2_PIN,  HSPI_AF);
    gpio_set_af_mode(ULPI_D3_PIN,  HSPI_AF);
    gpio_set_af_mode(ULPI_D4_PIN,  HSPI_AF);
    gpio_set_af_mode(ULPI_D5_PIN,  HSPI_AF);
    gpio_set_af_mode(ULPI_D6_PIN,  HSPI_AF);
    gpio_set_af_mode(ULPI_D7_PIN,  HSPI_AF);
    gpio_set_af_mode(ULPI_CLK_PIN, HSPI_AF);
    gpio_set_af_mode(ULPI_STP_PIN, HSPI_AF);
    gpio_set_af_mode(ULPI_DIR_PIN, HSPI_AF);
    gpio_set_af_mode(ULPI_NXT_PIN, HSPI_AF);
}

static void hspi_gpio_deinit(void)
{
    gpio_set_mode(ULPI_D0_PIN,  (gpio_pin_mode)GPIO_MODE_INPUT);
    gpio_set_mode(ULPI_D1_PIN,  (gpio_pin_mode)GPIO_MODE_INPUT);
    gpio_set_mode(ULPI_D2_PIN,  (gpio_pin_mode)GPIO_MODE_INPUT);
    gpio_set_mode(ULPI_D3_PIN,  (gpio_pin_mode)GPIO_MODE_INPUT);
    gpio_set_mode(ULPI_D4_PIN,  (gpio_pin_mode)GPIO_MODE_INPUT);
    gpio_set_mode(ULPI_D5_PIN,  (gpio_pin_mode)GPIO_MODE_INPUT);
    gpio_set_mode(ULPI_D6_PIN,  (gpio_pin_mode)GPIO_MODE_INPUT);
    gpio_set_mode(ULPI_D7_PIN,  (gpio_pin_mode)GPIO_MODE_INPUT);
    gpio_set_mode(ULPI_CLK_PIN, (gpio_pin_mode)GPIO_MODE_INPUT);
    gpio_set_mode(ULPI_STP_PIN, (gpio_pin_mode)GPIO_MODE_INPUT);
    gpio_set_mode(ULPI_DIR_PIN, (gpio_pin_mode)GPIO_MODE_INPUT);
    gpio_set_mode(ULPI_NXT_PIN, (gpio_pin_mode)GPIO_MODE_INPUT);
}

void hspi_clk_enable(void)
{
    /* 使能 OTG_HS + ULPI 时钟
     * 注意：不使用 RCC_AHB1RSTR 硬复位（会打断 ULPI PHY 通信），
     *       DCD_Init 内部通过 GRSTCTL.CSRST 做软复位即可 */
    RCC_AHB1ENR |= RCC_AHB1ENR_OTGHSEN | RCC_AHB1ENR_OTGHSULPIEN;
    __asm volatile ("dsb");
    __asm volatile ("isb");

    /* 使能睡眠模式时钟（确保 USB 唤醒可用） */
    RCC_AHB1LPENR |= RCC_AHB1ENR_OTGHSEN | RCC_AHB1ENR_OTGHSULPIEN;
}

static void hspi_clk_disable(void)
{
    /* 先断开DP上拉，让主机检测到断开 */
    USB_OTG_dev.regs.DREGS->DCTL |= (1U << 1);  /* DCTL.SDIS = 1 */

    RCC_AHB1LPENR &= ~(RCC_AHB1ENR_OTGHSEN | RCC_AHB1ENR_OTGHSULPIEN);
    RCC_AHB1ENR &= ~(RCC_AHB1ENR_OTGHSEN | RCC_AHB1ENR_OTGHSULPIEN);
    __asm volatile ("dsb");
}

/* ==================== PHY 上电 ==================== */

/*
 * 唤醒 USB3300 ULPI PHY + 配置 VBUS 检测
 *
 * GCCFG 寄存器 (offset 0x038):
 *   bit 16: PWRDWN — PHY 掉电
 *   bit 19: VBUSBSEN — VBUS B-device 检测使能
 *   bit 21: NOVBUSSENS — 禁用 VBUS 检测
 *
 * USB3300 通过 ULPI 接口提供 VBUS 有效信号，
 * 因此设置 VBUSBSEN=1 让核心使用 ULPI VBUS 指示，
 * 同时清除 PWRDWN 确保 ULPI 接口唤醒。
 */
void hspi_phy_power_up(void)
{
    volatile uint32_t *gccfg = (volatile uint32_t *)(USB_OTG_HS_BASE + 0x038);
    /* 清除 PWRDWN (bit 16) 唤醒 PHY
     * NOVBUSSENS=1 让核心内部认为 VBUS 始终有效，
     * 避免 USB3300 ULPI VBUS 检测延迟导致会话无效 →
     * 核心忽略主机 setup 包 → code 43。
     * 此配置与 HAL 参考项目 vbus_sensing_enable=DISABLE 一致 */
    *gccfg &= ~(1u << 16);                    /* PWRDWN = 0: 唤醒 PHY */
    *gccfg |=  (1u << 21);                    /* NOVBUSSENS = 1: 旁路 VBUS 检测 */
    *gccfg &= ~((1u << 19) | (1u << 18));     /* VBUSBSEN=0, VBUSASEN=0 */
    __asm volatile ("dsb");
    delay(5);
}

/* ==================== HS NVIC / 中断 ==================== */

void hspi_nvic_enable(void)
{
    nvic_irq_enable(NVIC_USB_HS);
    nvic_irq_set_priority(NVIC_USB_HS, 6);
}

static void hspi_nvic_disable(void)
{
    nvic_irq_disable(NVIC_USB_HS);
}

extern "C" void __irq_usb_hs(void)
{
    USBD_OTG_ISR_Handler(&USB_OTG_dev);
}

/* ==================== 公共接口 ==================== */

void usbd_msc_reinit()
{
    #define DBG_STEP_DONE(s) do { usb_debug_set_step((s), USB_STATUS_OK); usb_debug_refresh(); delay(50); } while(0)

    /* ──── 阶段 0：时钟使能 + GPIO ──── */
    hspi_gpio_init();
    hspi_clk_enable();
    delay(10);
    usb_debug_set_step(USB_STEP_CLOCK, USB_STATUS_OK);
    usb_debug_set_step(USB_STEP_GPIO, USB_STATUS_OK);
    usb_debug_refresh();
    delay(50);

    /* ──── 阶段 1：PHY 上电 ──── */
    hspi_phy_power_up();
    DBG_STEP_DONE(USB_STEP_PHY);

    /* ──── 阶段 2：核心库初始化 ────
     * phy_itface=1=USB_OTG_ULPI_PHY, 配合 usb_conf.h 中的
     * USB_OTG_HS_CORE + USB_OTG_ULPI_PHY_ENABLED + USB_OTG_HS_INTERNAL_DMA_ENABLED
     * DCD_Init 内部链路自动完成:
     *   CoreInit:   GUSBCFG.physel=0, GUSBCFG.ulpi_fsls=0, GCCFG.pwdn=0,
     *               GAHBCFG.DMAEN=1, hburstlen=5
     *   CoreInitDev: DCFG.devspd=HIGH, HS FIFO 配置, DTHRCTL(DMA阈值)
     *   EnableDevInt: DMA 模式中断(无 rxstsqlvl)
     *
     * 注意：CoreInit → CoreReset 会清除所有寄存器（包括 SDIS），
     * 因此 DCD_Init 返回后 SDIS=0（设备已连接）。必须立即重新设
     * SDIS=1，确保在全部配置完成之前保持断开。 */
    USB_OTG_dev.cfg.phy_itface = 1;

    /* 冷启动时跳过 USBD_DeInit（USB 从未初始化） */
    USB_OTG_dev.dev.class_cb = (USBD_Class_cb_TypeDef *)&MSC_cb;
    USB_OTG_dev.dev.usr_cb = &USR_cb;
    USB_OTG_dev.dev.usr_device = &USR_desc;

    /* 关键：在 DCD_Init 前清空 dma_enable
     * STM32_USB_Device_Library 默认设计为 SLAVE 模式（dma_enable=0），
     * 该模式使用 rxstsqlvl / TxFIFOEmpty 中断手动管理 FIFO。
     * Arduino 原版 VCP 库也使用 SLAVE 模式运行。
     *
     * DMA 模式 (dma_enable=1) 与该库的 ISR 处理流存在多处不兼容：
     *   1. CoreInitDev 在 dma_enable=0 时跳过 DTHRCTL 配置
     *   2. EnableDevInt 在 DMA=0 时开启 rxstsqlvl，DMA=1 时依赖 xfercompl
     *   3. EP0 的 USB_OTG_EP0_OutStart 在 DMA 下使用硬编码 DOEPCTL=0x80008000
     *   4. STATUS_OUT 阶段 DCD_EP_PrepareRx(NULL,0) 导致 DOEPDMA=0
     *
     * SLAVE 模式通过 PIO 方式读写 FIFO，由 rxstsqlvl / TxFIFOEmpty 中断驱动，
     * 完全兼容该库的 ISR 处理逻辑。HS 速度下的控制传输枚举在 FS 速度(12Mbps)
     * 进行，PIO 方式完全满足带宽需求。 */
    USB_OTG_dev.cfg.dma_enable = 0;

    DCD_Init(&USB_OTG_dev, USB_OTG_HS_CORE_ID);

    /* USB_OTG_SelectCore（在 DCD_Init 内部）无条件将 dma_enable 重置为 0。
     * 保持 SLAVE 模式。 */

    /* DCD_Init → CoreReset 清除了 DCTL.SDIS，必须在此处重新设
     * SDIS=1 确保设备断开。阶段 2.5~4 期间保持断开，避免主机在
     * EP0 未就绪时发送 setup 包 → code 43。
     * SDIS 将在阶段 5 的 DCD_DevConnect 中清除。 */
    {
        volatile uint32_t *dctl = (volatile uint32_t *)(USB_OTG_HS_BASE + 0x804);
        *dctl |= (1u << 1);  /* DCTL.SDIS = 1: 软断开 */
        __asm volatile ("dsb");
    }

    DBG_STEP_DONE(USB_STEP_DCD);

    /* ──── 阶段 2.5：EP0 初始化（修复 code 43 的关键） ────
     * USBD_Init() 未被调用，导致以下 EP0 配置被跳过：
     *   1. DCD_EP_Open(EP0 OUT 0x00) + DCD_EP_Open(EP0 IN 0x80)
     *   2. DCD_EP_PrepareRx(EP0, setup_packet_buf, 8)  → 设置 DOEPDMA0
     *   3. DOEPMSK.STUP 位使能 — 允许 setup 包中断
     *   4. DAINTMSK.EP0 — 在设备级中断使能 EP0
     *
     * 缺少这些配置：
     *   → 主机发送 8 字节设备描述符请求到 EP0
     *   → DMA 核心收到 packet，但 DOEPDMA0=0（未初始化）
     *   → DMA 不知道写入哪块内存，STUP 中断不触发
     *   → 软件永远收不到设备描述符请求
     *   → 主机等待超时 → "Device Descriptor Request Failed" (code 43) */
    {
        /* 1) 打开 EP0 IN (0x80) 和 EP0 OUT (0x00)，64 字节控制端点 */
        DCD_EP_Open(&USB_OTG_dev, 0x00, 64, USB_OTG_EP_CONTROL);
        DCD_EP_Open(&USB_OTG_dev, 0x80, 64, USB_OTG_EP_CONTROL);

        /* 2) 准备 EP0 OUT 接收 setup 包 — 这会设置 DOEPDMA0
         *    USB_OTG_dev.dev.setup_packet 是核心句柄内建的 uint8_t[24] 数组，
         *    直接用它作为 DMA 目标缓冲区（ISR 处理时也从此读取）。 */
        DCD_EP_PrepareRx(&USB_OTG_dev, 0x00,
                         USB_OTG_dev.dev.setup_packet, 8);

        /* 4) 手动使能 DOEPMSK.STUP + DAINTMSK.EP0
         *    这步通常由 USBD_Init() 内部的 USBD_LL_Init() 完成，
         *    但因为我们绕过了整个 USBD_Init，必须手动配置。 */
        volatile uint32_t *doepmsk  = (volatile uint32_t *)(USB_OTG_HS_BASE + 0x80C);
        volatile uint32_t *daintmsk = (volatile uint32_t *)(USB_OTG_HS_BASE + 0x818);
        *doepmsk  |= (1u << 3);        /* DOEPMSK.STUP = 1 */
        *daintmsk |= (1u << 0) |       /* DAINTMSK: EP0 OUT */
                     (1u << 16);       /* DAINTMSK: EP0 IN */
        __asm volatile ("dsb");
    }

    /* ──── 阶段 2.6：SLAVE 模式后处理 ────
     *
     * SLAVE 模式下：
     *   1. GAHBCFG.DMAEN=0 → 禁用内部 DMA
     *   2. rxstsqlvl 中断保持开启（CoreInitDev 已配置）→ PIO 方式读 RX FIFO
     *   3. DIEPMSK.txfifoundrn=1（CoreInitDev 已配置）→ PIO 方式写 TX FIFO
     *
     * GAHBCFG 仅需保证 GINTMSK=1（全局中断使能，CoreInit 已设置）。
     * 显式确认 HBSTLEN 不影响 SLAVE 模式但保持与 HAL 参考一致 (INCR4)。 */
    {
        volatile uint32_t *gahbcfg  = (volatile uint32_t *)(USB_OTG_HS_BASE + 0x008);
        *gahbcfg &= ~((1u << 5) | (0xFu << 1));  /* DMAEN=0, HBSTLEN=0 */
        *gahbcfg |=  (2u << 1);                    /* HBSTLEN=2 (INCR4, 不影响 SLAVE) */
        __asm volatile ("dsb");
    }

    /* GCCFG: 修复 CoreReset 后 PHY 掉电 + 旁路 VBUS 检测
     *
     * USB_OTG_CoreInit(ULPI路径) 的 bug:
     *   1. 设置 GCCFG.PWRDWN=0 唤醒 PHY
     *   2. USB_OTG_CoreReset 软复位 → 所有寄存器清零 → PWRDWN=1 → PHY 断电
     *   3. CoreReset 后只重配 GAHBCFG(DMA)，GCCFG 未恢复
     *
     * 此处重新唤醒 PHY 并设 NOVBUSSENS=1（与 HAL 参考项目一致）。
     * USB3300 ULPI PHY 的 VBUS 指示信号在此设计中不可靠，
     * NOVBUSSENS=1 让核心内部认为 VBUS 始终有效，避免因 VBUS 检测
     * 延迟导致核心忽略主机 setup 包 → code 43。 */
    {
        volatile uint32_t *gccfg = (volatile uint32_t *)(USB_OTG_HS_BASE + 0x038);
        *gccfg &= ~(1u << 16);            /* PWRDWN = 0: 唤醒 PHY */
        *gccfg |=  (1u << 21);            /* NOVBUSSENS = 1: 禁用硬件 VBUS 检测 */
        *gccfg &= ~((1u << 19) | (1u << 18)); /* VBUSBSEN=0, VBUSASEN=0 */
        __asm volatile ("dsb");
        delay(5);
    }

    DBG_STEP_DONE(USB_STEP_VBUS);

    /* ──── 阶段 3：速度 + FIFO 安全防护（库已配置，显式确保） ──── */
    USB_OTG_InitDevSpeed(&USB_OTG_dev, USB_OTG_SPEED_PARAM_HIGH);
    USB_OTG_dev.cfg.speed = USB_OTG_SPEED_HIGH;
    USB_OTG_dev.cfg.mps   = 512;

    /* FIFO：RX=512, TX0(EP0)=64, TX1(MSC EP1)=448 字
     *
     * 关键：OTG_HS FIFO RAM 总共 4KB = 1024 words，原 RX_FIFO_HS_SIZE=2048
     * 超出了硬件容量，导致 Tx FIFO startaddr 溢出到无效地址 →
     * 所有 Tx 操作写入错误位置 → 主机收不到描述符/数据响应 → code 43。
     *
     * 使用与 HAL 参考项目一致的小型 FIFO 布局：
     *   GRXFSIZ = 512,  TX0 = 64（start 512）, TX1 = 448（start 576）
     *   总占用 = 512+64+448 = 1024 words ≈ 4KB（恰好用满） */
    #define HS_RX_FIFO_SIZE  512
    #define HS_TX0_FIFO_SIZE 64
    #define HS_TX1_FIFO_SIZE 448

    USB_OTG_WRITE_REG32(&USB_OTG_dev.regs.GREGS->GRXFSIZ, HS_RX_FIFO_SIZE);

    USB_OTG_FSIZ_TypeDef fifo;
    fifo.d32 = 0;
    fifo.b.depth     = HS_TX0_FIFO_SIZE;
    fifo.b.startaddr = HS_RX_FIFO_SIZE;
    USB_OTG_WRITE_REG32(&USB_OTG_dev.regs.GREGS->DIEPTXF0_HNPTXFSIZ, fifo.d32);

    fifo.b.startaddr += fifo.b.depth;
    fifo.b.depth      = HS_TX1_FIFO_SIZE;
    USB_OTG_WRITE_REG32(&USB_OTG_dev.regs.GREGS->DIEPTXF[0], fifo.d32);

    for (int ep = 1; ep < 5; ep++) {
        fifo.b.startaddr += fifo.b.depth;
        fifo.b.depth = 0;
        USB_OTG_WRITE_REG32(&USB_OTG_dev.regs.GREGS->DIEPTXF[ep], fifo.d32);
    }

    DBG_STEP_DONE(USB_STEP_SPEED);

    /* ──── 阶段 4：GUSBCFG ULPI + 模式确认 ────
     * 与 HAL 参考项目 USB_CoreInit(ULPI) 对齐：
     *   清除 PHYSEL(bit6)/ULPIFSLS(bit17)/TSDPS(bit22) — 纯 ULPI 配置
     *   清除 ULPIEVBUSD(bit20)/ULPIEVBUSI(bit21) — VBUS 由 NOVBUSSENS 旁路
     *   清除 FHMOD(bit30) — USB_SetCurrentMode 已设 FDMOD(bit29)=1，
     *     设置 FHMOD 会导致设备/主机模式冲突。
     *   force_dev 注释原意是对的，但 bit30 实际是 FHMOD 不是 FDMOD，
     *   FDMOD 已在 DCD_Init → USB_SetCurrentMode 中设好，此处只做确认。 */
    {
        volatile uint32_t *gusbcfg = (volatile uint32_t *)(USB_OTG_HS_BASE + GUSBCFG_OFFSET);
        *gusbcfg &= ~((1u << 6)  | (1u << 17) | (1u << 22)); /* PHYSEL=0, ULPIFSLS=0, TSDPS=0 */
        *gusbcfg &= ~((1u << 21) | (1u << 20));               /* ULPIEVBUSI=0, ULPIEVBUSD=0 */
        *gusbcfg &= ~(1u << 30);                              /* FHMOD=0 (清除冲突位) */
        __asm volatile ("dsb");
    }

    /* ──── 阶段 5：用户回调 + DP 上拉（NVIC 先于连接使能）──── */
    USB_OTG_dev.dev.usr_cb->Init();
    hspi_nvic_enable();

    /* DCD_DevConnect: 清除 sftdiscon 通知 PHY 激活 DP 上拉
     *
     * 取消原有的断开重连循环（旧代码的 DCTL.SDIS 翻转），因为：
     *   1. 断开重连在 NVIC 已使能时进行 → 主机 Reset ISR 与 SDIS 翻转竞争
     *   2. 主机在检测到设备后立即开始枚举，断开重连导致状态机混乱 → code 43
     *
     * 只要 PHY 已唤醒（GCCFG.PWRDWN=0），首次连接应正确激活 DP 上拉。
     * 增加 200ms 稳定延迟确保 PHY 锁相环锁定和主机检测。 */
    DCD_DevConnect(&USB_OTG_dev);
    delay(200);

    DBG_STEP_DONE(USB_STEP_CONNECT);
}

bool usbd_msc_is_mounted()
{
    return (USB_OTG_dev.dev.device_status == USB_OTG_CONFIGURED);
}

void usbd_msc_disconnect()
{
    DCD_DevDisconnect(&USB_OTG_dev);
    delay(50);
    hspi_nvic_disable();
    USBD_DeInitFull(&USB_OTG_dev);
    hspi_clk_disable();
    hspi_gpio_deinit();
}

/* ==================== 复合设备 MSC + HID ==================== */

#if HID_ENABLE

/* MSC接口描述符 */
static const uint8_t msc_iface_desc[23] = {
    /* 接口描述符 */
    0x09, 0x04, 0x00, 0x00, 0x02, 0x08, 0x06, 0x50, 0x00,
    /* Endpoint OUT */
    0x07, 0x05, MSC_EP_OUT, 0x02,
    (uint8_t)(MSC_EP_SIZE & 0xFF), (uint8_t)((MSC_EP_SIZE >> 8) & 0xFF),
    0x00,
    /* Endpoint IN */
    0x07, 0x05, MSC_EP_IN, 0x02,
    (uint8_t)(MSC_EP_SIZE & 0xFF), (uint8_t)((MSC_EP_SIZE >> 8) & 0xFF),
    0x00,
};

/* HID接口描述符 */
static const uint8_t hid_iface_desc[25] = {
    /* 接口描述符 */
    0x09, 0x04, 0x01, 0x00, 0x01,
    0x03,        /* bInterfaceClass = HID */
    0x00,        /* bInterfaceSubClass = 0 (无子类) */
    0x00,        /* bInterfaceProtocol = 0 (无协议) */
    0x00,
    /* HID 描述符 */
    0x09, 0x21, 0x10, 0x01, 0x00, 0x01,
    0x22,        /* bDescriptorType = HID_REPORT_DESC */
    HID_REPORT_DESC_LEN & 0xFF, (HID_REPORT_DESC_LEN >> 8) & 0xFF,
    /* Endpoint IN (中断) */
    0x07, 0x05, HID_EP_IN, 0x03,
    (uint8_t)(HID_EP_SIZE & 0xFF), (uint8_t)((HID_EP_SIZE >> 8) & 0xFF),
    HID_HS_BINTERVAL,
};

/* 复合配置描述符: [配置头 9] + [MSC接口 23] + [HID接口 25] = 57 */
#define COMPOSITE_CFG_DESC_SIZE  57

static uint8_t composite_cfg_desc[COMPOSITE_CFG_DESC_SIZE] = {
    0x09, 0x02,
    (uint8_t)(COMPOSITE_CFG_DESC_SIZE & 0xFF),
    (uint8_t)((COMPOSITE_CFG_DESC_SIZE >> 8) & 0xFF),
    0x02,
    0x01, 0x00, 0xC0, 0x32,
};

/* ==================== 复合类回调 ==================== */

static uint8_t composite_init(void *pdev, uint8_t cfgidx)
{
    if (MSC_cb.Init)       MSC_cb.Init(pdev, cfgidx);
    if (USBD_HID_cb.Init)  USBD_HID_cb.Init(pdev, cfgidx);
    return 0;
}

static uint8_t composite_deinit(void *pdev, uint8_t cfgidx)
{
    if (MSC_cb.DeInit)       MSC_cb.DeInit(pdev, cfgidx);
    if (USBD_HID_cb.DeInit)  USBD_HID_cb.DeInit(pdev, cfgidx);
    return 0;
}

static uint8_t composite_setup(void *pdev, USB_SETUP_REQ *req)
{
    if ((req->bmRequest & USB_REQ_RECIPIENT_MASK) == USB_REQ_RECIPIENT_INTERFACE) {
        uint8_t iface = (uint8_t)(req->wIndex & 0xFF);
        if (iface == 0) {
            if (MSC_cb.Setup) return MSC_cb.Setup(pdev, req);
        } else if (iface == 1) {
            if (USBD_HID_cb.Setup) return USBD_HID_cb.Setup(pdev, req);
        }
    } else {
        if (MSC_cb.Setup) return MSC_cb.Setup(pdev, req);
    }
    return USBD_FAIL;
}

static uint8_t composite_data_in(void *pdev, uint8_t epnum)
{
    if (epnum == (MSC_EP_IN & 0x7F)) {
        if (MSC_cb.DataIn) return MSC_cb.DataIn(pdev, epnum);
    }
    if (epnum == (HID_EP_IN & 0x7F)) {
        if (USBD_HID_cb.DataIn) return USBD_HID_cb.DataIn(pdev, epnum);
    }
    return 0;
}

static uint8_t composite_data_out(void *pdev, uint8_t epnum)
{
    if (epnum == (MSC_EP_OUT & 0x7F)) {
        if (MSC_cb.DataOut) return MSC_cb.DataOut(pdev, epnum);
    }
    return 0;
}

static uint8_t *composite_get_cfg_desc(uint8_t speed, uint16_t *length)
{
    (void)speed;
    *length = COMPOSITE_CFG_DESC_SIZE;
    return composite_cfg_desc;
}

static uint8_t *composite_get_other_cfg_desc(uint8_t speed, uint16_t *length)
{
    (void)speed;
    *length = COMPOSITE_CFG_DESC_SIZE;
    return composite_cfg_desc;
}

static USBD_Class_cb_TypeDef composite_cb = {
    .Init               = composite_init,
    .DeInit             = composite_deinit,
    .Setup              = composite_setup,
    .EP0_TxSent         = NULL,
    .EP0_RxReady        = NULL,
    .DataIn             = composite_data_in,
    .DataOut            = composite_data_out,
    .SOF                = NULL,
    .IsoINIncomplete    = NULL,
    .IsoOUTIncomplete   = NULL,
    .GetConfigDescriptor = composite_get_cfg_desc,
    .GetOtherConfigDescriptor = composite_get_other_cfg_desc,
};

void usbd_composite_reinit()
{
    #define DBG_STEP_DONE(s) do { usb_debug_set_step((s), USB_STATUS_OK); usb_debug_refresh(); delay(50); } while(0)

    hspi_gpio_init();
    hspi_clk_enable();
    delay(10);
    usb_debug_set_step(USB_STEP_CLOCK, USB_STATUS_OK);
    usb_debug_set_step(USB_STEP_GPIO, USB_STATUS_OK);
    usb_debug_refresh();
    delay(50);

    hspi_phy_power_up();
    DBG_STEP_DONE(USB_STEP_PHY);

    memcpy(composite_cfg_desc + 9, msc_iface_desc, sizeof(msc_iface_desc));
    memcpy(composite_cfg_desc + 9 + sizeof(msc_iface_desc),
           hid_iface_desc, sizeof(hid_iface_desc));

    USB_OTG_dev.cfg.phy_itface = 1;
    USB_OTG_dev.dev.class_cb = (USBD_Class_cb_TypeDef *)&composite_cb;
    USB_OTG_dev.dev.usr_cb = &USR_cb;
    USB_OTG_dev.dev.usr_device = &USR_desc;

    USB_OTG_dev.cfg.dma_enable = 0;
    DCD_Init(&USB_OTG_dev, USB_OTG_HS_CORE_ID);

    /* DCD_Init → CoreReset 清除了 DCTL.SDIS → 重新断开 */
    {
        volatile uint32_t *dctl = (volatile uint32_t *)(USB_OTG_HS_BASE + 0x804);
        *dctl |= (1u << 1);
        __asm volatile ("dsb");
    }

    DBG_STEP_DONE(USB_STEP_DCD);

    /* EP0 初始化 */
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

    /* SLAVE 模式 */
    {
        volatile uint32_t *gahbcfg = (volatile uint32_t *)(USB_OTG_HS_BASE + 0x008);
        *gahbcfg &= ~((1u << 5) | (0xFu << 1));
        *gahbcfg |=  (2u << 1);
        __asm volatile ("dsb");
    }

    /* GCCFG: 重新唤醒 PHY */
    {
        volatile uint32_t *gccfg = (volatile uint32_t *)(USB_OTG_HS_BASE + 0x038);
        *gccfg &= ~(1u << 16);
        *gccfg |=  (1u << 21);
        *gccfg &= ~((1u << 19) | (1u << 18));
        __asm volatile ("dsb");
        delay(5);
    }

    DBG_STEP_DONE(USB_STEP_VBUS);

    USB_OTG_InitDevSpeed(&USB_OTG_dev, USB_OTG_SPEED_PARAM_HIGH);
    USB_OTG_dev.cfg.speed = USB_OTG_SPEED_HIGH;
    USB_OTG_dev.cfg.mps   = 512;

    /* FIFO 配置: RX=512, TX0(EP0)=64, TX1(MSC EP1)=192, TX2(HID EP2)=192 */
    #define HS_RX_FIFO_SIZE_C  512
    #define HS_TX0_FIFO_SIZE_C 64
    #define HS_TX1_FIFO_SIZE_C 192
    #define HS_TX2_FIFO_SIZE_C 192

    USB_OTG_WRITE_REG32(&USB_OTG_dev.regs.GREGS->GRXFSIZ, HS_RX_FIFO_SIZE_C);

    USB_OTG_FSIZ_TypeDef fifo;
    fifo.d32 = 0;
    fifo.b.depth     = HS_TX0_FIFO_SIZE_C;
    fifo.b.startaddr = HS_RX_FIFO_SIZE_C;
    USB_OTG_WRITE_REG32(&USB_OTG_dev.regs.GREGS->DIEPTXF0_HNPTXFSIZ, fifo.d32);

    fifo.b.startaddr += fifo.b.depth;
    fifo.b.depth      = HS_TX1_FIFO_SIZE_C;
    USB_OTG_WRITE_REG32(&USB_OTG_dev.regs.GREGS->DIEPTXF[0], fifo.d32);

    fifo.b.startaddr += fifo.b.depth;
    fifo.b.depth      = HS_TX2_FIFO_SIZE_C;
    USB_OTG_WRITE_REG32(&USB_OTG_dev.regs.GREGS->DIEPTXF[1], fifo.d32);

    for (int ep = 2; ep < 5; ep++) {
        fifo.b.startaddr += fifo.b.depth;
        fifo.b.depth = 0;
        USB_OTG_WRITE_REG32(&USB_OTG_dev.regs.GREGS->DIEPTXF[ep], fifo.d32);
    }

    DBG_STEP_DONE(USB_STEP_SPEED);

    /* GUSBCFG */
    {
        volatile uint32_t *gusbcfg = (volatile uint32_t *)(USB_OTG_HS_BASE + GUSBCFG_OFFSET);
        *gusbcfg &= ~((1u << 6) | (1u << 17) | (1u << 22));
        *gusbcfg &= ~((1u << 21) | (1u << 20));
        *gusbcfg &= ~(1u << 30);
        __asm volatile ("dsb");
    }

    USB_OTG_dev.dev.usr_cb->Init();
    hspi_nvic_enable();
    DCD_DevConnect(&USB_OTG_dev);
    delay(200);

    DBG_STEP_DONE(USB_STEP_CONNECT);
}

void usbd_hid_reinit()
{
    hspi_gpio_init();
    hspi_clk_enable();
    delay(10);
    usb_debug_set_step(USB_STEP_CLOCK, USB_STATUS_OK);
    usb_debug_set_step(USB_STEP_GPIO, USB_STATUS_OK);
    usb_debug_refresh();
    delay(50);

    hspi_phy_power_up();
    DBG_STEP_DONE(USB_STEP_PHY);

    USB_OTG_dev.cfg.phy_itface = 1;
    USB_OTG_dev.dev.class_cb = (USBD_Class_cb_TypeDef *)&USBD_HID_cb;
    USB_OTG_dev.dev.usr_cb = &USR_cb;
    USB_OTG_dev.dev.usr_device = &USR_desc;

    USB_OTG_dev.cfg.dma_enable = 0;
    DCD_Init(&USB_OTG_dev, USB_OTG_HS_CORE_ID);

    {
        volatile uint32_t *dctl = (volatile uint32_t *)(USB_OTG_HS_BASE + 0x804);
        *dctl |= (1u << 1);
        __asm volatile ("dsb");
    }

    DBG_STEP_DONE(USB_STEP_DCD);

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

    {
        volatile uint32_t *gccfg = (volatile uint32_t *)(USB_OTG_HS_BASE + 0x038);
        *gccfg &= ~(1u << 16);
        *gccfg |=  (1u << 21);
        *gccfg &= ~((1u << 19) | (1u << 18));
        __asm volatile ("dsb");
        delay(5);
    }

    DBG_STEP_DONE(USB_STEP_VBUS);

    USB_OTG_InitDevSpeed(&USB_OTG_dev, USB_OTG_SPEED_PARAM_HIGH);
    USB_OTG_dev.cfg.speed = USB_OTG_SPEED_HIGH;
    USB_OTG_dev.cfg.mps   = 512;

    #define HS_RX_FIFO_SIZE_HID  128
    #define HS_TX0_FIFO_SIZE_HID 64
    #define HS_TX2_FIFO_SIZE_HID 64

    USB_OTG_WRITE_REG32(&USB_OTG_dev.regs.GREGS->GRXFSIZ, HS_RX_FIFO_SIZE_HID);

    USB_OTG_FSIZ_TypeDef fifo;
    fifo.d32 = 0;
    fifo.b.depth     = HS_TX0_FIFO_SIZE_HID;
    fifo.b.startaddr = HS_RX_FIFO_SIZE_HID;
    USB_OTG_WRITE_REG32(&USB_OTG_dev.regs.GREGS->DIEPTXF0_HNPTXFSIZ, fifo.d32);

    fifo.b.startaddr += fifo.b.depth;
    fifo.b.depth      = 0;
    USB_OTG_WRITE_REG32(&USB_OTG_dev.regs.GREGS->DIEPTXF[0], fifo.d32);

    fifo.b.startaddr += fifo.b.depth;
    fifo.b.depth      = HS_TX2_FIFO_SIZE_HID;
    USB_OTG_WRITE_REG32(&USB_OTG_dev.regs.GREGS->DIEPTXF[1], fifo.d32);

    for (int ep = 2; ep < 5; ep++) {
        fifo.b.startaddr += fifo.b.depth;
        fifo.b.depth = 0;
        USB_OTG_WRITE_REG32(&USB_OTG_dev.regs.GREGS->DIEPTXF[ep], fifo.d32);
    }

    DBG_STEP_DONE(USB_STEP_SPEED);

    {
        volatile uint32_t *gusbcfg = (volatile uint32_t *)(USB_OTG_HS_BASE + GUSBCFG_OFFSET);
        *gusbcfg &= ~((1u << 6) | (1u << 17) | (1u << 22));
        *gusbcfg &= ~((1u << 21) | (1u << 20));
        *gusbcfg &= ~(1u << 30);
        __asm volatile ("dsb");
    }

    USB_OTG_dev.dev.usr_cb->Init();
    hspi_nvic_enable();
    DCD_DevConnect(&USB_OTG_dev);
    delay(200);

    DBG_STEP_DONE(USB_STEP_CONNECT);
}

/* ==================== CDC 复合模式共享辅助函数 ==================== */

/*
 * BSP + DCD 初始化通用部分（各 CDC 复合模式共享）
 * 包含：GPIO/时钟/PHY/DCD/EP0/SLAVE/GCCFG/速度/GUSBCFG 全流程
 */
static void cdc_composite_bsp_init(void)
{
    hspi_gpio_init();
    hspi_clk_enable();
    delay(10);
    usb_debug_set_step(USB_STEP_CLOCK, USB_STATUS_OK);
    usb_debug_set_step(USB_STEP_GPIO, USB_STATUS_OK);
    usb_debug_refresh();
    delay(50);

    hspi_phy_power_up();
    usb_debug_set_step(USB_STEP_PHY, USB_STATUS_OK);
    usb_debug_refresh();
    delay(50);

    USB_OTG_dev.cfg.phy_itface = 1;
    USB_OTG_dev.cfg.dma_enable = 0;
    DCD_Init(&USB_OTG_dev, USB_OTG_HS_CORE_ID);

    {
        volatile uint32_t *dctl = (volatile uint32_t *)(USB_OTG_HS_BASE + 0x804);
        *dctl |= (1u << 1);
        __asm volatile ("dsb");
    }

    usb_debug_set_step(USB_STEP_DCD, USB_STATUS_OK);
    usb_debug_refresh();
    delay(50);

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

    {
        volatile uint32_t *gccfg = (volatile uint32_t *)(USB_OTG_HS_BASE + 0x038);
        *gccfg &= ~(1u << 16);
        *gccfg |=  (1u << 21);
        *gccfg &= ~((1u << 19) | (1u << 18));
        __asm volatile ("dsb");
        delay(5);
    }

    usb_debug_set_step(USB_STEP_VBUS, USB_STATUS_OK);
    usb_debug_refresh();
    delay(50);

    USB_OTG_InitDevSpeed(&USB_OTG_dev, USB_OTG_SPEED_PARAM_HIGH);
    USB_OTG_dev.cfg.speed = USB_OTG_SPEED_HIGH;
    USB_OTG_dev.cfg.mps   = 512;

    {
        volatile uint32_t *gusbcfg = (volatile uint32_t *)(USB_OTG_HS_BASE + GUSBCFG_OFFSET);
        *gusbcfg &= ~((1u << 6) | (1u << 17) | (1u << 22));
        *gusbcfg &= ~((1u << 21) | (1u << 20));
        *gusbcfg &= ~(1u << 30);
        __asm volatile ("dsb");
    }
}

static void cdc_composite_connect(void)
{
    USB_OTG_dev.dev.usr_cb->Init();
    hspi_nvic_enable();
    DCD_DevConnect(&USB_OTG_dev);
    delay(200);
    usb_debug_set_step(USB_STEP_CONNECT, USB_STATUS_OK);
    usb_debug_refresh();
    delay(50);
}

/* 从 usbd_cdc.cpp 获取 CDC 接口描述符 */
extern const uint8_t* cdc_get_iface_desc(void);
extern uint16_t cdc_get_iface_desc_size(void);

/*
 * 构建 CDC 接口描述符（复制并修补接口号）
 * cdc_iface_desc 硬编码为 Interface 2+3，此处根据 comm_iface 修补
 */
static void patch_cdc_iface_desc(uint8_t *buf, uint8_t comm_iface)
{
    memcpy(buf, cdc_get_iface_desc(), cdc_get_iface_desc_size());
    buf[2]  = comm_iface;        /* Communication Interface bInterfaceNumber */
    buf[26] = comm_iface;        /* Union Func Desc bMasterInterface */
    buf[27] = comm_iface + 1;    /* Union Func Desc bSlaveInterface */
    buf[37] = comm_iface + 1;    /* Data Interface bInterfaceNumber */
}

/* ==================== MSC+HID+CDC 三接口复合 ==================== */

#define COMPOSITE_CDC_CFG_DESC_SIZE  115
#define COMPOSITE_CDC_NUM_IFACES     4

static uint8_t composite_cdc_cfg_desc[COMPOSITE_CDC_CFG_DESC_SIZE];
static uint8_t composite_cdc_cdc_desc[58]; /* CDC 接口描述符副本（带修补） */

static void build_composite_cdc_desc(void)
{
    uint8_t *p = composite_cdc_cfg_desc;
    /* 配置头 */
    p[0] = 0x09; p[1] = 0x02;
    p[2] = COMPOSITE_CDC_CFG_DESC_SIZE & 0xFF;
    p[3] = (COMPOSITE_CDC_CFG_DESC_SIZE >> 8) & 0xFF;
    p[4] = COMPOSITE_CDC_NUM_IFACES;
    p[5] = 0x01; p[6] = 0x00; p[7] = 0xC0; p[8] = 0x32;
    p += 9;

    /* MSC 接口 0 */
    memcpy(p, msc_iface_desc, sizeof(msc_iface_desc));
    p += sizeof(msc_iface_desc);

    /* HID 接口 1 */
    memcpy(p, hid_iface_desc, sizeof(hid_iface_desc));
    p += sizeof(hid_iface_desc);

    /* CDC 接口 2+3 */
    patch_cdc_iface_desc(composite_cdc_cdc_desc, 2);
    memcpy(p, composite_cdc_cdc_desc, sizeof(composite_cdc_cdc_desc));
}

static uint8_t composite_cdc_data_in(void *pdev, uint8_t epnum)
{
    if (epnum == (MSC_EP_IN & 0x7F)) {
        if (MSC_cb.DataIn) return MSC_cb.DataIn(pdev, epnum);
    }
    if (epnum == (HID_EP_IN & 0x7F)) {
        if (USBD_HID_cb.DataIn) return USBD_HID_cb.DataIn(pdev, epnum);
    }
    return 0;
}

static uint8_t composite_cdc_data_out(void *pdev, uint8_t epnum)
{
    if (epnum == (MSC_EP_OUT & 0x7F)) {
        if (MSC_cb.DataOut) return MSC_cb.DataOut(pdev, epnum);
    }
    if (epnum == (CDC_DATA_OUT_EP & 0x7F)) {
        return cdc_class_data_out(pdev, epnum);
    }
    return 0;
}

static uint8_t composite_cdc_sof(void *pdev)
{
    return cdc_class_sof(pdev);
}

static uint8_t *composite_cdc_get_cfg_desc(uint8_t speed, uint16_t *length)
{
    (void)speed;
    *length = COMPOSITE_CDC_CFG_DESC_SIZE;
    return composite_cdc_cfg_desc;
}

static uint8_t *composite_cdc_get_other_cfg_desc(uint8_t speed, uint16_t *length)
{
    (void)speed;
    *length = COMPOSITE_CDC_CFG_DESC_SIZE;
    return composite_cdc_cfg_desc;
}

static uint8_t composite_cdc_init(void *pdev, uint8_t cfgidx)
{
    if (MSC_cb.Init)       MSC_cb.Init(pdev, cfgidx);
    if (USBD_HID_cb.Init)  USBD_HID_cb.Init(pdev, cfgidx);
    cdc_class_init(pdev, cfgidx);
    return 0;
}

static uint8_t composite_cdc_deinit(void *pdev, uint8_t cfgidx)
{
    if (MSC_cb.DeInit)       MSC_cb.DeInit(pdev, cfgidx);
    if (USBD_HID_cb.DeInit)  USBD_HID_cb.DeInit(pdev, cfgidx);
    cdc_class_deinit(pdev, cfgidx);
    return 0;
}

static uint8_t composite_cdc_setup(void *pdev, USB_SETUP_REQ *req)
{
    if ((req->bmRequest & USB_REQ_RECIPIENT_MASK) == USB_REQ_RECIPIENT_INTERFACE) {
        uint8_t iface = (uint8_t)(req->wIndex & 0xFF);
        if (iface == 0) {
            if (MSC_cb.Setup) return MSC_cb.Setup(pdev, req);
        } else if (iface == 1) {
            if (USBD_HID_cb.Setup) return USBD_HID_cb.Setup(pdev, req);
        } else if (iface == 2 || iface == 3) {
            return cdc_class_setup(pdev, req);
        }
    } else {
        if (MSC_cb.Setup) return MSC_cb.Setup(pdev, req);
    }
    return USBD_FAIL;
}

static USBD_Class_cb_TypeDef composite_cdc_cb;

static void setup_composite_cdc_cb(void)
{
    memset(&composite_cdc_cb, 0, sizeof(composite_cdc_cb));
    composite_cdc_cb.Init       = composite_cdc_init;
    composite_cdc_cb.DeInit     = composite_cdc_deinit;
    composite_cdc_cb.Setup      = composite_cdc_setup;
    composite_cdc_cb.EP0_TxSent = NULL;
    composite_cdc_cb.EP0_RxReady = NULL;
    composite_cdc_cb.DataIn     = composite_cdc_data_in;
    composite_cdc_cb.DataOut    = composite_cdc_data_out;
    composite_cdc_cb.SOF        = composite_cdc_sof;
    composite_cdc_cb.IsoINIncomplete  = NULL;
    composite_cdc_cb.IsoOUTIncomplete = NULL;
    composite_cdc_cb.GetConfigDescriptor     = composite_cdc_get_cfg_desc;
    composite_cdc_cb.GetOtherConfigDescriptor = composite_cdc_get_other_cfg_desc;
}

void usbd_composite_cdc_reinit()
{
    g_cdc_ready = false;
    g_cdc_dtr = false;
    g_cdc_rx_head = 0;
    g_cdc_rx_tail = 0;
    g_cdc_tx_head = 0;
    g_cdc_tx_tail = 0;

    cdc_composite_bsp_init();

    setup_composite_cdc_cb();
    build_composite_cdc_desc();

    USB_OTG_dev.dev.class_cb   = &composite_cdc_cb;
    USB_OTG_dev.dev.usr_cb     = &USR_cb;
    USB_OTG_dev.dev.usr_device = &USR_desc;

    /* FIFO: RX=512, TX0(EP0)=64, TX1(MSC EP1)=128, TX2(HID EP2)=96, TX3(CDC notif EP3)=16, TX4(CDC data EP4)=128 */
    USB_OTG_WRITE_REG32(&USB_OTG_dev.regs.GREGS->GRXFSIZ, 512);

    USB_OTG_FSIZ_TypeDef fifo;
    fifo.d32 = 0;
    fifo.b.depth     = 64;
    fifo.b.startaddr = 512;
    USB_OTG_WRITE_REG32(&USB_OTG_dev.regs.GREGS->DIEPTXF0_HNPTXFSIZ, fifo.d32);

    fifo.b.startaddr += fifo.b.depth;
    fifo.b.depth      = 128;
    USB_OTG_WRITE_REG32(&USB_OTG_dev.regs.GREGS->DIEPTXF[0], fifo.d32);

    fifo.b.startaddr += fifo.b.depth;
    fifo.b.depth      = 96;
    USB_OTG_WRITE_REG32(&USB_OTG_dev.regs.GREGS->DIEPTXF[1], fifo.d32);

    fifo.b.startaddr += fifo.b.depth;
    fifo.b.depth      = 16;
    USB_OTG_WRITE_REG32(&USB_OTG_dev.regs.GREGS->DIEPTXF[2], fifo.d32);

    fifo.b.startaddr += fifo.b.depth;
    fifo.b.depth      = 128;
    USB_OTG_WRITE_REG32(&USB_OTG_dev.regs.GREGS->DIEPTXF[3], fifo.d32);

    for (int ep = 4; ep < 5; ep++) {
        fifo.b.startaddr += fifo.b.depth;
        fifo.b.depth = 0;
        USB_OTG_WRITE_REG32(&USB_OTG_dev.regs.GREGS->DIEPTXF[ep], fifo.d32);
    }

    usb_debug_set_step(USB_STEP_SPEED, USB_STATUS_OK);
    usb_debug_refresh();
    delay(50);

    cdc_composite_connect();
}

/* ==================== HID+CDC 双接口复合 ==================== */

#define HID_CDC_CFG_DESC_SIZE   92   /* 9 + 25(HID iface 0) + 58(CDC iface 1+2) */
#define HID_CDC_NUM_IFACES      3

/* HID 接口描述符（Interface 0，用于 HID+CDC 模式） */
static const uint8_t hid0_iface_desc[25] = {
    0x09, 0x04, 0x00, 0x00, 0x01,
    0x03, 0x00, 0x00, 0x00,
    0x09, 0x21, 0x10, 0x01, 0x00, 0x01,
    0x22,
    HID_REPORT_DESC_LEN & 0xFF, (HID_REPORT_DESC_LEN >> 8) & 0xFF,
    0x07, 0x05, HID_EP_IN, 0x03,
    (uint8_t)(HID_EP_SIZE & 0xFF), (uint8_t)((HID_EP_SIZE >> 8) & 0xFF),
    HID_HS_BINTERVAL,
};

static uint8_t hid_cdc_cfg_desc[HID_CDC_CFG_DESC_SIZE];
static uint8_t hid_cdc_cdc_desc[58];

static void build_hid_cdc_desc(void)
{
    uint8_t *p = hid_cdc_cfg_desc;
    p[0] = 0x09; p[1] = 0x02;
    p[2] = HID_CDC_CFG_DESC_SIZE & 0xFF;
    p[3] = (HID_CDC_CFG_DESC_SIZE >> 8) & 0xFF;
    p[4] = HID_CDC_NUM_IFACES;
    p[5] = 0x01; p[6] = 0x00; p[7] = 0xC0; p[8] = 0x32;
    p += 9;

    memcpy(p, hid0_iface_desc, sizeof(hid0_iface_desc));
    p += sizeof(hid0_iface_desc);

    patch_cdc_iface_desc(hid_cdc_cdc_desc, 1);
    memcpy(p, hid_cdc_cdc_desc, sizeof(hid_cdc_cdc_desc));
}

static uint8_t hid_cdc_init(void *pdev, uint8_t cfgidx)
{
    if (USBD_HID_cb.Init)  USBD_HID_cb.Init(pdev, cfgidx);
    cdc_class_init(pdev, cfgidx);
    return 0;
}

static uint8_t hid_cdc_deinit(void *pdev, uint8_t cfgidx)
{
    if (USBD_HID_cb.DeInit)  USBD_HID_cb.DeInit(pdev, cfgidx);
    cdc_class_deinit(pdev, cfgidx);
    return 0;
}

static uint8_t hid_cdc_setup(void *pdev, USB_SETUP_REQ *req)
{
    if ((req->bmRequest & USB_REQ_RECIPIENT_MASK) == USB_REQ_RECIPIENT_INTERFACE) {
        uint8_t iface = (uint8_t)(req->wIndex & 0xFF);
        if (iface == 0) {
            if (USBD_HID_cb.Setup) return USBD_HID_cb.Setup(pdev, req);
        } else if (iface == 1 || iface == 2) {
            return cdc_class_setup(pdev, req);
        }
    } else {
        if (USBD_HID_cb.Setup) return USBD_HID_cb.Setup(pdev, req);
    }
    return USBD_FAIL;
}

static uint8_t *hid_cdc_get_cfg_desc(uint8_t speed, uint16_t *length)
{
    (void)speed;
    *length = HID_CDC_CFG_DESC_SIZE;
    return hid_cdc_cfg_desc;
}

static uint8_t *hid_cdc_get_other_cfg_desc(uint8_t speed, uint16_t *length)
{
    (void)speed;
    *length = HID_CDC_CFG_DESC_SIZE;
    return hid_cdc_cfg_desc;
}

static USBD_Class_cb_TypeDef hid_cdc_cb;

static void setup_hid_cdc_cb(void)
{
    memset(&hid_cdc_cb, 0, sizeof(hid_cdc_cb));
    hid_cdc_cb.Init       = hid_cdc_init;
    hid_cdc_cb.DeInit     = hid_cdc_deinit;
    hid_cdc_cb.Setup      = hid_cdc_setup;
    hid_cdc_cb.EP0_TxSent = NULL;
    hid_cdc_cb.EP0_RxReady = NULL;
    hid_cdc_cb.DataIn     = composite_cdc_data_in;  /* 复用 HID EP2 + CDC 路由 */
    hid_cdc_cb.DataOut    = composite_cdc_data_out;  /* 复用 CDC EP4 OUT 路由 */
    hid_cdc_cb.SOF        = composite_cdc_sof;
    hid_cdc_cb.IsoINIncomplete  = NULL;
    hid_cdc_cb.IsoOUTIncomplete = NULL;
    hid_cdc_cb.GetConfigDescriptor     = hid_cdc_get_cfg_desc;
    hid_cdc_cb.GetOtherConfigDescriptor = hid_cdc_get_other_cfg_desc;
}

void usbd_hid_cdc_reinit()
{
    g_cdc_ready = false;
    g_cdc_dtr = false;
    g_cdc_rx_head = 0;
    g_cdc_rx_tail = 0;
    g_cdc_tx_head = 0;
    g_cdc_tx_tail = 0;

    cdc_composite_bsp_init();

    setup_hid_cdc_cb();
    build_hid_cdc_desc();

    USB_OTG_dev.dev.class_cb   = &hid_cdc_cb;
    USB_OTG_dev.dev.usr_cb     = &USR_cb;
    USB_OTG_dev.dev.usr_device = &USR_desc;

    /* FIFO: RX=256, TX0(EP0)=64, TX1(EP1)=0, TX2(HID EP2)=96, TX3(CDC notif EP3)=16, TX4(CDC data EP4)=128 */
    USB_OTG_WRITE_REG32(&USB_OTG_dev.regs.GREGS->GRXFSIZ, 256);

    USB_OTG_FSIZ_TypeDef fifo;
    fifo.d32 = 0;
    fifo.b.depth     = 64;
    fifo.b.startaddr = 256;
    USB_OTG_WRITE_REG32(&USB_OTG_dev.regs.GREGS->DIEPTXF0_HNPTXFSIZ, fifo.d32);

    fifo.b.startaddr += fifo.b.depth;
    fifo.b.depth      = 0;
    USB_OTG_WRITE_REG32(&USB_OTG_dev.regs.GREGS->DIEPTXF[0], fifo.d32);

    fifo.b.startaddr += fifo.b.depth;
    fifo.b.depth      = 96;
    USB_OTG_WRITE_REG32(&USB_OTG_dev.regs.GREGS->DIEPTXF[1], fifo.d32);

    fifo.b.startaddr += fifo.b.depth;
    fifo.b.depth      = 16;
    USB_OTG_WRITE_REG32(&USB_OTG_dev.regs.GREGS->DIEPTXF[2], fifo.d32);

    fifo.b.startaddr += fifo.b.depth;
    fifo.b.depth      = 128;
    USB_OTG_WRITE_REG32(&USB_OTG_dev.regs.GREGS->DIEPTXF[3], fifo.d32);

    for (int ep = 4; ep < 5; ep++) {
        fifo.b.startaddr += fifo.b.depth;
        fifo.b.depth = 0;
        USB_OTG_WRITE_REG32(&USB_OTG_dev.regs.GREGS->DIEPTXF[ep], fifo.d32);
    }

    usb_debug_set_step(USB_STEP_SPEED, USB_STATUS_OK);
    usb_debug_refresh();
    delay(50);

    cdc_composite_connect();
}

/* ==================== MSC+CDC 双接口复合 ==================== */

#define MSC_CDC_CFG_DESC_SIZE   90   /* 9 + 23(MSC iface 0) + 58(CDC iface 1+2) */
#define MSC_CDC_NUM_IFACES      3

static uint8_t msc_cdc_cfg_desc[MSC_CDC_CFG_DESC_SIZE];
static uint8_t msc_cdc_cdc_desc[58];

static void build_msc_cdc_desc(void)
{
    uint8_t *p = msc_cdc_cfg_desc;
    p[0] = 0x09; p[1] = 0x02;
    p[2] = MSC_CDC_CFG_DESC_SIZE & 0xFF;
    p[3] = (MSC_CDC_CFG_DESC_SIZE >> 8) & 0xFF;
    p[4] = MSC_CDC_NUM_IFACES;
    p[5] = 0x01; p[6] = 0x00; p[7] = 0xC0; p[8] = 0x32;
    p += 9;

    memcpy(p, msc_iface_desc, sizeof(msc_iface_desc));
    p += sizeof(msc_iface_desc);

    patch_cdc_iface_desc(msc_cdc_cdc_desc, 1);
    memcpy(p, msc_cdc_cdc_desc, sizeof(msc_cdc_cdc_desc));
}

static uint8_t msc_cdc_init(void *pdev, uint8_t cfgidx)
{
    if (MSC_cb.Init)  MSC_cb.Init(pdev, cfgidx);
    cdc_class_init(pdev, cfgidx);
    return 0;
}

static uint8_t msc_cdc_deinit(void *pdev, uint8_t cfgidx)
{
    if (MSC_cb.DeInit)  MSC_cb.DeInit(pdev, cfgidx);
    cdc_class_deinit(pdev, cfgidx);
    return 0;
}

static uint8_t msc_cdc_setup(void *pdev, USB_SETUP_REQ *req)
{
    if ((req->bmRequest & USB_REQ_RECIPIENT_MASK) == USB_REQ_RECIPIENT_INTERFACE) {
        uint8_t iface = (uint8_t)(req->wIndex & 0xFF);
        if (iface == 0) {
            if (MSC_cb.Setup) return MSC_cb.Setup(pdev, req);
        } else if (iface == 1 || iface == 2) {
            return cdc_class_setup(pdev, req);
        }
    } else {
        if (MSC_cb.Setup) return MSC_cb.Setup(pdev, req);
    }
    return USBD_FAIL;
}

static uint8_t *msc_cdc_get_cfg_desc(uint8_t speed, uint16_t *length)
{
    (void)speed;
    *length = MSC_CDC_CFG_DESC_SIZE;
    return msc_cdc_cfg_desc;
}

static uint8_t *msc_cdc_get_other_cfg_desc(uint8_t speed, uint16_t *length)
{
    (void)speed;
    *length = MSC_CDC_CFG_DESC_SIZE;
    return msc_cdc_cfg_desc;
}

static USBD_Class_cb_TypeDef msc_cdc_cb;

static void setup_msc_cdc_cb(void)
{
    memset(&msc_cdc_cb, 0, sizeof(msc_cdc_cb));
    msc_cdc_cb.Init       = msc_cdc_init;
    msc_cdc_cb.DeInit     = msc_cdc_deinit;
    msc_cdc_cb.Setup      = msc_cdc_setup;
    msc_cdc_cb.EP0_TxSent = NULL;
    msc_cdc_cb.EP0_RxReady = NULL;
    msc_cdc_cb.DataIn     = composite_cdc_data_in;   /* 复用 MSC EP1 IN 路由 */
    msc_cdc_cb.DataOut    = composite_cdc_data_out;  /* 复用 MSC EP1 OUT + CDC EP4 OUT 路由 */
    msc_cdc_cb.SOF        = composite_cdc_sof;
    msc_cdc_cb.IsoINIncomplete  = NULL;
    msc_cdc_cb.IsoOUTIncomplete = NULL;
    msc_cdc_cb.GetConfigDescriptor     = msc_cdc_get_cfg_desc;
    msc_cdc_cb.GetOtherConfigDescriptor = msc_cdc_get_other_cfg_desc;
}

void usbd_msc_cdc_reinit()
{
    g_cdc_ready = false;
    g_cdc_dtr = false;
    g_cdc_rx_head = 0;
    g_cdc_rx_tail = 0;
    g_cdc_tx_head = 0;
    g_cdc_tx_tail = 0;

    cdc_composite_bsp_init();

    setup_msc_cdc_cb();
    build_msc_cdc_desc();

    USB_OTG_dev.dev.class_cb   = &msc_cdc_cb;
    USB_OTG_dev.dev.usr_cb     = &USR_cb;
    USB_OTG_dev.dev.usr_device = &USR_desc;

    /* FIFO: RX=384, TX0(EP0)=64, TX1(MSC EP1)=128, TX2(EP2)=0, TX3(CDC notif EP3)=16, TX4(CDC data EP4)=128 */
    USB_OTG_WRITE_REG32(&USB_OTG_dev.regs.GREGS->GRXFSIZ, 384);

    USB_OTG_FSIZ_TypeDef fifo;
    fifo.d32 = 0;
    fifo.b.depth     = 64;
    fifo.b.startaddr = 384;
    USB_OTG_WRITE_REG32(&USB_OTG_dev.regs.GREGS->DIEPTXF0_HNPTXFSIZ, fifo.d32);

    fifo.b.startaddr += fifo.b.depth;
    fifo.b.depth      = 128;
    USB_OTG_WRITE_REG32(&USB_OTG_dev.regs.GREGS->DIEPTXF[0], fifo.d32);

    fifo.b.startaddr += fifo.b.depth;
    fifo.b.depth      = 0;
    USB_OTG_WRITE_REG32(&USB_OTG_dev.regs.GREGS->DIEPTXF[1], fifo.d32);

    fifo.b.startaddr += fifo.b.depth;
    fifo.b.depth      = 16;
    USB_OTG_WRITE_REG32(&USB_OTG_dev.regs.GREGS->DIEPTXF[2], fifo.d32);

    fifo.b.startaddr += fifo.b.depth;
    fifo.b.depth      = 128;
    USB_OTG_WRITE_REG32(&USB_OTG_dev.regs.GREGS->DIEPTXF[3], fifo.d32);

    for (int ep = 4; ep < 5; ep++) {
        fifo.b.startaddr += fifo.b.depth;
        fifo.b.depth = 0;
        USB_OTG_WRITE_REG32(&USB_OTG_dev.regs.GREGS->DIEPTXF[ep], fifo.d32);
    }

    usb_debug_set_step(USB_STEP_SPEED, USB_STATUS_OK);
    usb_debug_refresh();
    delay(50);

    cdc_composite_connect();
}

#else

void usbd_composite_reinit()
{
    usbd_msc_reinit();
}

void usbd_composite_cdc_reinit()
{
    usbd_msc_reinit();
}

void usbd_hid_cdc_reinit()
{
    usbd_msc_reinit();
}

void usbd_msc_cdc_reinit()
{
    usbd_msc_reinit();
}

#endif /* HID_ENABLE */

#endif /* USB_MSC_ENABLE */
