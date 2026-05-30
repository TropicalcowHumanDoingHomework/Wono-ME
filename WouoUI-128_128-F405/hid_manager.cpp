/*
 * USB HID复合设备实现 — STM32 HAL USBD HID
 *
 * 单个HID接口承载Consumer Control（多媒体）+ Keyboard（键盘）
 * 通过Report ID区分：
 *   Report ID 1: Consumer Control（16位使用码）
 *   Report ID 2: Keyboard（8位修饰键 + 6键数组）
 *
 * 当前状态：
 * - HID_ENABLE=0：所有USB发送操作被跳过，仅更新内部状态
 * - HID_ENABLE=1：需要对应的HID类驱动（usbd_hid）实现后启用
 */

#include "hid_manager.h"
#include <string.h>

/* ==================== HID报告描述符 ==================== */

/*
 * Consumer Control + Keyboard 复合报告描述符
 *
 * 字节排列说明：
 *   Consumer部分：Report ID=1, 16位usage code
 *   Keyboard部分：Report ID=2, 1B修饰键 + 1B保留 + 6B键码
 */
const uint8_t hid_report_desc[HID_REPORT_DESC_LEN] = {
    /* ---- Consumer Control ---- */
    0x05, 0x0C,        /* Usage Page (Consumer Devices) */
    0x09, 0x01,        /* Usage (Consumer Control) */
    0xA1, 0x01,        /* Collection (Application) */
    0x85, 0x01,        /*   Report ID (1) */
    0x15, 0x00,        /*   Logical Minimum (0) */
    0x26, 0xFF, 0x03,  /*   Logical Maximum (1023) */
    0x19, 0x00,        /*   Usage Minimum (0) */
    0x2A, 0xFF, 0x03,  /*   Usage Maximum (1023) */
    0x75, 0x10,        /*   Report Size (16) */
    0x95, 0x01,        /*   Report Count (1) */
    0x81, 0x00,        /*   Input (Data, Array, Abs) */
    0xC0,              /* End Collection */

    /* ---- Keyboard ---- */
    0x05, 0x01,        /* Usage Page (Generic Desktop) */
    0x09, 0x06,        /* Usage (Keyboard) */
    0xA1, 0x01,        /* Collection (Application) */
    0x85, 0x02,        /*   Report ID (2) */
    /* 修饰键位图（8位） */
    0x05, 0x07,        /*   Usage Page (Keyboard/Keypad) */
    0x19, 0xE0,        /*   Usage Minimum (224: Left Ctrl) */
    0x29, 0xE7,        /*   Usage Maximum (231: Right GUI) */
    0x15, 0x00,        /*   Logical Minimum (0) */
    0x25, 0x01,        /*   Logical Maximum (1) */
    0x75, 0x01,        /*   Report Size (1) */
    0x95, 0x08,        /*   Report Count (8) */
    0x81, 0x02,        /*   Input (Data, Var, Abs) */
    /* 保留字节 */
    0x75, 0x08,        /*   Report Size (8) */
    0x95, 0x01,        /*   Report Count (1) */
    0x81, 0x01,        /*   Input (Const) */
    /* 6键数组 */
    0x15, 0x00,        /*   Logical Minimum (0) */
    0x25, 0x65,        /*   Logical Maximum (101) */
    0x75, 0x08,        /*   Report Size (8) */
    0x95, 0x06,        /*   Report Count (6) */
    0x05, 0x07,        /*   Usage Page (Keyboard/Keypad) */
    0x19, 0x00,        /*   Usage Minimum (0) */
    0x29, 0x65,        /*   Usage Maximum (101) */
    0x81, 0x00,        /*   Input (Data, Array) */
    /* 键盘LED输出（5位） */
    0x75, 0x01,        /*   Report Size (1) */
    0x95, 0x05,        /*   Report Count (5) */
    0x05, 0x08,        /*   Usage Page (LEDs) */
    0x19, 0x01,        /*   Usage Minimum (Num Lock) */
    0x29, 0x05,        /*   Usage Maximum (Kana) */
    0x91, 0x02,        /*   Output (Data, Var, Abs) */
    /* 填充对齐 */
    0x75, 0x03,        /*   Report Size (3) */
    0x95, 0x01,        /*   Report Count (1) */
    0x91, 0x01,        /*   Output (Const) */
    0xC0,              /* End Collection */
};

