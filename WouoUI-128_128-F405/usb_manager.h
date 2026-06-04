#ifndef USB_MANAGER_H
#define USB_MANAGER_H

#include "config.h"

/*
 * USB磁盘大小配置（W25Q512JVEIQ 512Mbit = 64MB）
 *
 * USB_DISK_SIZE = 64MB：W25Q512全容量
 * USB_DISK_BLOCK_SIZE = 512：标准U盘扇区
 * USB_DISK_BLOCK_COUNT = 131072：64MB / 512B
 */
#define USB_DISK_SIZE           (64U * 1024U * 1024U)
#define USB_DISK_BLOCK_SIZE     512U
#define USB_DISK_BLOCK_COUNT    (USB_DISK_SIZE / USB_DISK_BLOCK_SIZE)

#if USB_MSC_ENABLE

/*
 * USB管理器
 *
 * 基于USB_OTG_HS + USB3300 ULPI PHY
 * 支持三种模式：
 *   - MSC：W25Q512 SPI Flash 作为存储后端（64MB U盘）
 *   - HID：Consumer Control（多媒体）+ Keyboard（键盘）
 *   - Composite：MSC + HID 复合设备
 */
class USBManager {
public:
    /*
     * 注册W25Q512 Flash存储组件
     */
    static void registerComponent();

    /*
     * 启动USB设备（自动根据开关选择模式）
     * 立即返回，状态通过 poll() 或 isEnabled() 查询
     */
    static void begin();

    /*
     * 轮询 USB 枚举状态
     * 返回 true 表示枚举已完成（成功或超时）
     */
    static bool poll();

    /*
     * 断开USB连接并释放硬件
     */
    static void end();

    /*
     * USB已枚举且配置完成
     */
    static bool isEnabled();

    /*
     * 查询是否有未完成的写入操作（仅MSC模式有效）
     */
    static bool isDirty();
};

#else

class USBManager {
public:
    static void registerComponent() {}
    static void begin() {}
    static bool poll() { return false; }
    static void end() {}
    static bool isEnabled() { return false; }
    static bool isDirty() { return false; }
};

#endif

#endif
