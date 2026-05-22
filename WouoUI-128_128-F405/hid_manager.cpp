#include "hid_manager.h"

#if HID_ENABLE

/************************************* USB模拟 *************************************/

USBHID HID;

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
