#ifndef HID_MANAGER_H
#define HID_MANAGER_H

#include "config.h"

#if HID_ENABLE

#include <USBComposite.h>

/************************************* USB HID模拟 *************************************/

//USB HID设备对象：管理复合HID设备
extern USBHID HID;
//USB HID消费者设备对象：发送多媒体键（音量等）
extern HIDConsumer Consumer;
//USB HID键盘设备对象：发送键盘按键
extern HIDKeyboard Keyboard;

//HID初始化函数：注册HID报告描述符并启动
void hid_init();

#endif

#endif