const uint16_t hid_report_desc_len = sizeof(hid_report_desc);

/* ==================== HIDConsumer 实现 ==================== */

void HIDConsumer::press(uint16_t usage)
{
    _usage = usage;
#if HID_ENABLE
    /* Consumer报告格式: [ReportID=1][Usage低字节][Usage高字节]
     * TODO: 实现HID类驱动后，通过USBD_HID_SendReport发送 */
    // uint8_t report[3] = {0x01, (uint8_t)(usage & 0xFF), (uint8_t)(usage >> 8)};
    // HID发送report...
#endif
}

void HIDConsumer::release()
{
    _usage = 0;
#if HID_ENABLE
    /* TODO: 发送usage=0的释放报告 */
    // uint8_t report[3] = {0x01, 0x00, 0x00};
    // HID发送report...
#endif
}

/* ==================== HIDKeyboard 实现 ==================== */

HIDKeyboard::HIDKeyboard()
    : _modifier(0)
{
    memset(_keys, 0, sizeof(_keys));
}

void HIDKeyboard::press(uint8_t keycode)
{
    /* 修饰键（0xE0-0xE7）→ 位掩码 */
    if (keycode >= 0xE0 && keycode <= 0xE7) {
        _modifier |= (1 << (keycode - 0xE0));
    }
    /* 普通键码（0-101）→ 6键数组 */
    else if (keycode <= 101) {
        /* 查找空位或已存在的位置 */
        uint8_t slot = 0xFF;
        for (uint8_t i = 0; i < 6; i++) {
            if (_keys[i] == keycode) {
                slot = i;   /* 已存在，不用添加 */
                break;
            }
            if (_keys[i] == 0 && slot == 0xFF) {
                slot = i;   /* 空位 */
            }
        }
        if (slot != 0xFF && _keys[slot] != keycode) {
            _keys[slot] = keycode;
        }
    }
    _send_report();
}

void HIDKeyboard::release(uint8_t keycode)
{
    /* 修饰键释放 */
    if (keycode >= 0xE0 && keycode <= 0xE7) {
        _modifier &= ~(1 << (keycode - 0xE0));
    }
    /* 普通键码释放 */
    else if (keycode <= 101) {
        for (uint8_t i = 0; i < 6; i++) {
            if (_keys[i] == keycode) {
                _keys[i] = 0;
                break;
            }
        }
        /* 压缩数组，移除空洞 */
        uint8_t wr = 0;
        for (uint8_t rd = 0; rd < 6; rd++) {
            if (_keys[rd] != 0) {
                _keys[wr++] = _keys[rd];
            }
        }
        while (wr < 6) {
            _keys[wr++] = 0;
        }
    }
    _send_report();
}

void HIDKeyboard::releaseAll()
{
    _modifier = 0;
    memset(_keys, 0, sizeof(_keys));
    _send_report();
}

/*
 * 发送键盘报告
 * 格式: [ReportID=2][Modifier][Reserved=0][Key0]...[Key5]
 */
void HIDKeyboard::_send_report()
{
#if HID_ENABLE
    /* TODO: 实现HID类驱动后，通过USBD_HID_SendReport发送 */
    // uint8_t report[9];
    // report[0] = 0x02;
    // report[1] = _modifier;
    // report[2] = 0x00;
    // memcpy(&report[3], _keys, 6);
    // HID发送report...
#else
    (void)_modifier;  /* 未使用时消除警告 */
#endif
}

/* ==================== 全局对象 ==================== */

HIDConsumer Consumer;
HIDKeyboard Keyboard;

/* ==================== HID初始化 ==================== */

void hid_init()
{
    /* HID初始化预留。
     * 当HID_ENABLE=1且HID类驱动就绪时：
     * 1. 注册HID报告描述符
     * 2. 启用HID端点
     */
}

