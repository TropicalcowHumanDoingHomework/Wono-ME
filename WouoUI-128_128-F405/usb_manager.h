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
 * USB大容量存储管理器
 *
 * 基于USB_OTG_HS + USB3300 ULPI PHY + W25Q512 SPI Flash
 * - W25Q512作为存储后端，4KB扇区缓存写入
 * - 自包含USBD MSC类驱动处理BOT/SCSI协议
 */
class USBManager {
public:
    /*
     * 注册USB MSC组件
     * 初始化W25Q512 SPI Flash并验证器件ID
     */
    static void registerComponent();

    /*
     * 启动USB MSC设备
     * 通过usbd_msc_reinit()初始化为MSC模式
     * 立即返回，status 通过 poll() 或 isEnabled() 查询
     */
    static void begin();

    /*
     * 轮询 USB 枚举状态（在 loop() 中调用）
     * 在 begin() 之后定期调用，不再阻塞
     * 返回 true 表示已枚举完成
     */
    static bool poll();

    /*
     * 断开USB连接
     */
    static void end();

    /*
     * 查询USB连接状态
     */
    static bool isEnabled();

    /*
     * 查询是否有未完成的写入操作
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
