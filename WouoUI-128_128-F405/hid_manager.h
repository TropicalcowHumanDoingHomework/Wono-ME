#ifndef HID_MANAGER_H
#define HID_MANAGER_H

#include "config.h"

#if HID_ENABLE

#include <USBComposite.h>

/************************************* USB模拟 *************************************/

extern USBHID HID;
extern HIDConsumer Consumer;
extern HIDKeyboard Keyboard;

void hid_init();

#endif

#endif
