#ifndef HID_MANAGER_H
#define HID_MANAGER_H

#include "config.h"

/*
 * USB HID复合设备 — STM32 HAL USBD HID实现
 *
 * 使用单个HID接口实现Consumer（多媒体）+ Keyboard（键盘）复合设备
 * 通过Report ID区分两种报告类型：
 *   Report ID 1: Consumer Control（音量+/-, 亮度+/-, 静音等）
 *   Report ID 2: Keyboard（6键无冲 + 修饰键）
 *
 * 当前状态：
 * - HID_ENABLE=0（默认）时，所有方法为空操作，不依赖任何USB库
 * - HID_ENABLE=1 需要对应的HID类驱动（usbd_hid），与MSC类似的独立实现
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
    /* 发送多媒体按键（按下的瞬间发送，release发送0=无按键） */
    void press(uint16_t usage);
    void release();

    /* 常量别名，保持API兼容 */
    static const uint16_t VOLUME_UP       = HID_USAGE_VOLUME_UP;
    static const uint16_t VOLUME_DOWN     = HID_USAGE_VOLUME_DOWN;
    static const uint16_t BRIGHTNESS_UP   = HID_USAGE_BRIGHTNESS_UP;
    static const uint16_t BRIGHTNESS_DOWN = HID_USAGE_BRIGHTNESS_DOWN;

private:
    uint16_t _usage;
};

/* ==================== HID Keyboard 类 ==================== */
class HIDKeyboard {
public:
    HIDKeyboard();

    /* 按下按键（等待release后释放） */
    void press(uint8_t keycode);
    /* 释放按键 */
    void release(uint8_t keycode);
    /* 释放所有按键 */
    void releaseAll();

private:
    uint8_t _modifier;       /* 修饰键位掩码 */
    uint8_t _keys[6];        /* 最多6键同时按下 */
    void _send_report();     /* 发送键盘报告 */
};

/* ==================== 外部声明 ==================== */

/* HID报告描述符长度（Consumer 25B + Keyboard 65B = 90B） */
#define HID_REPORT_DESC_LEN 90

/* HID报告描述符（在hid_manager.cpp中定义） */
extern const uint8_t hid_report_desc[HID_REPORT_DESC_LEN];
extern const uint16_t hid_report_desc_len;

/* 全局HID设备对象 */
extern HIDConsumer Consumer;
extern HIDKeyboard Keyboard;

/* HID初始化 */
void hid_init();

#endif
