#ifndef USB_MANAGER_H
#define USB_MANAGER_H

#include "config.h"

#if USB_MSC_ENABLE

#define USB_DISK_SIZE           (64U * 1024U)
#define USB_DISK_BLOCK_SIZE     512U
#define USB_DISK_BLOCK_COUNT    (USB_DISK_SIZE / USB_DISK_BLOCK_SIZE)
#define USB_DISK_FLASH_ADDR     0x080C0000U

class USBManager {
public:
    static void registerComponent();
    static void begin();
    static void end();
    static bool isEnabled();
    static void flushToFlash();
    static void loadFromFlash();
};

#else

class USBManager {
public:
    static void registerComponent() {}
    static void begin() {}
    static void end() {}
    static bool isEnabled() { return false; }
    static void flushToFlash() {}
    static void loadFromFlash() {}
};

#endif

#endif
