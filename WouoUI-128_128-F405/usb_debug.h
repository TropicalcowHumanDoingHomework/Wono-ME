#ifndef USB_DEBUG_H
#define USB_DEBUG_H

#include <stdint.h>
#include <stdbool.h>

/*
 * USB 初始化屏幕调试系统 — 寄存器直显版
 * 在 Sharp LS013B7DH03 128x128 Memory LCD 上显示 USB MSC 初始化进度
 * 和关键寄存器值，方便拍照诊断枚举问题。
 *
 * 布局分区：
 *   正常初始化：显示步骤进度（逐步更新）
 *   失败/超时：  显示标题 + 10行寄存器数据
 */

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== 步骤定义（初始化阶段使用） ==================== */

typedef enum {
    USB_STEP_CLOCK   = 0,   /* 时钟使能 + 复位释放 */
    USB_STEP_GPIO    = 1,   /* ULPI GPIO 初始化 */
    USB_STEP_PHY     = 2,   /* USB3300 PHY 上电 */
    USB_STEP_DCD     = 3,   /* DCD_Init 核心初始化 */
    USB_STEP_SPEED   = 4,   /* 速度 + FIFO 配置 */
    USB_STEP_VBUS    = 5,   /* VBUS/NOVBUSSENS 配置 */
    USB_STEP_CONNECT = 6,   /* DP 上拉连接 */
    USB_STEP_DONE    = 7,   /* 枚举等待 / 完成 */
    USB_STEP_TOTAL   = 8
} UsbDebugStep;

#define USB_STATUS_WAITING  0
#define USB_STATUS_BUSY     1
#define USB_STATUS_OK       2
#define USB_STATUS_FAIL     3

/* ==================== USB 寄存器快照结构体 ==================== */

typedef struct {
    uint32_t gotgctl;    /* 0x000 — OTG Control & Status */
    uint32_t gahbcfg;    /* 0x008 — AHB Config (DMAEN, HBSTLEN, GINT) */
    uint32_t gusbcfg;    /* 0x00C — USB Config (ULPI/device mode) */
    uint32_t gintsts;    /* 0x014 — Global Interrupt Status */
    uint32_t gintmsk;    /* 0x018 — Global Interrupt Mask */
    uint32_t grxfsiz;    /* 0x024 — Rx FIFO Size */
    uint32_t dieptxf0;   /* 0x028 — EP0 Tx FIFO (DIEPTXF0/HNPTXFSIZ) */
    uint32_t gccfg;      /* 0x038 — Global Config (PWRDWN, NOVBUSSENS) */
    uint32_t dcfg;       /* 0x800 — Device Config (speed) */
    uint32_t dctl;       /* 0x804 — Device Control (SDIS) */
    uint32_t dsts;       /* 0x808 — Device Status (ENUMSPD) */
    uint32_t doepmsk;    /* 0x80C — Device OUT EP Mask (STUP) */
    uint32_t diepmsk;    /* 0x810 — Device IN EP Mask */
    uint32_t daint;      /* 0x814 — Device All Interrupts */
    uint32_t daintmsk;   /* 0x818 — Device All Interrupts Mask */
} usb_regs_t;

/* ==================== 函数接口 ==================== */

/* 一次性读取所有 USB 关键寄存器 */
void usb_debug_read_regs(usb_regs_t *regs);

/* 初始化阶段步骤控制 */
void usb_debug_set_step(int step, int status);
void usb_debug_set_msg(const char *msg);
void usb_debug_refresh(void);
void usb_debug_reset(void);

/*
 * 最终显示（成功或失败）
 * 失败时 regs != NULL 显示寄存器诊断页
 */
void usb_debug_final(bool success, const char *msg,
                     const usb_regs_t *regs);

/* 标记 debug 为已完成状态，抑制后续 usb_debug_refresh() 绘制屏幕 */
void usb_debug_mark_done(void);

#ifdef __cplusplus
}
#endif

#endif
