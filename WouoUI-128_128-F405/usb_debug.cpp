/*
 * USB 初始化屏幕调试实现 — 寄存器直显版
 * 使用 U8g2 在 Sharp LS013B7DH03 128x128 上绘制 USB 初始化进度
 * 超时时直接显示关键寄存器值，方便拍照诊断。
 *
 * 布局（128x128）：
 *  正常阶段：标题 + 步骤列表（8步）
 *  失败/超时：标题 + 10 行寄存器信息
 */

#include "usb_debug.h"
#include "config.h"
#include "display.h"  /* for extern U8G2 */
#include <Arduino.h>
#include <string.h>
#include <stdio.h>

/* ==================== USB 寄存器基址 ==================== */
#define USB_HS_BASE  0x40040000UL
#define REG(off)     (*(volatile const uint32_t *)(USB_HS_BASE + (off)))

/* ==================== 步骤名称 ==================== */
static const char *step_names[USB_STEP_TOTAL] = {
    "1. CLK+RST",
    "2. ULPI GPIO",
    "3. PHY POWER",
    "4. DCD INIT",
    "5. SPEED+FIFO",
    "6. VBUS CFG",
    "7. CONNECT",
    "8. DONE"
};

/* ==================== 状态缓存 ==================== */
static int step_status[USB_STEP_TOTAL];
static char status_msg[32];
static char debug_title[32] = "USB MSC Init";
static bool final_shown = false;

void usb_debug_reset(void)
{
    for (int i = 0; i < USB_STEP_TOTAL; i++) {
        step_status[i] = USB_STATUS_WAITING;
    }
    status_msg[0] = '\0';
    final_shown = false;
}

void usb_debug_mark_done(void)
{
    final_shown = true;
}

void usb_debug_set_title(const char *title)
{
    strncpy(debug_title, title, sizeof(debug_title) - 1);
    debug_title[sizeof(debug_title) - 1] = '\0';
}

void usb_debug_set_step(int step, int status)
{
    if (step >= 0 && step < USB_STEP_TOTAL) {
        step_status[step] = status;
    }
}

void usb_debug_set_msg(const char *msg)
{
    strncpy(status_msg, msg, sizeof(status_msg) - 1);
    status_msg[sizeof(status_msg) - 1] = '\0';
}

/* ==================== 寄存器读取 ==================== */

void usb_debug_read_regs(usb_regs_t *regs)
{
    regs->gotgctl  = REG(0x000);
    regs->gahbcfg  = REG(0x008);
    regs->gusbcfg  = REG(0x00C);
    regs->gintsts  = REG(0x014);
    regs->gintmsk  = REG(0x018);
    regs->grxfsiz  = REG(0x024);
    regs->dieptxf0 = REG(0x028);
    regs->gccfg    = REG(0x038);
    regs->dcfg     = REG(0x800);
    regs->dctl     = REG(0x804);
    regs->dsts     = REG(0x808);
    regs->doepmsk  = REG(0x80C);
    regs->diepmsk  = REG(0x810);
    regs->daint    = REG(0x814);
    regs->daintmsk = REG(0x818);
}

/* ==================== 居中绘制辅助 ==================== */

static void draw_centered_str(int y, const char *s)
{
    extern U8G2_LS013B7DH03_128X128_F_4W_SW_SPI u8g2;
    int16_t w = u8g2.getStrWidth(s);
    int16_t x = (128 - w) / 2;
    if (x < 0) x = 0;
    u8g2.drawStr(x, y, s);
}

/* ==================== 绘制一行步骤：状态标记 + 步骤名（左对齐） ==================== */

static void draw_step_row(int i, int y)
{
    extern U8G2_LS013B7DH03_128X128_F_4W_SW_SPI u8g2;
    const char *tag;
    switch (step_status[i]) {
        case USB_STATUS_WAITING: tag = "[  ]"; break;
        case USB_STATUS_BUSY:    tag = "[**]"; break;
        case USB_STATUS_OK:      tag = "[OK]"; break;
        case USB_STATUS_FAIL:    tag = "[XX]"; break;
        default:                 tag = "[??]"; break;
    }
    u8g2.drawStr(2, y, tag);
    u8g2.drawStr(30, y, step_names[i]);
}

/* ==================== 初始化步骤页 ==================== */

void usb_debug_refresh(void)
{
    if (final_shown) return;

    extern U8G2_LS013B7DH03_128X128_F_4W_SW_SPI u8g2;

    u8g2.firstPage();
    do {
        /* 标题居中 */
        u8g2.setFont(u8g2_font_helvB08_tr);
        draw_centered_str(12, debug_title);

        /* 步骤列表左对齐 */
        u8g2.setFont(u8g2_font_helvR08_tr);
        for (int i = 0; i < USB_STEP_TOTAL; i++) {
            draw_step_row(i, 26 + i * 13);
        }

        /* 底部状态消息左对齐 */
        if (status_msg[0] != '\0') {
            u8g2.setFont(u8g2_font_helvR08_tr);
            u8g2.drawStr(2, 124, status_msg);
        }
    } while (u8g2.nextPage());
}

/* ==================== 最终页绘制（共享：final + animate 共用） ==================== */

