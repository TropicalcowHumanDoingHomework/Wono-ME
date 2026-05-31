/*
 * USB大容量存储管理器 — W25Q512JVEIQ SPI NOR Flash 存储后端
 *
 * 架构：W25Q512 (64MB) ←→ storage callbacks ←→ usbd_msc (BOT/SCSI) ←→ USB_OTG_HS ←→ USB3300 ULPI PHY
 *
 * 特点：
 * - 读取直接从SPI Flash读取（无需RAM缓存块）
 * - 写入由usbd_msc.cpp中的w25q_write_block()处理（4KB扇区缓存策略）
 * - 无需MCU内部Flash参与
 */

#include "usb_manager.h"

#if USB_MSC_ENABLE

#include "usbd_msc.h"
#include <string.h>
#include "usb_debug.h"
#include "ui_state.h"
#include "usbd_hid.h"

extern USB_OTG_CORE_HANDLE USB_OTG_dev;

/* ==================== 全局状态 ==================== */

static bool usb_active  = false;
static bool usb_reg     = false;
static bool usb_dirty   = false;
static bool usb_pending = false;
static uint32_t usb_poll_timeout = 0;
static bool w25q_failed = false;
static bool s_first_init = true;

#define USB_POLL_TIMEOUT       5000
#define USB_POLL_ERROR_TIMEOUT 50

/* ==================== USBD_STORAGE 回调实现 ==================== */

static int8_t storage_init(uint8_t lun)
{
    (void)lun;
    return 0;
}

static int8_t storage_get_capacity(uint8_t lun, uint32_t *block_num, uint32_t *block_size)
{
    (void)lun;
    *block_num  = USB_DISK_BLOCK_COUNT;   /* 131072 (64MB / 512B) */
    *block_size = USB_DISK_BLOCK_SIZE;    /* 512 */
    return 0;
}

static int8_t storage_is_ready(uint8_t lun)
{
    (void)lun;
    return 0;  /* SPI Flash 始终就绪 */
}

static int8_t storage_is_write_protected(uint8_t lun)
{
    (void)lun;
    return ui.param[USB_WP] ? 1 : 0;
}

/*
 * READ10 回调
 * 直接从W25Q512读取到USB缓冲区
 */
static int8_t storage_read(uint8_t lun, uint8_t *buf, uint32_t blk_addr, uint16_t blk_len)
{
    (void)lun;

    if ((uint32_t)blk_addr + blk_len > USB_DISK_BLOCK_COUNT) {
        return -1;
    }

    for (uint16_t i = 0; i < blk_len; i++) {
        w25q_read_block(blk_addr + i, buf + (uint32_t)i * 512);
    }

    return 0;
}

/*
 * WRITE10 回调
 * 通过w25q_write_block()写入（自动处理4KB扇区擦除）
 */
static int8_t storage_write(uint8_t lun, uint8_t *buf, uint32_t blk_addr, uint16_t blk_len)
{
    (void)lun;

    if ((uint32_t)blk_addr + blk_len > USB_DISK_BLOCK_COUNT) {
        return -1;
    }

    for (uint16_t i = 0; i < blk_len; i++) {
        if (w25q_write_block(blk_addr + i, buf + (uint32_t)i * 512) != 0) {
            return -1;
        }
    }

    usb_dirty = true;
    return 0;
}

static int8_t storage_get_max_lun(void)
{
    return 0;
}

/* INQUIRY数据：标识虚拟U盘 */
static uint8_t inquiry_data[36] = {
    0x00,
    0x80,
    0x04,
    0x02,
    36 - 5,
    0x00, 0x00, 0x00,
    'W','o','u','o','U','I',' ',' ',
    'W','2','5','Q','5','1','2',' ',' ',' ',' ',' ',' ',' ',' ',' ',
    '1','.','0','0'
};

static USBD_STORAGE_cb_TypeDef usb_storage_fops = {
    storage_init,
    storage_get_capacity,
    storage_is_ready,
    storage_is_write_protected,
    storage_read,
    storage_write,
    storage_get_max_lun,
    (int8_t *)inquiry_data
};

