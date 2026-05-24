/*
 * EEPROM.h - STM32F405 Flash模拟EEPROM实现
 * 
 * 使用STM32F405的Flash存储器最后一个扇区（11号扇区，地址0x080FC000）
 * 来模拟EEPROM的功能，实现配置参数的持久化存储。
 * 
 * Flash特性：
 * - 必须按扇区擦除（64KB大小）
 * - 可逐字编程（32位）
 * - 擦除后所有位为1（0xFFFFFFFF）
 * - 写入前必须解锁，写入后重新锁定
 */

#ifndef EEPROM_h
#define EEPROM_h

#include "Arduino.h"
#include <string.h>

//Flash模拟EEPROM的起始地址：使用最后一个扇区（扇区11）
#define EEPROM_START_ADDR    0x080FC000
//模拟EEPROM的大小：512字节
#define EEPROM_SIZE          512

/************************************* Flash寄存器定义 *************************************/

//Flash控制器外设基地址
#define FLASH_BASE           0x40023C00U
//Flash密钥寄存器（解锁用）
#define FLASH_KEYR          (*(volatile uint32_t*)(FLASH_BASE + 0x04))
//Flash状态寄存器（含忙标志）
#define FLASH_SR            (*(volatile uint32_t*)(FLASH_BASE + 0x0C))
//Flash控制寄存器（含擦除/编程使能）
#define FLASH_CR            (*(volatile uint32_t*)(FLASH_BASE + 0x10))

//Flash解锁密钥1
#define FLASH_KEY1           0x45670123U
//Flash解锁密钥2
#define FLASH_KEY2           0xCDEF89ABU
//状态寄存器忙标志位
#define FLASH_SR_BSY         0x00010000U
//控制寄存器锁定标志位
#define FLASH_CR_LOCK        0x80000000U
//控制寄存器编程使能位
#define FLASH_CR_PG          0x00000001U
//控制寄存器扇区擦除使能位
#define FLASH_CR_SER         0x00000002U
//控制寄存器启动操作位
#define FLASH_CR_STRT        0x00010000U
//控制寄存器扇区编号位域
#define FLASH_CR_SNB         0x00000078U
//控制寄存器编程大小位域
#define FLASH_CR_PSIZE       0x00000300U
//编程大小：32位字
#define FLASH_PSIZE_WORD     0x00000200U

/*
 * 等待Flash操作完成
 * 
 * 检查FLASH_SR寄存器的BSY（忙）标志位
 * Flash擦除或编程操作期间BSY置1，完成后清零
 * 在执行擦除或编程命令后必须等待BSY位清零
 */
static void eeprom_wait_busy(void) {
  while ((FLASH_SR & FLASH_SR_BSY) != 0U);
}

/*
 * 解锁Flash控制器
 * 
 * STM32F405的Flash控制器默认处于锁定状态
 * 需要通过FLASH_KEYR写入两个密钥（KEY1和KEY2）来解锁
 * 只有在解锁后才能执行擦除和编程操作
 */
static void eeprom_flash_unlock(void) {
  if ((FLASH_CR & FLASH_CR_LOCK) != 0U) {
    FLASH_KEYR = FLASH_KEY1;
    FLASH_KEYR = FLASH_KEY2;
  }
}

/*
 * 锁定Flash控制器
 * 
 * 擦除/编程操作完成后立即重新锁定，防止意外写入
 */
static void eeprom_flash_lock(void) {
  FLASH_CR |= FLASH_CR_LOCK;
}

/*
 * 将数据写入Flash模拟EEPROM
 * 
 * 写入流程：
 * 1. 解锁Flash控制器
 * 2. 等待上次操作完成
 * 3. 设置编程大小（32位字）
 * 4. 擦除目标扇区（整个11号扇区）
 * 5. 逐字写入数据（每次写入32位，不足部分补0xFF）
 * 6. 清除编程标志并重新锁定
 * 
 * 参数：
 *   data - 要写入的数据缓冲区指针
 *   len - 要写入的字节数（不超过EEPROM_SIZE）
 */
static void eeprom_write_data(const uint8_t *data, uint16_t len) {
  eeprom_flash_unlock();
  eeprom_wait_busy();
  
  FLASH_CR &= ~FLASH_CR_PSIZE;
  FLASH_CR |= FLASH_PSIZE_WORD;
  
  FLASH_CR &= ~FLASH_CR_SNB;
  FLASH_CR |= (11U << 3);
  FLASH_CR |= FLASH_CR_SER;
  FLASH_CR |= FLASH_CR_STRT;
  eeprom_wait_busy();
  FLASH_CR &= ~FLASH_CR_SER;
  
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

/*
 * 从Flash读取EEPROM数据
 * 
 * 直接从Flash地址空间读取数据
 * Flash读取不需要解锁，可以直接通过指针访问
 * 
 * 参数：
 *   data - 数据接收缓冲区指针
 *   len - 要读取的字节数
 */
static void eeprom_read_data(uint8_t *data, uint16_t len) {
  memcpy(data, (const uint8_t*)EEPROM_START_ADDR, len);
}

/*
 * EEPROM类：提供类似标准EEPROM.h的接口
 * 
 * 在内存中维护一个512字节的缓冲区
 * 初始化时从Flash加载数据到缓冲区
 * 修改时只改缓冲区，commit()时才写入Flash
 * 这种缓存机制减少了Flash擦写次数，延长Flash寿命
 */
struct EEPROMClass {
  uint8_t buffer[EEPROM_SIZE];  //内存缓冲区
  bool    initialized;          //初始化标志
  
  EEPROMClass() : initialized(false) {}
  
  //初始化：从Flash读取数据到缓冲区
  void begin(int size = 0) {
    (void)size;
    eeprom_read_data(buffer, EEPROM_SIZE);
    initialized = true;
  }
  
  //读取一个字节（从缓冲区读取）
  uint8_t read(int idx) {
    if (!initialized || idx < 0 || idx >= EEPROM_SIZE) return 0xFF;
    return buffer[idx];
  }
  
  //写入一个字节（写入缓冲区，不立即写入Flash）
  void write(int idx, uint8_t val) {
    if (!initialized || idx < 0 || idx >= EEPROM_SIZE) return;
    buffer[idx] = val;
  }
  
  //更新一个字节（兼容接口，与write相同）
  void update(int idx, uint8_t val) {
    write(idx, val);
  }
  
  //提交更改：将缓冲区数据真正写入Flash
  void commit(void) {
    if (!initialized) return;
    eeprom_write_data(buffer, EEPROM_SIZE);
  }
  
  //返回EEPROM大小
  uint16_t length() { return EEPROM_SIZE; }
};

//全局EEPROM实例
static EEPROMClass EEPROM;
#endif
