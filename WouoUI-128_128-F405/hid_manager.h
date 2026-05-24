#ifndef HID_MANAGER_H
#define HID_MANAGER_H

#include "config.h"

#if HID_ENABLE

#include <USBComposite.h>

/************************************* USB HID键盘与消费者设备 *************************************/

/*
 * USB HID基础设备对象
 * 管理复合HID设备的报告描述符和通信通道
 * 作为Consumer和Keyboard的底层通信载体
 */
extern USBHID HID;

/*
 * USB HID消费者设备
 * 发送多媒体控制指令（音量+、音量-、静音、播放/暂停等）
 * 通过HID Consumer Control报告描述符实现
 * 在旋钮旋转功能设置为VOL时使用
 */
extern HIDConsumer Consumer;

/*
 * USB HID键盘设备
 * 发送标准键盘按键（字母A-Z、数字、功能键、方向键等）
 * 通过HID Keyboard报告描述符实现
 * 支持6键无冲
 * 在旋钮按键功能设置时选择按键映射
 */
extern HIDKeyboard Keyboard;

/*
 * HID初始化函数
 * 注册Consumer和Keyboard的复合报告描述符
 * 等待USB枚举完成
 * 在setup()中调用
 */
void hid_init();

#endif

#endif
