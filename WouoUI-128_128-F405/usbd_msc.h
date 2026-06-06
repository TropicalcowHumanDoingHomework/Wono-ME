#ifndef USBD_MSC_H
#define USBD_MSC_H

/*
 * USB MSC (Mass Storage Class) 设备实现
 *
 * 硬件：STM32F405RGT6 + USB3300-EZK-TR (ULPI HS PHY)
 * 使用 USB_OTG_HS 内核 + ULPI 8-bit 接口
 * BOT协议 + SCSI命令集，单LUN
 *
 * 存储后端：W25Q512JVEIQ SPI NOR Flash (64MB)
 */

#include <Arduino.h>
#include "config.h"

/* ==================== 存储回调结构体 ==================== */

typedef struct {
    int8_t  (*Init)            (uint8_t lun);
    int8_t  (*GetCapacity)     (uint8_t lun, uint32_t *block_num, uint32_t *block_size);
    int8_t  (*IsReady)         (uint8_t lun);
    int8_t  (*IsWriteProtected)(uint8_t lun);
    int8_t  (*Read)            (uint8_t lun, uint8_t *buf, uint32_t blk_addr, uint16_t blk_len);
    int8_t  (*Write)           (uint8_t lun, uint8_t *buf, uint32_t blk_addr, uint16_t blk_len);
    int8_t  (*GetMaxLun)       (void);
    int8_t  *pInquiry;
} USBD_STORAGE_cb_TypeDef;

extern USBD_STORAGE_cb_TypeDef *USBD_STORAGE_fops;

/* ==================== SCSI命令码 ==================== */
#define SCSI_TEST_UNIT_READY    0x00
#define SCSI_REQUEST_SENSE      0x03
#define SCSI_INQUIRY            0x12
#define SCSI_MODE_SENSE6        0x1A
#define SCSI_START_STOP_UNIT    0x1B
#define SCSI_PREVENT_ALLOW      0x1E
#define SCSI_READ_FORMAT_CAP    0x23
#define SCSI_READ_CAPACITY10    0x25
#define SCSI_READ10             0x28
#define SCSI_WRITE10            0x2A
#define SCSI_VERIFY10           0x2F
#define SCSI_MODE_SENSE10       0x5A

/* ==================== W25Q512 SPI Flash 驱动接口 ==================== */

/*
 * 初始化W25Q512（SPI引脚 + 器件验证）
 * 返回 true 表示检测到正确的JEDEC ID
 */
bool w25q_init(void);

/*
 * 获取上次失败的JEDEC ID字符串（如 "ID: 0xEF401A"）
 * 用于调试显示
 */
const char* w25q_get_last_id_str(void);

/*
 * 从W25Q512读取数据
 * addr: 24位字节地址（0 ~ 64MB-1）
 * buf:  输出缓冲区
 * len:  读取长度（无限制）
 */
void w25q_read(uint32_t addr, uint8_t *buf, uint32_t len);

/*
 * 向W25Q512写入一个512B扇区（处理擦除）
 *
 * W25Q512要求写入前目标区域必须已擦除（全0xFF），最小擦除单位4KB
 * 策略：缓存4KB扇区 → 修改512B → 擦除 → 重写4KB
 *
 * blk_addr: 512B逻辑块地址（0 ~ 131071）
 * buf:      512B数据
 * 返回 0=成功, -1=失败
 */
int8_t w25q_write_block(uint32_t blk_addr, const uint8_t *buf);

/*
 * 从W25Q512读取一个512B扇区
 * blk_addr: 512B逻辑块地址
 * buf:      输出缓冲区（512B）
 */
void w25q_read_block(uint32_t blk_addr, uint8_t *buf);

/* ==================== 公共接口 ==================== */

/*
 * 初始化USB HS ULPI BSP + 以MSC类回调启动USB设备
 * 调用前需确保USBD_STORAGE_fops已设置
 */
void usbd_msc_reinit();

/*
 * 查询MSC设备是否已配置（被主机枚举完成）
 */
bool usbd_msc_is_mounted();

/*
 * 断开MSC设备（完全关闭USB HS）
 */
void usbd_msc_disconnect();

/*
 * 复合设备重初始化（MSC + HID）
 * HID_ENABLE=0 时等同于 usbd_msc_reinit()
 */
void usbd_composite_reinit();

/*
 * CDC-only 模式重初始化
 */
void usbd_cdc_reinit();

/*
 * 复合设备 MSC + HID + CDC（三接口）
 */
void usbd_composite_cdc_reinit();

/*
 * 复合设备 HID + CDC（双接口）
 */
void usbd_hid_cdc_reinit();

/*
 * 复合设备 MSC + CDC（双接口）
 */
void usbd_msc_cdc_reinit();

#endif
