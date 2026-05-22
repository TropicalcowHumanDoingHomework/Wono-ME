/*
  EEPROM.h - 简单的 Flash 模拟 EEPROM
  用于 STM32F405
*/

#ifndef EEPROM_h
#define EEPROM_h

#include "Arduino.h"
#include <string.h>

// 使用 STM32 的 Flash 模拟 EEPROM
// 存储地址：Flash 最后一个扇区（11号扇区，地址 0x080FC000）
#define EEPROM_START_ADDR    0x080FC000
#define EEPROM_SIZE          512

// Flash 寄存器定义
#define FLASH_BASE           0x40023C00U
#define FLASH_KEYR          (*(volatile uint32_t*)(FLASH_BASE + 0x04))
#define FLASH_SR            (*(volatile uint32_t*)(FLASH_BASE + 0x0C))
#define FLASH_CR            (*(volatile uint32_t*)(FLASH_BASE + 0x10))

#define FLASH_KEY1           0x45670123U
#define FLASH_KEY2           0xCDEF89ABU
#define FLASH_SR_BSY         0x00010000U
#define FLASH_CR_LOCK        0x80000000U
#define FLASH_CR_PG          0x00000001U
#define FLASH_CR_SER         0x00000002U
#define FLASH_CR_STRT        0x00010000U
#define FLASH_CR_SNB         0x00000078U
#define FLASH_CR_PSIZE       0x00000300U
#define FLASH_PSIZE_WORD     0x00000200U

// 等待 Flash 操作完成
static void eeprom_wait_busy(void) {
  while ((FLASH_SR & FLASH_SR_BSY) != 0U);
}

// 解锁 Flash
static void eeprom_flash_unlock(void) {
  if ((FLASH_CR & FLASH_CR_LOCK) != 0U) {
    FLASH_KEYR = FLASH_KEY1;
    FLASH_KEYR = FLASH_KEY2;
  }
}

// 锁定 Flash
static void eeprom_flash_lock(void) {
  FLASH_CR |= FLASH_CR_LOCK;
}

// 写入数据到 EEPROM
static void eeprom_write_data(const uint8_t *data, uint16_t len) {
  eeprom_flash_unlock();
  eeprom_wait_busy();
  
  // 设置 PSIZE 为 32位
  FLASH_CR &= ~FLASH_CR_PSIZE;
  FLASH_CR |= FLASH_PSIZE_WORD;
  
  // 先擦除扇区
  FLASH_CR &= ~FLASH_CR_SNB;
  FLASH_CR |= (11U << 3);
  FLASH_CR |= FLASH_CR_SER;
  FLASH_CR |= FLASH_CR_STRT;
  eeprom_wait_busy();
  FLASH_CR &= ~FLASH_CR_SER;
  
  // 再写入数据
  FLASH_CR |= FLASH_CR_PG;
  uint32_t addr = EEPROM_START_ADDR;
  for (uint16_t i = 0; i < EEPROM_SIZE; i += 4) {
    uint32_t word = 0xFFFFFFFF;
    if (i + 0 < len) word = (uint32_t)data[i + 0];
    if (i + 1 < len) word |= (uint32_t)data[i + 1] << 8;
    if (i + 2 < len) word |= (uint32_t)data[i + 2] << 16;
    if (i + 3 < len) word |= (uint32_t)data[i + 3] << 24;
    *(volatile uint32_t*)addr = word;
    eeprom_wait_busy();
    addr += 4;
  }
  
  FLASH_CR &= ~FLASH_CR_PG;
  eeprom_flash_lock();
}

// 读取数据从 EEPROM
static void eeprom_read_data(uint8_t *data, uint16_t len) {
  memcpy(data, (const uint8_t*)EEPROM_START_ADDR, len);
}

// 简单的 EEPROM 类
struct EEPROMClass {
  uint8_t buffer[EEPROM_SIZE];
  bool    initialized;
  
  EEPROMClass() : initialized(false) {}
  
  void begin(int size = 0) {
    (void)size;
    eeprom_read_data(buffer, EEPROM_SIZE);
    initialized = true;
  }
  
  uint8_t read(int idx) {
    if (!initialized || idx < 0 || idx >= EEPROM_SIZE) return 0xFF;
    return buffer[idx];
  }
  
  void write(int idx, uint8_t val) {
    if (!initialized || idx < 0 || idx >= EEPROM_SIZE) return;
    buffer[idx] = val;
  }
  
  void update(int idx, uint8_t val) {
    write(idx, val);
  }
  
  void commit(void) {
    if (!initialized) return;
    eeprom_write_data(buffer, EEPROM_SIZE);
  }
  
  // 兼容接口
  uint16_t length() { return EEPROM_SIZE; }
};

static EEPROMClass EEPROM;
#endif
