#include "usb_manager.h"

#if USB_MSC_ENABLE

#include <USBComposite.h>
#include <string.h>

/************************************* Flash寄存器地址定义 *************************************/

/*
 * STM32F405 Flash控制器寄存器映射
 * 
 * Flash控制器基地址：0x40023C00
 * 
 * 关键寄存器：
 * - FLASH_KEYR：解锁密钥寄存器（写入两个密钥解锁）
 * - FLASH_SR：状态寄存器（含BSY忙标志和错误标志）
 * - FLASH_CR：控制寄存器（编程/擦除使能、扇区选择）
 * 
 * Flash编程注意事项：
 * 1. 必须先解锁才能操作（写入KEY1→KEY2）
 * 2. 擦除前必须设置PSIZE（编程大小）
 * 3. 擦除/编程后必须等待BSY位清零
 * 4. 操作完成后必须重新锁定
 */
#define FLASH_BASE            0x40023C00U
#define FLASH_KEYR            (*(volatile uint32_t*)(FLASH_BASE + 0x04))
#define FLASH_SR              (*(volatile uint32_t*)(FLASH_BASE + 0x0C))
#define FLASH_CR              (*(volatile uint32_t*)(FLASH_BASE + 0x10))

/* Flash解锁密钥（由芯片设计固定） */
#define FLASH_KEY1            0x45670123U
#define FLASH_KEY2            0xCDEF89ABU

/* Flash状态寄存器标志位 */
#define FLASH_SR_BSY          0x00010000U  //忙标志（操作进行中）

/* Flash控制寄存器标志位 */
#define FLASH_CR_LOCK         0x80000000U  //锁定标志（1=锁定）
#define FLASH_CR_PG           0x00000001U  //编程使能
#define FLASH_CR_SER          0x00000002U  //扇区擦除使能
#define FLASH_CR_STRT         0x00010000U  //启动操作
#define FLASH_CR_PSIZE        0x00000300U  //编程大小位域
#define FLASH_PSIZE_WORD      0x00000200U  //32位字编程

/************************************* 全局变量 *************************************/

/*
 * USB磁盘RAM缓冲区（64KB）
 * 
 * 在RAM中维护完整的磁盘映像，实现高速读写
 * USB主机对磁盘的所有读写操作都在此缓冲区进行
 * 仅在USB断开或进入睡眠时将缓冲区写入Flash持久化
 * 
 * 优点：避免每次USB读写都操作Flash（Flash擦写寿命有限）
 * 缺点：需占用64KB RAM（STM32F405有192KB，可承受）
 */
static uint8_t usb_disk_buffer[USB_DISK_SIZE];

//USB MSC设备对象（由USBComposite库提供）
static USBMassStorage usb_msc;

//USB是否已激活（设备已连接至主机）
static bool usb_active = false;

//USB MSC组件是否已注册（回调已设置）
static bool usb_registered = false;

/************************************* Flash底层操作 *************************************/

/*
 * 等待Flash操作完成
 * 
 * STM32F405的Flash擦除和编程操作需要一定时间
 * 在此期间BSY标志位置1，操作完成后自动清零
 * 必须在每次擦除/编程操作后调用此函数等待完成
 */
static void flash_wait_busy(void) {
    while ((FLASH_SR & FLASH_SR_BSY) != 0U) {}
}

/*
 * 解锁Flash控制器
 * 
 * Flash控制器默认处于锁定状态（防止意外写入）
 * 解锁流程：依次向KEYR写入KEY1和KEY2
 * 解锁后才能执行扇区擦除和编程操作
 */
static void flash_unlock(void) {
    if ((FLASH_CR & FLASH_CR_LOCK) != 0U) {
        FLASH_KEYR = FLASH_KEY1;
        FLASH_KEYR = FLASH_KEY2;
    }
}

/*
 * 重新锁定Flash控制器
 * 
 * 操作完成后立即锁定，防止代码跑飞导致Flash意外擦写
 */
static void flash_lock(void) {
    FLASH_CR |= FLASH_CR_LOCK;
}

/*
 * 擦除Flash指定扇区
 * 
 * STM32F405的Flash扇区大小不统一：
 * - 0~3号扇区：16KB
 * - 4号扇区：64KB
 * - 5~11号扇区：128KB
 * 
 * USB_DISK_FLASH_ADDR=0x080C0000落在8号扇区（128KB）
 * 扇区编号计算：
 * - 地址0x08000000~0x08010000：扇区0~3（每16KB）
 * - 地址0x08010000~0x08020000：扇区4（64KB）
 * - 地址≥0x08020000：扇区5~11（每128KB）
 * 
 * 擦除步骤：
 * 1. 解锁Flash
 * 2. 等待空闲
 * 3. 设置PSIZE=32位
 * 4. 设置SNB（扇区编号）
 * 5. 置位SER（扇区擦除使能）
 * 6. 置位STRT启动操作
 * 7. 等待BSY清零
 * 8. 清除SER标志并重新锁定
 */
