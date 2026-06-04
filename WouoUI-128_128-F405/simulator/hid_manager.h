#ifndef HID_MANAGER_H
#define HID_MANAGER_H

// Simulator version: config.h include removed
// The original hid_manager.h does #include "config.h", but
// CONFIG_H is defined globally by the force-include mechanism,
// so the Arduino/U8g2 types in config.h are not needed here.

/*
 * USB HID复合设备 — STM32 HAL USBD HID实现
 *
 * 使用单个HID接口实现Consumer（多媒体）+ Keyboard（键盘）复合设备
 * 通过Report ID区分两种报告类型：
 *   Report ID 1: Consumer Control（音量+/-, 亮度+/-, 静音等）
 *   Report ID 2: Keyboard（6键无冲 + 修饰键）
 */

/* ==================== Consumer 使用码 ==================== */
#define HID_USAGE_VOLUME_UP      0x00E9
#define HID_USAGE_VOLUME_DOWN    0x00EA
#define HID_USAGE_MUTE           0x00E2
#define HID_USAGE_BRIGHTNESS_UP  0x006F
#define HID_USAGE_BRIGHTNESS_DOWN 0x0070

/* ==================== HID Consumer 类 ==================== */
class HIDConsumer {
public:
    void press(unsigned short usage);
    void release();

    static const unsigned short VOLUME_UP       = HID_USAGE_VOLUME_UP;
    static const unsigned short VOLUME_DOWN     = HID_USAGE_VOLUME_DOWN;
    static const unsigned short BRIGHTNESS_UP   = HID_USAGE_BRIGHTNESS_UP;
    static const unsigned short BRIGHTNESS_DOWN = HID_USAGE_BRIGHTNESS_DOWN;

private:
    unsigned short _usage;
};

/* ==================== HID Keyboard 类 ==================== */
class HIDKeyboard {
public:
    HIDKeyboard();

    void press(unsigned char keycode);
    void release(unsigned char keycode);
    void releaseAll();

private:
    unsigned char _modifier;
    unsigned char _keys[6];
    void _send_report();
};

/* ==================== 外部声明 ==================== */
#define HID_REPORT_DESC_LEN 90

extern const unsigned char hid_report_desc[HID_REPORT_DESC_LEN];
extern const unsigned short hid_report_desc_len;

extern HIDConsumer Consumer;
extern HIDKeyboard Keyboard;

void hid_init();

#endif
