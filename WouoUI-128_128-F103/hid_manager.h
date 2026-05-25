#ifndef HID_MANAGER_H
#define HID_MANAGER_H

#include "config.h"

#include <USBComposite.h>

/************************************* USB模拟 *************************************/

extern USBHID HID;
extern HIDConsumer Consumer;
extern HIDKeyboard Keyboard;

void hid_init();

#endif