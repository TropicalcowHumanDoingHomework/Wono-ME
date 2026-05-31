#ifndef USBD_HID_H
#define USBD_HID_H

#include "config.h"
#include <Arduino.h>

#if HID_ENABLE

extern "C" {
#include <STM32_USB_Device_Library/Core/inc/usbd_core.h>
}

#include "hid_manager.h"

#define HID_EP_IN      0x82   /* EP2 IN — 中断传输 */
#define HID_EP_SIZE    16     /* HS中断包最大大小 */

#define USB_HID_CONFIG_DESC_SIZ  34
#define USB_HID_DESC_SIZ         9

#define HID_DESCRIPTOR_TYPE      0x21
#define HID_REPORT_DESC          0x22

#define HID_REQ_SET_PROTOCOL     0x0B
#define HID_REQ_GET_PROTOCOL     0x03
#define HID_REQ_SET_IDLE         0x0A
#define HID_REQ_GET_IDLE         0x02
#define HID_REQ_SET_REPORT       0x09
#define HID_REQ_GET_REPORT       0x01

#define HID_HS_BINTERVAL         0x07
#define HID_FS_BINTERVAL         0x0A

extern USBD_Class_cb_TypeDef USBD_HID_cb;

uint8_t usbd_hid_send_report(uint8_t *report, uint16_t len);
bool    usbd_hid_is_ready(void);
void    usbd_hid_reinit(void);

extern const uint8_t hid_report_desc[HID_REPORT_DESC_LEN];
extern const uint16_t hid_report_desc_len;

#endif /* HID_ENABLE */
#endif /* USBD_HID_H */
