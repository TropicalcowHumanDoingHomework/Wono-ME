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
 * - HID_ENABLE=1：通过usbd_hid类驱动发送真实HID报告
 */

#include "hid_manager.h"
#include <string.h>

#if HID_ENABLE
#include "usbd_hid.h"
#include "ui_state.h"
#endif

/*
 * Arduino Keyboard库键码 → HID Keyboard Usage ID 转换
 *
 * KPF菜单存储的是Arduino Keyboard库键码，但HID报告
 * 需要USB HID Keyboard Usage ID（Usage Page 0x07）。
 * 本函数做转换映射。
 */
static uint8_t arduino_to_hid_usage(uint8_t key)
{
    /* 修饰键 0xE0-0xE8 直接透传（HIDKeyboard内部做位掩码处理） */
    if (key >= 0xE0 && key <= 0xE8) return key;

    /* 字母 A-Z (65-90) → HID Usage 0x04-0x1D */
    if (key >= 'A' && key <= 'Z') return (uint8_t)(key - 'A' + 4);
    if (key >= 'a' && key <= 'z') return (uint8_t)(key - 'a' + 4);

    /* 数字 1-9 (49-57) → HID Usage 0x1E-0x26, 0 (48) → 0x27 */
    if (key >= '1' && key <= '9') return (uint8_t)(key - '1' + 0x1E);
    if (key == '0') return 0x27;

    /* 其他特殊键 */
    switch (key) {
    case 40:  return 0x28;   /* KEY_RETURN → Enter */
    case 41:  return 0x29;   /* KEY_ESC → Escape */
    case 42:  return 0x2A;   /* KEY_BACKSPACE */
    case 43:  return 0x2B;   /* KEY_TAB */
    case 58:  return 0x3A;   /* KEY_F1-F7: 58-64 → 0x3A-0x40 */
    case 59:  return 0x3B;
    case 60:  return 0x3C;
    case 61:  return 0x3D;
    case 62:  return 0x3E;
    case 63:  return 0x3F;
    case 64:  return 0x40;
    case 0xE9: return 0x41;  /* KEY_F8-F12: 0xE9-0xED → 0x41-0x45 */
    case 0xEA: return 0x42;
    case 0xEB: return 0x43;
    case 0xEC: return 0x44;
    case 0xED: return 0x45;
    case 73:  return 0x49;   /* KEY_INSERT */
    case 74:  return 0x4A;   /* KEY_HOME */
    case 75:  return 0x4B;   /* KEY_PAGE_UP */
    case 76:  return 0x4C;   /* KEY_DELETE */
    case 77:  return 0x4D;   /* KEY_END */
    case 78:  return 0x4E;   /* KEY_PAGE_DOWN */
    case 79:  return 0x4F;   /* KEY_RIGHT_ARROW */
    case 80:  return 0x50;   /* KEY_LEFT_ARROW */
    case 81:  return 0x51;   /* KEY_DOWN_ARROW */
    case 82:  return 0x52;   /* KEY_UP_ARROW */
    default:  return 0;      /* 未知键 → 无操作 */
    }
}

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
    uint8_t report[3] = {0x01, (uint8_t)(usage & 0xFF), (uint8_t)(usage >> 8)};
    usbd_hid_send_report(report, 3);
#endif
}

void HIDConsumer::release()
{
    _usage = 0;
#if HID_ENABLE
    uint8_t report[3] = {0x01, 0x00, 0x00};
    usbd_hid_send_report(report, 3);
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
    uint8_t hid_key = arduino_to_hid_usage(keycode);
    if (hid_key == 0) return;   /* 无效键码，跳过 */

    /* 大写模式：字母键自动施加Shift */
    bool need_shift = (knob.param[KNOB_CASE] == 1)
                   && (hid_key >= 4 && hid_key <= 29);  // HID a-z 范围
    if (need_shift) {
        _modifier |= (1 << (KEY_LEFT_SHIFT - 0xE0));
    }

    /* 修饰键（0xE0-0xE7）→ 位掩码 */
    if (hid_key >= 0xE0 && hid_key <= 0xE8) {
        _modifier |= (1 << (hid_key - 0xE0));
    }
    /* 普通键码 → 6键数组 */
    else {
        uint8_t slot = 0xFF;
        for (uint8_t i = 0; i < 6; i++) {
            if (_keys[i] == hid_key) { slot = i; break; }
            if (_keys[i] == 0 && slot == 0xFF) slot = i;
        }
        if (slot != 0xFF && _keys[slot] != hid_key) {
            _keys[slot] = hid_key;
        }
    }
    _send_report();
}

void HIDKeyboard::release(uint8_t keycode)
{
    uint8_t hid_key = arduino_to_hid_usage(keycode);
    if (hid_key == 0) return;

    /* 修饰键释放 */
    if (hid_key >= 0xE0 && hid_key <= 0xE8) {
        _modifier &= ~(1 << (hid_key - 0xE0));
    }
    /* 普通键码释放 */
    else {
        for (uint8_t i = 0; i < 6; i++) {
            if (_keys[i] == hid_key) { _keys[i] = 0; break; }
        }
        uint8_t wr = 0;
        for (uint8_t rd = 0; rd < 6; rd++) {
            if (_keys[rd] != 0) _keys[wr++] = _keys[rd];
        }
        while (wr < 6) _keys[wr++] = 0;
    }

    /* 释放大写Shift（如果之前施加了） */
    _modifier &= ~(1 << (KEY_LEFT_SHIFT - 0xE0));

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
    uint8_t report[9];
    report[0] = 0x02;
    report[1] = _modifier;
    report[2] = 0x00;
    memcpy(&report[3], _keys, 6);
    usbd_hid_send_report(report, 9);
#else
    (void)_modifier;
#endif
}

/* ==================== 全局对象 ==================== */

HIDConsumer Consumer;
HIDKeyboard Keyboard;

/* ==================== HID初始化 ==================== */

void hid_init()
{
    /* HID 初始化 — 实际驱动注册在 usbd_hid_reinit() / composite_reinit() 中完成。
     * 本函数保留供未来分离 HID 初始化与 USB 枚举流程的独立入口。 */
}

