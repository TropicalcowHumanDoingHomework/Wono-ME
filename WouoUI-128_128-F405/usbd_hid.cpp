#include "usbd_hid.h"
#include <string.h>

#if HID_ENABLE

extern "C" {
#include <STM32_USB_Device_Library/Core/inc/usbd_core.h>
#include <STM32_USB_Device_Library/Core/inc/usbd_ioreq.h>
#include <STM32_USB_Device_Library/Core/inc/usbd_usr.h>
#include <STM32_USB_OTG_Driver/inc/usb_dcd.h>
}

extern USB_OTG_CORE_HANDLE USB_OTG_dev;

/* ==================== HID 全局状态 ==================== */

static bool hid_ready = false;
static uint8_t hid_idle_rate = 0;
static uint8_t hid_protocol = 1;

/* ==================== HID 配置描述符 ==================== */

static uint8_t hid_cfg_desc[USB_HID_CONFIG_DESC_SIZ] = {
    /* 配置描述符 */
    0x09, 0x02,
    (uint8_t)(USB_HID_CONFIG_DESC_SIZ & 0xFF),
    (uint8_t)((USB_HID_CONFIG_DESC_SIZ >> 8) & 0xFF),
    0x01, 0x01, 0x00, 0xC0, 0x32,

    /* 接口描述符 */
    0x09, 0x04, 0x01, 0x00, 0x01,
    0x03,        /* bInterfaceClass = HID */
    0x00,        /* bInterfaceSubClass = 0 (无子类) */
    0x00,        /* bInterfaceProtocol = 0 (无协议) */
    0x00,

    /* HID 描述符 */
    0x09,        /* bLength */
    0x21,        /* bDescriptorType = HID */
    0x10, 0x01,  /* bcdHID = 1.10 */
    0x00,        /* bCountryCode */
    0x01,        /* bNumDescriptors */
    0x22,        /* bDescriptorType = HID_REPORT_DESC */
    HID_REPORT_DESC_LEN & 0xFF, (HID_REPORT_DESC_LEN >> 8) & 0xFF,

    /* Endpoint IN (中断) */
    0x07, 0x05, HID_EP_IN, 0x03,
    (uint8_t)(HID_EP_SIZE & 0xFF), (uint8_t)((HID_EP_SIZE >> 8) & 0xFF),
    HID_HS_BINTERVAL,
};

/* ==================== HID 类回调 ==================== */

static uint8_t hid_init(void *pdev, uint8_t cfgidx)
{
    (void)cfgidx;
    USB_OTG_CORE_HANDLE *dev = (USB_OTG_CORE_HANDLE *)pdev;

    DCD_EP_Open(dev, HID_EP_IN, HID_EP_SIZE, USB_OTG_EP_INT);
    hid_ready = true;

    return 0;
}

static uint8_t hid_deinit(void *pdev, uint8_t cfgidx)
{
    (void)cfgidx;
    USB_OTG_CORE_HANDLE *dev = (USB_OTG_CORE_HANDLE *)pdev;

    DCD_EP_Close(dev, HID_EP_IN);
    hid_ready = false;

    return 0;
}

static uint8_t hid_setup(void *pdev, USB_SETUP_REQ *req)
{
    USB_OTG_CORE_HANDLE *dev = (USB_OTG_CORE_HANDLE *)pdev;

    switch (req->bRequest) {

    case HID_REQ_GET_REPORT:
        break;

    case HID_REQ_GET_IDLE:
        USBD_CtlSendData(dev, &hid_idle_rate, 1);
        return USBD_OK;

    case HID_REQ_SET_IDLE:
        hid_idle_rate = (uint8_t)(req->wValue >> 8);
        return USBD_OK;

    case HID_REQ_GET_PROTOCOL:
        USBD_CtlSendData(dev, &hid_protocol, 1);
        return USBD_OK;

    case HID_REQ_SET_PROTOCOL:
        hid_protocol = (uint8_t)(req->wValue & 0xFF);
        return USBD_OK;

    case HID_REQ_SET_REPORT:
        DCD_EP_PrepareRx(dev, 0x00, dev->dev.setup_packet, req->wLength);
        return USBD_OK;
    }

    if ((req->bmRequest & USB_REQ_TYPE_MASK) == USB_REQ_TYPE_STANDARD) {
        if (req->bRequest == 0x06) {
            uint16_t desc_type = (req->wValue >> 8) & 0xFF;
            uint16_t desc_index = req->wValue & 0xFF;
            (void)desc_index;

            if (desc_type == HID_REPORT_DESC) {
                USBD_CtlSendData(dev,
                    (uint8_t *)hid_report_desc,
                    HID_REPORT_DESC_LEN > req->wLength ?
                        req->wLength : HID_REPORT_DESC_LEN);
                return USBD_OK;
            }

            if (desc_type == HID_DESCRIPTOR_TYPE) {
                USBD_CtlSendData(dev, hid_cfg_desc + 18, USB_HID_DESC_SIZ);
                return USBD_OK;
            }
        }
    }

    return USBD_FAIL;
}

static uint8_t hid_data_in(void *pdev, uint8_t epnum)
{
    (void)pdev;
    (void)epnum;
    return 0;
}

static uint8_t hid_data_out(void *pdev, uint8_t epnum)
{
    (void)pdev;
    (void)epnum;
    return 0;
}

static uint8_t *hid_get_cfg_desc(uint8_t speed, uint16_t *length)
{
    (void)speed;
    *length = sizeof(hid_cfg_desc);
    return hid_cfg_desc;
}

static uint8_t *hid_get_other_cfg_desc(uint8_t speed, uint16_t *length)
{
    (void)speed;
    *length = sizeof(hid_cfg_desc);
    return hid_cfg_desc;
}

/* ==================== HID 类回调结构体 ==================== */

USBD_Class_cb_TypeDef USBD_HID_cb = {
    .Init               = hid_init,
    .DeInit             = hid_deinit,
    .Setup              = hid_setup,
    .EP0_TxSent         = NULL,
    .EP0_RxReady        = NULL,
    .DataIn             = hid_data_in,
    .DataOut            = hid_data_out,
    .SOF                = NULL,
    .IsoINIncomplete    = NULL,
    .IsoOUTIncomplete   = NULL,
    .GetConfigDescriptor = hid_get_cfg_desc,
    .GetOtherConfigDescriptor = hid_get_other_cfg_desc,
};

/* ==================== 公共接口 ==================== */

uint8_t usbd_hid_send_report(uint8_t *report, uint16_t len)
{
    if (!hid_ready || !report || len == 0) {
        return USBD_FAIL;
    }
    DCD_EP_Tx(&USB_OTG_dev, HID_EP_IN, report, len);
    return USBD_OK;
}

bool usbd_hid_is_ready(void)
{
    return hid_ready && (USB_OTG_dev.dev.device_status == USB_OTG_CONFIGURED);
}

#endif /* HID_ENABLE */
