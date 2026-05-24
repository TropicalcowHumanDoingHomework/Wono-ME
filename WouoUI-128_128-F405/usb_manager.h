#ifndef USB_MANAGER_H
#define USB_MANAGER_H

#include "config.h"

#if USB_MSC_ENABLE

/*
 * USB磁盘大小配置
 * 
 * USB_DISK_SIZE = 64KB：虚拟U盘总容量
 * USB_DISK_BLOCK_SIZE = 512：每块大小（标准U盘扇区大小）
 * USB_DISK_BLOCK_COUNT = 128：总块数（64KB / 512B）
 * USB_DISK_FLASH_ADDR = 0x080C0000：Flash中存储磁盘数据的起始地址
 *   - 对应STM32F405的8号扇区（128KB大小）
 *   - 64KB磁盘数据存储在扇区前半部分
 */
#define USB_DISK_SIZE           (64U * 1024U)
#define USB_DISK_BLOCK_SIZE     512U
#define USB_DISK_BLOCK_COUNT    (USB_DISK_SIZE / USB_DISK_BLOCK_SIZE)
#define USB_DISK_FLASH_ADDR     0x080C0000U

/*
 * USB大容量存储管理器
 * 
 * 实现基于Flash模拟的U盘功能：
 * - 在RAM中维护64KB磁盘缓冲区以提供快速读写
 * - 通过Flash扇区擦写实现数据持久化
 * - 支持USB连接/断开时的数据同步
 * 
 * 使用场景：
 * - 用户通过USB连接电脑时，虚拟U盘可在文件管理器中访问
 * - 断开USB或进入睡眠时，RAM数据自动写入Flash
 */
class USBManager {
public:
    /*
     * 注册USB MSC组件到USBComposite框架
     * 设置容量回调、读写回调
     * 首次注册时从Flash加载磁盘数据
     */
    static void registerComponent();
    
    /*
     * 启动USB连接
     * 调用USBComposite.begin()使USB设备上线
     * 在此之前必须先调用registerComponent()
     */
    static void begin();
    
    /*
     * 断开USB连接
     * 先将RAM缓冲区数据刷入Flash，再断开USB
     * 确保数据完整性
     */
    static void end();
    
    /*
     * 查询USB连接状态
     * 返回true表示USB设备已连接并处于活动状态
     */
    static bool isEnabled();
    
    /*
     * 将RAM缓冲区数据写入Flash
     * 执行扇区擦除和逐字编程
     * 在USB断开或进入睡眠前调用
     */
    static void flushToFlash();
    
    /*
     * 从Flash加载数据到RAM缓冲区
     * 检查Flash区域是否为空（未擦写过），是则初始化空磁盘
     * 否则逐字复制到RAM缓冲区
     */
    static void loadFromFlash();
};

#else

/*
 * USB禁用时的空实现
 * 
 * 当USB_MSC_ENABLE=0时，所有方法均为空操作
 * 确保调用代码无需条件编译
 */
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
