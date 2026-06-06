#ifndef USBD_CDC_H
#define USBD_CDC_H

#include "config.h"
#include <Arduino.h>

#if USB_MSC_ENABLE

extern "C" {
#include <STM32_USB_Device_Library/Core/inc/usbd_core.h>
}

/* ==================== CDC 端点定义 ==================== */
#define CDC_NOTIF_EP            0x83   /* EP3 IN — 中断通知 */
#define CDC_DATA_OUT_EP         0x04   /* EP4 OUT — 数据 Bulk OUT */
#define CDC_DATA_IN_EP          0x84   /* EP4 IN — 数据 Bulk IN */

#define CDC_NOTIF_PACKET_SIZE   8
#define CDC_DATA_PACKET_SIZE    512    /* HS 下 Bulk 包大小 */
#define CDC_DATA_PACKET_SIZE_FS 64     /* FS 下 Bulk 包大小 */

#define USB_CDC_CONFIG_DESC_SIZ 67

/* CDC 类请求 */
#define CDC_SEND_ENCAPSULATED_COMMAND   0x00
#define CDC_GET_ENCAPSULATED_RESPONSE   0x01
#define CDC_SET_LINE_CODING             0x20
#define CDC_GET_LINE_CODING             0x21
#define CDC_SET_CONTROL_LINE_STATE      0x22
#define CDC_SEND_BREAK                  0x23

/* LINE_CODING 结构（与 USB CDC 标准定义一致） */
typedef struct __attribute__((packed)) {
    uint32_t bitrate;
    uint8_t  format;      /* 0=1 stop bit, 1=1.5, 2=2 */
    uint8_t  paritytype;  /* 0=None, 1=Odd, 2=Even, 3=Mark, 4=Space */
    uint8_t  datatype;    /* 5,6,7,8 or 16 */
} LINE_CODING_TypeDef;

/* ==================== CDC 接收缓冲区大小 ==================== */
#define CDC_RX_BUF_SIZE  2048
#define CDC_TX_BUF_SIZE  2048

/* ==================== CDC 公共 API ==================== */

/*
 * 初始化 CDC-only 模式（独立 CDC 虚拟串口）
 */
void usbd_cdc_reinit(void);

/*
 * CDC 设备是否已枚举完成
 */
bool usbd_cdc_is_ready(void);

/*
 * 发送数据到 CDC 虚拟串口 TX 缓冲区
 * 返回实际写入的字节数（非阻塞）
 */
uint32_t cdc_write(const uint8_t *buf, uint32_t len);

/*
 * 发送单个字节
 */
uint32_t cdc_write_byte(uint8_t c);

/*
 * 从 CDC 虚拟串口 RX 缓冲区读取数据
 * 返回实际读取的字节数（非阻塞）
 */
uint32_t cdc_read(uint8_t *buf, uint32_t len);

/*
 * 查询 RX 缓冲区中可读取的字节数
 */
uint32_t cdc_available(void);

/*
 * 查询 CDC 端口是否已打开（主机 DTR 信号状态）
 */
bool cdc_is_connected(void);

/*
 * CDC printf 辅助函数
 */
void cdc_printf(const char *fmt, ...);

/*
 * CDC 全局状态（供复合设备回调使用）
 */
extern bool            g_cdc_ready;
extern LINE_CODING_TypeDef g_line_coding;
extern uint8_t         g_cdc_rx_buf[CDC_RX_BUF_SIZE];
extern volatile uint32_t g_cdc_rx_head;
extern volatile uint32_t g_cdc_rx_tail;
extern uint8_t         g_cdc_tx_buf[CDC_TX_BUF_SIZE];
extern volatile uint32_t g_cdc_tx_head;
extern volatile uint32_t g_cdc_tx_tail;
extern volatile bool   g_cdc_dtr;
extern USB_OTG_CORE_HANDLE *g_cdc_pdev;

/* 供复合设备调用的 CDC 类回调 */
uint8_t cdc_class_init(void *pdev, uint8_t cfgidx);
uint8_t cdc_class_deinit(void *pdev, uint8_t cfgidx);
uint8_t cdc_class_setup(void *pdev, USB_SETUP_REQ *req);
uint8_t cdc_class_data_in(void *pdev, uint8_t epnum);
uint8_t cdc_class_data_out(void *pdev, uint8_t epnum);
uint8_t cdc_class_sof(void *pdev);
void    cdc_class_prepare_rx(void *pdev);

#endif /* USB_MSC_ENABLE */
#endif /* USBD_CDC_H */