static void draw_final_page(bool success, const char *msg,
                            const usb_regs_t *regs, bool show_title)
{
    extern U8G2_LS013B7DH03_128X128_F_4W_SW_SPI u8g2;

    if (success) {
        /* ====== 成功页 ====== */
        if (show_title) {
            u8g2.setFont(u8g2_font_helvB10_tr);
            draw_centered_str(14, "USB OK!");
        }

        u8g2.setFont(u8g2_font_helvR08_tr);
        for (int i = 0; i < USB_STEP_TOTAL; i++) {
            draw_step_row(i, 26 + i * 13);   /* 行距与初始化页一致 */
        }
        if (msg) {
            u8g2.drawStr(2, 124, msg);
        }
    } else {
        /* ====== 失败页：寄存器直显 ====== */
        if (show_title) {
            u8g2.setFont(u8g2_font_helvB10_tr);
            draw_centered_str(14, "USB FAIL");
        }

        if (msg) {
            u8g2.setFont(u8g2_font_helvR08_tr);
            u8g2.drawStr(2, 24, msg);
        }

        if (regs) {
            u8g2.setFont(u8g2_font_helvR08_tr);
            char buf[24];
            int y = 36;

            snprintf(buf, sizeof(buf), "AHB:%08lX D%lu H%lu G%lu",
                (unsigned long)regs->gahbcfg,
                (unsigned long)((regs->gahbcfg >> 5) & 1),
                (unsigned long)((regs->gahbcfg >> 1) & 7),
                (unsigned long)(regs->gahbcfg & 1));
            u8g2.drawStr(2, y, buf);

            y += 11;
            snprintf(buf, sizeof(buf), "USB:%08lX MD=%lu ULPI=%lu",
                (unsigned long)regs->gusbcfg,
                (unsigned long)((regs->gusbcfg >> 29) & 1),
                (unsigned long)((regs->gusbcfg >> 6) & 1) ^ 1);
            u8g2.drawStr(2, y, buf);

            y += 11;
            snprintf(buf, sizeof(buf), "CFG:%08lX P%lu N%lu V%lu",
                (unsigned long)regs->gccfg,
                (unsigned long)((regs->gccfg >> 16) & 1),
                (unsigned long)((regs->gccfg >> 21) & 1),
                (unsigned long)((regs->gccfg >> 19) & 1));
            u8g2.drawStr(2, y, buf);

            y += 11;
            snprintf(buf, sizeof(buf), "GIN:%08lX E%lu R%lu O%lu I%lu",
                (unsigned long)regs->gintsts,
                (unsigned long)((regs->gintsts >> 6) & 1),
                (unsigned long)((regs->gintsts >> 7) & 1),
                (unsigned long)((regs->gintsts >> 19) & 1),
                (unsigned long)((regs->gintsts >> 18) & 1));
            u8g2.drawStr(2, y, buf);

            y += 11;
            snprintf(buf, sizeof(buf), "MSK:%08lX R%lu O%lu I%lu",
                (unsigned long)regs->gintmsk,
                (unsigned long)((regs->gintmsk >> 4) & 1),
                (unsigned long)((regs->gintmsk >> 19) & 1),
                (unsigned long)((regs->gintmsk >> 18) & 1));
            u8g2.drawStr(2, y, buf);

            y += 11;
            snprintf(buf, sizeof(buf), "DCT:%08lX SD=%lu",
                (unsigned long)regs->dctl,
                (unsigned long)((regs->dctl >> 1) & 1));
            u8g2.drawStr(2, y, buf);

            y += 11;
            {
                uint32_t enumspd = regs->dsts & 3;
                const char *spd;
                switch (enumspd) {
                    case 0: spd = "LS"; break;
                    case 1: spd = "FS"; break;
                    case 2: spd = "HS"; break;
                    default: spd = "?"; break;
                }
                snprintf(buf, sizeof(buf), "DST:%08lX %s SLP=%lu",
                    (unsigned long)regs->dsts, spd,
                    (unsigned long)((regs->dsts >> 2) & 1));
            }
            u8g2.drawStr(2, y, buf);

            y += 11;
            snprintf(buf, sizeof(buf), "OTG:%08lX BV%lu AV%lu D%lu",
                (unsigned long)regs->gotgctl,
                (unsigned long)((regs->gotgctl >> 19) & 1),
                (unsigned long)((regs->gotgctl >> 18) & 1),
                (unsigned long)((regs->gotgctl >> 16) & 1));
            u8g2.drawStr(2, y, buf);

            y += 11;
            snprintf(buf, sizeof(buf), "DOEP:%04lX DAIM:%04lX",
                (unsigned long)(regs->doepmsk & 0xFFFF),
                (unsigned long)(regs->daintmsk & 0xFFFF));
            u8g2.drawStr(2, y, buf);

            y += 11;
            snprintf(buf, sizeof(buf), "RXF:%lu TX0:%lu",
                (unsigned long)regs->grxfsiz,
                (unsigned long)((regs->dieptxf0 >> 16) & 0xFFFF));
            u8g2.drawStr(2, y, buf);
        }
    }
}

/* ==================== 寄存器诊断页（失败/超时时显示） ==================== */

void usb_debug_final(bool success, const char *msg,
                     const usb_regs_t *regs)
{
    if (final_shown) return;

    extern U8G2_LS013B7DH03_128X128_F_4W_SW_SPI u8g2;

    u8g2.firstPage();
    do {
        draw_final_page(success, msg, regs, true);
    } while (u8g2.nextPage());

    final_shown = true;
}