/* ==================== USBManager 方法实现 ==================== */

void USBManager::registerComponent()
{
    if (usb_reg) return;

    if (w25q_init()) {
        usb_reg = true;
    }
}

void USBManager::begin()
{
    if (usb_active) return;

    if (!s_first_init) {
        usb_debug_mark_done();
    }
    s_first_init = false;

    w25q_failed = false;

    bool need_msc = ui.param[USB_ENABLE];

    if (need_msc) {
        if (!usb_reg) {
            registerComponent();
        }
        if (!usb_reg) {
            w25q_failed = true;
            usb_poll_timeout = USB_POLL_ERROR_TIMEOUT;
            usb_pending = true;
            return;
        }
        USBD_STORAGE_fops = &usb_storage_fops;
    }

    if (ui.param[HID_ENABLE_SW] && !ui.param[USB_ENABLE]) {
        usb_debug_set_title("USB HID Init");
        usbd_hid_reinit();
    } else if (ui.param[USB_ENABLE] && ui.param[HID_ENABLE_SW]) {
        usb_debug_set_title("USB Composite Init");
        usbd_composite_reinit();
    } else {
        usb_debug_set_title("USB MSC Init");
        usbd_msc_reinit();
    }
    usb_active = true;
    usb_poll_timeout = USB_POLL_TIMEOUT;
    usb_pending = true;
}

/*
 * 非阻塞轮询 — 在 loop() 中定期调用
 * 每次调用检查一次枚举状态，配合 yield() 让出 CPU
 * 返回 true 表示枚举已完成（成功或超时）
 */
bool USBManager::poll()
{
    if (!usb_pending) return false;

    /* 让后台任务有机会运行 */
    yield();

    /* W25Q512 初始化失败 —— 直接显示错误并退出 */
    if (w25q_failed) {
        usb_pending = false;
        usb_debug_set_step(USB_STEP_DONE, USB_STATUS_FAIL);
        char err_msg[40];
        const char *id_str = w25q_get_last_id_str();
        if (id_str && id_str[0]) {
            snprintf(err_msg, sizeof(err_msg), "W25Q fail  %s", id_str);
        } else {
            snprintf(err_msg, sizeof(err_msg), "W25Q512 not found!");
        }
        usb_debug_final(false, err_msg, NULL);
        return true;
    }

    if (usbd_msc_is_mounted()) {
        usb_active = true;
        usb_pending = false;
        usb_debug_set_step(USB_STEP_DONE, USB_STATUS_OK);
        const char *ready_msg;
        if (ui.param[HID_ENABLE_SW] && !ui.param[USB_ENABLE]) {
            ready_msg = "USB HID Ready";
        } else if (ui.param[USB_ENABLE] && ui.param[HID_ENABLE_SW]) {
            ready_msg = "USB Composite Ready";
        } else {
            ready_msg = "USB MSC Ready";
        }
        usb_debug_final(true, ready_msg, NULL);
        return true;
    }

    if (usb_poll_timeout > 0) {
        usb_poll_timeout--;
    } else {
        /* 超时 — 读取寄存器快照辅助诊断 */
        usb_pending = false;
        usb_debug_set_step(USB_STEP_DONE, USB_STATUS_FAIL);
        usb_regs_t regs;
        usb_debug_read_regs(&regs);
        usb_debug_final(false, "Enumeration timeout!", &regs);
        return true;
    }

    return false;  /* 仍在等待 */
}

void USBManager::end()
{
    if (!usb_active) return;

    usbd_msc_disconnect();
    usb_active = false;
    usb_dirty = false;
}

bool USBManager::isEnabled()
{
    return usb_active && (USB_OTG_dev.dev.device_status == USB_OTG_CONFIGURED);
}

bool USBManager::isDirty()
{
    return usb_dirty;
}

#endif /* USB_MSC_ENABLE */