static void flash_erase_sector(uint32_t sector_addr) {
    uint32_t snb;
    flash_unlock();
    flash_wait_busy();

    /* 根据地址计算扇区编号 */
    if (sector_addr < 0x08010000U) {
        snb = (sector_addr - 0x08000000U) / 0x4000U;  //16KB扇区
    }
    else if (sector_addr < 0x08020000U) {
        snb = 4U;  //64KB扇区
    }
    else {
        snb = 5U + (sector_addr - 0x08020000U) / 0x20000U;  //128KB扇区
    }

    /* 配置擦除参数并启动 */
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

/*
 * 向Flash写入一个32位字
 * 
 * STM32F405的Flash编程特点：
 * - 必须按32位字写入（不能按字节写入）
 * - 写入前目标地址必须为0xFFFFFFFF（已擦除状态）
 * - 写入后该地址不能再写入（除非重新擦除）
 * - 写入操作会自动校准电压，无需额外配置
 * 
 * 步骤：
 * 1. 解锁Flash
 * 2. 等待空闲
 * 3. 设置PSIZE=32位
 * 4. 置位PG（编程使能）
 * 5. 写入32位数据到目标地址
 * 6. 等待BSY清零
 * 7. 清除PG标志并重新锁定
 */
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

/************************************* USB MSC回调函数 *************************************/

/*
 * 报告磁盘容量（USB MSC回调）
 * 
 * USB主机在枚举设备时调用此函数
 * 返回虚拟磁盘的块数量
 * 块大小固定为512字节
 */
static uint32_t msc_get_capacity(void) {
    return USB_DISK_BLOCK_COUNT;
}

/*
 * 读取磁盘数据（USB MSC回调）
 * 
 * USB主机读取磁盘数据时调用
 * 直接从RAM缓冲区复制数据
 * 
 * 参数：
 *   buf - 数据接收缓冲区
 *   offset - 读取起始偏移
 *   size - 读取字节数
 * 
 * 返回值：实际读取的字节数
 */
static uint32_t msc_read(uint8_t *buf, uint32_t offset, uint32_t size) {
    if (offset + size > USB_DISK_SIZE) {
        size = USB_DISK_SIZE - offset;  //防止越界
    }
    memcpy(buf, usb_disk_buffer + offset, size);
    return size;
}

/*
 * 写入磁盘数据（USB MSC回调）
 * 
 * USB主机写入磁盘数据时调用
 * 数据写入RAM缓冲区（暂不写入Flash）
 * 仅在USB断开或进入睡眠时统一刷入Flash
 * 
 * 参数：
 *   buf - 待写入数据缓冲区
 *   offset - 写入起始偏移
 *   size - 写入字节数
 * 
 * 返回值：实际写入的字节数
 */
static uint32_t msc_write(const uint8_t *buf, uint32_t offset, uint32_t size) {
    if (offset + size > USB_DISK_SIZE) {
        size = USB_DISK_SIZE - offset;  //防止越界
    }
    memcpy(usb_disk_buffer + offset, buf, size);
    return size;
}

/************************************* USBManager方法实现 *************************************/

/*
 * 注册USB MSC组件
 * 
 * 设置三个回调函数给USBComposite库：
 * 1. get_capacity：报告磁盘容量
 * 2. read_callback：读取磁盘数据
 * 3. write_callback：写入磁盘数据
 * 
 * 首次注册时从Flash加载磁盘数据
 * 每个方法只能注册一次（通过usb_registered标志防止重复）
 */
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

/*
 * 启动USB连接
 * 
 * 调用USBComposite.begin()启动USB设备
 * 此时计算机会检测到新的U盘设备
 * 
 * 注意：必须先调用registerComponent()注册组件
 */
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

/*
 * 断开USB连接并保存数据
 * 
 * 步骤：
 * 1. 将RAM缓冲区数据刷入Flash持久化
 * 2. 调用USBComposite.end()断开USB连接
 * 3. 重置状态标志
 * 
 * 在进入睡眠模式前必须调用此方法
 */
void USBManager::end() {
    if (!usb_active) {
        return;
    }

    flushToFlash();
    USBComposite.end();
    usb_active = false;
    usb_registered = false;
}

/*
 * 查询USB是否已连接
 */
bool USBManager::isEnabled() {
    return usb_active;
}

/*
 * 将RAM缓冲区数据写入Flash
 * 
 * 执行步骤：
 * 1. 擦除整个目标扇区
 * 2. 逐字（32位）将RAM缓冲区写入Flash
 * 3. 共写入USB_DISK_SIZE/4=16384个字
 * 
 * 注意：Flash写入前必须确保目标地址已被擦除
 * 因此每次都先擦除整个扇区再写入全部数据
 */
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

/*
 * 从Flash加载数据到RAM缓冲区
 * 
 * 先检查目标Flash区域是否为空：
 * - 如果前两个字都是0xFFFFFFFF，说明扇区从未被写入
 * - 此时初始化RAM缓冲区为全0（空磁盘格式）
 * - 否则将Flash数据逐字复制到RAM缓冲区
 * 
 * 检查前两个字即可判断是否为空
 * 因为擦除后的Flash全为0xFF，写入后至少会有部分位为0
 */
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
