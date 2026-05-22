#include "usb_manager.h"

#if USB_MSC_ENABLE

#include <USBComposite.h>
#include <string.h>

#define FLASH_BASE            0x40023C00U
#define FLASH_KEYR            (*(volatile uint32_t*)(FLASH_BASE + 0x04))
#define FLASH_SR              (*(volatile uint32_t*)(FLASH_BASE + 0x0C))
#define FLASH_CR              (*(volatile uint32_t*)(FLASH_BASE + 0x10))

#define FLASH_KEY1            0x45670123U
#define FLASH_KEY2            0xCDEF89ABU
#define FLASH_SR_BSY          0x00010000U
#define FLASH_CR_LOCK         0x80000000U
#define FLASH_CR_PG           0x00000001U
#define FLASH_CR_SER          0x00000002U
#define FLASH_CR_STRT         0x00010000U
#define FLASH_CR_PSIZE        0x00000300U
#define FLASH_PSIZE_WORD      0x00000200U

static uint8_t usb_disk_buffer[USB_DISK_SIZE];
static USBMassStorage usb_msc;
static bool usb_active = false;
static bool usb_registered = false;

static void flash_wait_busy(void) {
    while ((FLASH_SR & FLASH_SR_BSY) != 0U) {}
}

static void flash_unlock(void) {
    if ((FLASH_CR & FLASH_CR_LOCK) != 0U) {
        FLASH_KEYR = FLASH_KEY1;
        FLASH_KEYR = FLASH_KEY2;
    }
}

static void flash_lock(void) {
    FLASH_CR |= FLASH_CR_LOCK;
}

static void flash_erase_sector(uint32_t sector_addr) {
    uint32_t snb;
    flash_unlock();
    flash_wait_busy();

    if (sector_addr < 0x08010000U) {
        snb = (sector_addr - 0x08000000U) / 0x4000U;
    }
    else if (sector_addr < 0x08020000U) {
        snb = 4U;
    }
    else {
        snb = 5U + (sector_addr - 0x08020000U) / 0x20000U;
    }

    FLASH_CR &= ~FLASH_CR_PSIZE;
    FLASH_CR |= FLASH_PSIZE_WORD;
    FLASH_CR &= ~(0x78U);
    FLASH_CR |= (snb << 3U);
    FLASH_CR |= FLASH_CR_SER;
    FLASH_CR |= FLASH_CR_STRT;
    flash_wait_busy();
    FLASH_CR &= ~FLASH_CR_SER;

    flash_lock();
}

static void flash_program_word(uint32_t addr, uint32_t data) {
    uint32_t *p;
    flash_unlock();
    flash_wait_busy();

    FLASH_CR &= ~FLASH_CR_PSIZE;
    FLASH_CR |= FLASH_PSIZE_WORD;
    FLASH_CR |= FLASH_CR_PG;

    p = (uint32_t *)addr;
    *p = data;
    flash_wait_busy();

    FLASH_CR &= ~FLASH_CR_PG;
    flash_lock();
}

static uint32_t msc_get_capacity(void) {
    return USB_DISK_BLOCK_COUNT;
}

static uint32_t msc_read(uint8_t *buf, uint32_t offset, uint32_t size) {
    if (offset + size > USB_DISK_SIZE) {
        size = USB_DISK_SIZE - offset;
    }
    memcpy(buf, usb_disk_buffer + offset, size);
    return size;
}

static uint32_t msc_write(const uint8_t *buf, uint32_t offset, uint32_t size) {
    if (offset + size > USB_DISK_SIZE) {
        size = USB_DISK_SIZE - offset;
    }
    memcpy(usb_disk_buffer + offset, buf, size);
    return size;
}

void USBManager::registerComponent() {
    if (usb_registered) {
        return;
    }

    loadFromFlash();

    usb_msc.setCapacityCallback(msc_get_capacity);
    usb_msc.setReadCallback(msc_read);
    usb_msc.setWriteCallback(msc_write);
    usb_msc.registerComponent();

    usb_registered = true;
}

void USBManager::begin() {
    if (usb_active) {
        return;
    }

    if (!usb_registered) {
        registerComponent();
    }

    USBComposite.begin();
    usb_active = true;
}

void USBManager::end() {
    if (!usb_active) {
        return;
    }

    flushToFlash();
    USBComposite.end();
    usb_active = false;
    usb_registered = false;
}

bool USBManager::isEnabled() {
    return usb_active;
}

void USBManager::flushToFlash() {
    uint32_t addr;
    uint32_t *src;
    uint32_t word_count;
    uint32_t i;

    flash_erase_sector(USB_DISK_FLASH_ADDR);

    addr = USB_DISK_FLASH_ADDR;
    src = (uint32_t *)usb_disk_buffer;
    word_count = USB_DISK_SIZE / 4U;

    for (i = 0U; i < word_count; i++) {
        flash_program_word(addr, src[i]);
        addr += 4U;
    }
}

void USBManager::loadFromFlash() {
    uint32_t addr;
    uint32_t *dst;
    uint32_t word_count;
    uint32_t i;
    uint32_t blank;

    addr = USB_DISK_FLASH_ADDR;
    blank = 0xFFFFFFFFU;

    if (*(volatile uint32_t *)addr == blank && *(volatile uint32_t *)(addr + 4U) == blank) {
        memset(usb_disk_buffer, 0x00, USB_DISK_SIZE);
        return;
    }

    dst = (uint32_t *)usb_disk_buffer;
    word_count = USB_DISK_SIZE / 4U;

    for (i = 0U; i < word_count; i++) {
        dst[i] = *(volatile uint32_t *)addr;
        addr += 4U;
    }
}

#endif
