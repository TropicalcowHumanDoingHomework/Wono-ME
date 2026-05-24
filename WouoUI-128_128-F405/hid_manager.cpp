/*
 * USB HID复合设备实现
 * 
 * 使用USBComposite库构建复合HID设备：
 * - HIDConsumer：发送多媒体控制指令（音量调节等）
 * - HIDKeyboard：发送标准键盘按键
 * 
 * 两个设备共享同一个USB HID接口
 * 通过复合报告描述符实现多设备共存
 * 
 * 使用场景：
 * - 旋钮旋转功能设为VOL时，旋钮旋转发送音量+/-指令
 * - 睡眠模式下，旋钮按键发送预设键码
 */

#include "hid_manager.h"

#if HID_ENABLE

/************************************* USB HID设备实例化 *************************************/

USBHID HID;

/*
 * 复合报告描述符
 * 同时包含Consumer和Keyboard的报告描述符
 * 使USB主机能同时识别为多媒体控制器和键盘
 */
const uint8_t reportDescription[] = {
    HID_CONSUMER_REPORT_DESCRIPTOR(),
    HID_KEYBOARD_REPORT_DESCRIPTOR()
};

HIDConsumer Consumer(HID);
HIDKeyboard Keyboard(HID);

void hid_init() {
    HID.begin(reportDescription, sizeof(reportDescription));
    while (!USBComposite);
}

#endif
