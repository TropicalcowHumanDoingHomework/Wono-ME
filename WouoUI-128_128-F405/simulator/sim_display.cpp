#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "sim_config.h"
#include <algorithm>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <cmath>

// ============================================================
// Font array definitions
// ============================================================
const uint8_t u8g2_font_helvB24_tr[] = { 1 };
const uint8_t u8g2_font_HelvetiPixel_tr[] = { 2 };

// helvB24 character widths (ASCII 32-126)
static const uint8_t width_helvB24[96] = {
     8,  7, 11, 21, 19, 30, 25,  8, 11, 11, 17, 20,  8, 12,  8, 18,
    19, 19, 19, 19, 19, 19, 19, 19, 19, 19,  8,  8, 20, 20, 20, 19,
    33, 25, 25, 25, 25, 23, 21, 28, 26, 10, 20, 24, 20, 30, 26, 28,
    24, 28, 24, 25, 23, 26, 24, 35, 24, 23, 21,  9, 18,  9, 19, 19,
    11, 20, 21, 18, 21, 19, 13, 21, 20,  8, 10, 19,  8, 30, 20, 21,
    21, 21, 13, 19, 12, 20, 18, 26, 18, 18, 16, 12,  8, 12, 22, 15
};

// IBM CGA 8x8 Font ROM (public domain, from IBM PC BIOS)
// Used in: Linux kernel, Adafruit GFX, countless embedded projects
static const uint8_t width_helvPix[95] = {
     3,  5,  7,  8,  7,  7,  8,  5,  6,  6,  8,  7,  5,  7,  5,  7,
     7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  5,  5,  7,  7,  7,  7,
     7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  8,  8,  7,  7,
     7,  7,  7,  7,  7,  7,  7,  8,  7,  7,  6,  7,  6,  7,  8,  5,
     7,  7,  7,  7,  7,  7,  7,  7,  7,  6,  7,  7,  8,  8,  7,  7,
     7,  7,  7,  7,  7,  7,  7,  8,  7,  7,  7,  7,  5,  7,  7
};

static const uint8_t glyph_helvPix[95][8] = {
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, // 32 ' '
    {0x18,0x18,0x18,0x18,0x18,0x00,0x18,0x00}, // 33 !
    {0x66,0x66,0x66,0x00,0x00,0x00,0x00,0x00}, // 34 "
    {0x66,0x66,0xFF,0x66,0xFF,0x66,0x66,0x00}, // 35 #
    {0x18,0x3E,0x60,0x3C,0x06,0x7C,0x18,0x00}, // 36 $
    {0x62,0x66,0x0C,0x18,0x30,0x66,0x46,0x00}, // 37 %
    {0x1C,0x36,0x1C,0x38,0x6F,0x66,0x3B,0x00}, // 38 &
    {0x18,0x18,0x18,0x00,0x00,0x00,0x00,0x00}, // 39 '
    {0x0C,0x18,0x30,0x30,0x30,0x18,0x0C,0x00}, // 40 (
    {0x30,0x18,0x0C,0x0C,0x0C,0x18,0x30,0x00}, // 41 )
    {0x00,0x66,0x3C,0xFF,0x3C,0x66,0x00,0x00}, // 42 *
    {0x00,0x18,0x18,0x7E,0x18,0x18,0x00,0x00}, // 43 +
    {0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x30}, // 44 ,
    {0x00,0x00,0x00,0x7E,0x00,0x00,0x00,0x00}, // 45 -
    {0x00,0x00,0x00,0x00,0x00,0x18,0x18,0x00}, // 46 .
    {0x06,0x06,0x0C,0x18,0x30,0x60,0x60,0x00}, // 47 /
    {0x3C,0x66,0x6E,0x76,0x66,0x66,0x3C,0x00}, // 48 0
    {0x18,0x38,0x18,0x18,0x18,0x18,0x7E,0x00}, // 49 1
    {0x3C,0x66,0x06,0x0C,0x18,0x30,0x7E,0x00}, // 50 2
    {0x3C,0x66,0x06,0x1C,0x06,0x66,0x3C,0x00}, // 51 3
    {0x0C,0x1C,0x2C,0x4C,0x7E,0x0C,0x0C,0x00}, // 52 4
    {0x7E,0x60,0x7C,0x06,0x06,0x66,0x3C,0x00}, // 53 5
    {0x1C,0x30,0x60,0x7C,0x66,0x66,0x3C,0x00}, // 54 6
    {0x7E,0x66,0x06,0x0C,0x18,0x18,0x18,0x00}, // 55 7
    {0x3C,0x66,0x66,0x3C,0x66,0x66,0x3C,0x00}, // 56 8
    {0x3C,0x66,0x66,0x3E,0x06,0x0C,0x38,0x00}, // 57 9
    {0x00,0x00,0x18,0x18,0x00,0x18,0x18,0x00}, // 58 :
    {0x00,0x00,0x18,0x18,0x00,0x18,0x18,0x30}, // 59 ;
    {0x0C,0x18,0x30,0x60,0x30,0x18,0x0C,0x00}, // 60 <
    {0x00,0x00,0x7E,0x00,0x00,0x7E,0x00,0x00}, // 61 =
    {0x30,0x18,0x0C,0x06,0x0C,0x18,0x30,0x00}, // 62 >
    {0x3C,0x66,0x06,0x0C,0x18,0x00,0x18,0x00}, // 63 ?
    {0x3C,0x66,0x6E,0x6A,0x6E,0x60,0x3C,0x00}, // 64 @
    {0x18,0x3C,0x66,0x66,0x7E,0x66,0x66,0x00}, // 65 A
    {0x7C,0x66,0x66,0x7C,0x66,0x66,0x7C,0x00}, // 66 B
    {0x3C,0x66,0x60,0x60,0x60,0x66,0x3C,0x00}, // 67 C
    {0x78,0x6C,0x66,0x66,0x66,0x6C,0x78,0x00}, // 68 D
    {0x7E,0x60,0x60,0x78,0x60,0x60,0x7E,0x00}, // 69 E
    {0x7E,0x60,0x60,0x78,0x60,0x60,0x60,0x00}, // 70 F
    {0x3C,0x66,0x60,0x6E,0x66,0x66,0x3E,0x00}, // 71 G
    {0x66,0x66,0x66,0x7E,0x66,0x66,0x66,0x00}, // 72 H
    {0x7E,0x18,0x18,0x18,0x18,0x18,0x7E,0x00}, // 73 I
    {0x1E,0x0C,0x0C,0x0C,0x0C,0x6C,0x38,0x00}, // 74 J
    {0x66,0x6C,0x78,0x70,0x78,0x6C,0x66,0x00}, // 75 K
    {0x60,0x60,0x60,0x60,0x60,0x60,0x7E,0x00}, // 76 L
    {0x63,0x77,0x7F,0x6B,0x63,0x63,0x63,0x00}, // 77 M
    {0x66,0x76,0x7E,0x7E,0x6E,0x66,0x66,0x00}, // 78 N
    {0x3C,0x66,0x66,0x66,0x66,0x66,0x3C,0x00}, // 79 O
    {0x7C,0x66,0x66,0x7C,0x60,0x60,0x60,0x00}, // 80 P
    {0x3C,0x66,0x66,0x66,0x6A,0x6C,0x36,0x00}, // 81 Q
    {0x7C,0x66,0x66,0x7C,0x6C,0x66,0x66,0x00}, // 82 R
    {0x3C,0x66,0x60,0x3C,0x06,0x66,0x3C,0x00}, // 83 S
    {0x7E,0x18,0x18,0x18,0x18,0x18,0x18,0x00}, // 84 T
    {0x66,0x66,0x66,0x66,0x66,0x66,0x3C,0x00}, // 85 U
    {0x66,0x66,0x66,0x66,0x66,0x3C,0x18,0x00}, // 86 V
    {0x63,0x63,0x63,0x6B,0x7F,0x77,0x63,0x00}, // 87 W
    {0x66,0x66,0x3C,0x18,0x3C,0x66,0x66,0x00}, // 88 X
    {0x66,0x66,0x66,0x3C,0x18,0x18,0x18,0x00}, // 89 Y
    {0x7E,0x06,0x0C,0x18,0x30,0x60,0x7E,0x00}, // 90 Z
    {0x7C,0x60,0x60,0x60,0x60,0x60,0x7C,0x00}, // 91 [
    {0x60,0x60,0x30,0x18,0x0C,0x06,0x06,0x00}, // 92 backslash
    {0x3E,0x06,0x06,0x06,0x06,0x06,0x3E,0x00}, // 93 ]
    {0x18,0x3C,0x66,0x00,0x00,0x00,0x00,0x00}, // 94 ^
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF}, // 95 _
    {0x30,0x18,0x0C,0x00,0x00,0x00,0x00,0x00}, // 96 `
    {0x00,0x00,0x3C,0x06,0x3E,0x66,0x3E,0x00}, // 97 a
    {0x60,0x60,0x7C,0x66,0x66,0x66,0x7C,0x00}, // 98 b
    {0x00,0x00,0x3C,0x66,0x60,0x66,0x3C,0x00}, // 99 c
    {0x06,0x06,0x3E,0x66,0x66,0x66,0x3E,0x00}, // 100 d
    {0x00,0x00,0x3C,0x66,0x7E,0x60,0x3C,0x00}, // 101 e
    {0x1C,0x30,0x30,0x7C,0x30,0x30,0x30,0x00}, // 102 f
    {0x00,0x00,0x3E,0x66,0x66,0x3E,0x06,0x3C}, // 103 g
    {0x60,0x60,0x7C,0x66,0x66,0x66,0x66,0x00}, // 104 h
    {0x18,0x00,0x38,0x18,0x18,0x18,0x3C,0x00}, // 105 i
    {0x0C,0x00,0x1C,0x0C,0x0C,0x6C,0x38,0x00}, // 106 j
    {0x60,0x60,0x66,0x6C,0x78,0x6C,0x66,0x00}, // 107 k
    {0x38,0x18,0x18,0x18,0x18,0x18,0x3C,0x00}, // 108 l
    {0x00,0x00,0x76,0x7F,0x6B,0x63,0x63,0x00}, // 109 m
    {0x00,0x00,0x7C,0x66,0x66,0x66,0x66,0x00}, // 110 n
    {0x00,0x00,0x3C,0x66,0x66,0x66,0x3C,0x00}, // 111 o
    {0x00,0x00,0x7C,0x66,0x66,0x7C,0x60,0x60}, // 112 p
    {0x00,0x00,0x3E,0x66,0x66,0x3E,0x06,0x06}, // 113 q
    {0x00,0x00,0x7C,0x66,0x60,0x60,0x60,0x00}, // 114 r
    {0x00,0x00,0x3E,0x60,0x3C,0x06,0x7C,0x00}, // 115 s
    {0x30,0x30,0x7C,0x30,0x30,0x36,0x1C,0x00}, // 116 t
    {0x00,0x00,0x66,0x66,0x66,0x66,0x3E,0x00}, // 117 u
    {0x00,0x00,0x66,0x66,0x66,0x3C,0x18,0x00}, // 118 v
    {0x00,0x00,0x63,0x6B,0x7F,0x36,0x22,0x00}, // 119 w
    {0x00,0x00,0x66,0x3C,0x18,0x3C,0x66,0x00}, // 120 x
    {0x00,0x00,0x66,0x66,0x66,0x3E,0x0C,0x78}, // 121 y
    {0x00,0x00,0x7E,0x0C,0x18,0x30,0x7E,0x00}, // 122 z
    {0x0E,0x18,0x18,0x70,0x18,0x18,0x0E,0x00}, // 123 {
    {0x18,0x18,0x18,0x00,0x18,0x18,0x18,0x00}, // 124 |
    {0x70,0x18,0x18,0x0E,0x18,0x18,0x70,0x00}, // 125 }
    {0x32,0x4C,0x00,0x00,0x00,0x00,0x00,0x00}, // 126 ~
};

// ============================================================
// Global objects
// ============================================================
U8G2_LS013B7DH03_128X128_F_4W_SW_SPI u8g2;
uint8_t* buf_ptr = nullptr;
uint16_t buf_len = 2048;
// Also defined in sim_config.h as: extern uint16_t buf_len;

// ============================================================
// SimU8g2 implementation
// ============================================================
U8G2_LS013B7DH03_128X128_F_4W_SW_SPI::U8G2_LS013B7DH03_128X128_F_4W_SW_SPI()
    : drawColor(1)
    , glyph_w(8), glyph_h(8), glyph_baseline(7)
    , is_big_font(false)
    , font_direction(0)
    , cursor_x(0), cursor_y(0)
    , rotation(0)
{
    clip = {0, 0, DISP_W - 1, DISP_H - 1};
    memset(buffer, 0, 2048);
}

void U8G2_LS013B7DH03_128X128_F_4W_SW_SPI::setDisplayRotation(const u8g2_cb_t* rot) {
    rotation = (uint8_t)(uintptr_t)rot;
}

void U8G2_LS013B7DH03_128X128_F_4W_SW_SPI::set_pixel(int x, int y) {
    if (x < clip.x1 || x > clip.x2 || y < clip.y1 || y > clip.y2) return;
    if (x < 0 || x >= DISP_W || y < 0 || y >= DISP_H) return;
    buffer[y * 16 + (x >> 3)] |= (0x80 >> (x & 7));
}

void U8G2_LS013B7DH03_128X128_F_4W_SW_SPI::clear_pixel(int x, int y) {
    if (x < clip.x1 || x > clip.x2 || y < clip.y1 || y > clip.y2) return;
    if (x < 0 || x >= DISP_W || y < 0 || y >= DISP_H) return;
    buffer[y * 16 + (x >> 3)] &= ~(0x80 >> (x & 7));
}

void U8G2_LS013B7DH03_128X128_F_4W_SW_SPI::xor_pixel(int x, int y) {
    if (x < clip.x1 || x > clip.x2 || y < clip.y1 || y > clip.y2) return;
    if (x < 0 || x >= DISP_W || y < 0 || y >= DISP_H) return;
    buffer[y * 16 + (x >> 3)] ^= (0x80 >> (x & 7));
}

void U8G2_LS013B7DH03_128X128_F_4W_SW_SPI::hline(int x, int y, int w) {
    for (int i = 0; i < w; ++i) {
        int px = x + i;
        if (px < clip.x1 || px > clip.x2) continue;
        if (y < clip.y1 || y > clip.y2) continue;
        if (px < 0 || px >= DISP_W || y < 0 || y >= DISP_H) continue;
        if (drawColor == 1) set_pixel(px, y);
        else if (drawColor == 0) clear_pixel(px, y);
        else if (drawColor == 2) xor_pixel(px, y);
    }
}

void U8G2_LS013B7DH03_128X128_F_4W_SW_SPI::vline(int x, int y, int h) {
    for (int i = 0; i < h; ++i) {
        int py = y + i;
        if (x < clip.x1 || x > clip.x2) continue;
        if (py < clip.y1 || py > clip.y2) continue;
        if (x < 0 || x >= DISP_W || py < 0 || py >= DISP_H) continue;
        if (drawColor == 1) set_pixel(x, py);
        else if (drawColor == 0) clear_pixel(x, py);
        else if (drawColor == 2) xor_pixel(x, py);
    }
}

void U8G2_LS013B7DH03_128X128_F_4W_SW_SPI::drawBox(int x, int y, int w, int h) {
    int x2 = x + w - 1, y2 = y + h - 1;
    int cx1 = (std::max)(clip.x1, x), cy1 = (std::max)(clip.y1, y);
    int cx2 = (std::min)(clip.x2, x2), cy2 = (std::min)(clip.y2, y2);
    for (int row = cy1; row <= cy2; ++row)
        for (int col = cx1; col <= cx2; ++col) {
            if (drawColor == 1) set_pixel(col, row);
            else if (drawColor == 0) clear_pixel(col, row);
            else if (drawColor == 2) xor_pixel(col, row);
        }
}

void U8G2_LS013B7DH03_128X128_F_4W_SW_SPI::drawFrame(int x, int y, int w, int h) {
    hline(x, y, w);
    hline(x, y + h - 1, w);
    vline(x, y, h);
    vline(x + w - 1, y, h);
}

void U8G2_LS013B7DH03_128X128_F_4W_SW_SPI::drawRBox(int x, int y, int w, int h, float r) {
    int rr = (int)(r + 0.5f);
    if (rr <= 0) { drawBox(x, y, w, h); return; }

    // 匹配U8g2原生drawRBox实现:
    // 1. 画4个1/4圆角（使用drawDisc的象限模式）
    // 2. 画顶部和底部横条（连接左右圆角之间）
    // 3. 画中间填充区域

    // 圆角圆心坐标
    int cx_l = x + rr;          // 左侧圆心x
    int cx_r = x + w - rr - 1;  // 右侧圆心x
    int cy_u = y + rr;          // 上方圆心y
    int cy_l = y + h - rr - 1;  // 下方圆心y

    // 画4个1/4圆盘（使用像素循环，匹配u8g2 drawDisc象限方式）
    // UL: px <= cx_l && py <= cy_u  (Upper-Left)
    // UR: px >= cx_r && py <= cy_u  (Upper-Right)
    // LL: px <= cx_l && py >= cy_l  (Lower-Left)
    // LR: px >= cx_r && py >= cy_l  (Lower-Right)
    for (int dy = 0; dy <= rr; ++dy) {
        for (int dx = 0; dx <= rr; ++dx) {
            if (dx * dx + dy * dy > rr * rr) continue; // 在圆内才绘制

            // 上左象限
            { int px = cx_l - dx, py = cy_u - dy; if (drawColor == 1) set_pixel(px, py); else if (drawColor == 0) clear_pixel(px, py); else if (drawColor == 2) xor_pixel(px, py); }
            // 上右象限
            { int px = cx_r + dx, py = cy_u - dy; if (drawColor == 1) set_pixel(px, py); else if (drawColor == 0) clear_pixel(px, py); else if (drawColor == 2) xor_pixel(px, py); }
            // 下左象限
            { int px = cx_l - dx, py = cy_l + dy; if (drawColor == 1) set_pixel(px, py); else if (drawColor == 0) clear_pixel(px, py); else if (drawColor == 2) xor_pixel(px, py); }
            // 下右象限
            { int px = cx_r + dx, py = cy_l + dy; if (drawColor == 1) set_pixel(px, py); else if (drawColor == 0) clear_pixel(px, py); else if (drawColor == 2) xor_pixel(px, py); }
        }
    }

    // 顶部横条（在左右圆角之间，不重叠）
    int inner_w = w - rr * 2;
    if (inner_w >= 3) {
        drawBox(cx_l + 1, y, inner_w - 2, rr + 1);
        // 底部横条
        drawBox(cx_l + 1, cy_l, inner_w - 2, rr + 1);
    }

    // 中间填充区域（全宽，在上下圆角之间，不重叠）
    int inner_h = h - rr * 2;
    if (inner_h >= 3) {
        drawBox(x, cy_u + 1, w, inner_h - 2);
    }
}

void U8G2_LS013B7DH03_128X128_F_4W_SW_SPI::drawRFrame(int x, int y, int w, int h, float r) {
    int rr = (int)(r + 0.5f);
    if (rr <= 0) { drawFrame(x, y, w, h); return; }
    int saved = drawColor;
    drawColor = 1;
    drawRBox(x, y, w, h, r);
    drawColor = 0;
    drawRBox(x + 1, y + 1, w - 2, h - 2, (float)(rr - 1));
    drawColor = (saved == 2) ? 1 : saved;
    if (saved == 2) {
        drawColor = 1;
        drawRBox(x, y, w, h, r);
        drawColor = 0;
        drawRBox(x + 1, y + 1, w - 2, h - 2, (float)(rr - 1));
        drawColor = saved;
    }
}

void U8G2_LS013B7DH03_128X128_F_4W_SW_SPI::drawHLine(int x, int y, int w) { hline(x, y, w); }
void U8G2_LS013B7DH03_128X128_F_4W_SW_SPI::drawVLine(int x, int y, int h) { vline(x, y, h); }

void U8G2_LS013B7DH03_128X128_F_4W_SW_SPI::drawLine(int x1, int y1, int x2, int y2) {
    int dx = abs(x2 - x1), sx = x1 < x2 ? 1 : -1;
    int dy = -abs(y2 - y1), sy = y1 < y2 ? 1 : -1;
    int err = dx + dy;
    while (true) {
        if (drawColor == 1) set_pixel(x1, y1);
        else if (drawColor == 0) clear_pixel(x1, y1);
        else if (drawColor == 2) xor_pixel(x1, y1);
        if (x1 == x2 && y1 == y2) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x1 += sx; }
        if (e2 <= dx) { err += dx; y1 += sy; }
    }
}

int U8G2_LS013B7DH03_128X128_F_4W_SW_SPI::getStrWidth(const char* s) {
    if (!s) return 0;
    if (is_big_font) {
        // Use GDI to measure text width for big font (always 36px)
        HDC hdc = CreateCompatibleDC(NULL);
        BITMAPINFO bmi = {0};
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = 1;
        bmi.bmiHeader.biHeight = 1;
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 32;
        bmi.bmiHeader.biCompression = BI_RGB;
        uint32_t* px;
        HBITMAP bmp = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, (void**)&px, NULL, 0);
        HGDIOBJ old_bmp = SelectObject(hdc, bmp);

        HFONT hFont = CreateFontA(-36, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                                  DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                                  CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                                  DEFAULT_PITCH | FF_DONTCARE, "Arial");
        HGDIOBJ old_font = SelectObject(hdc, hFont);

        SIZE sz;
        GetTextExtentPoint32A(hdc, s, (int)strlen(s), &sz);

        SelectObject(hdc, old_font);
        SelectObject(hdc, old_bmp);
        DeleteObject(hFont);
        DeleteObject(bmp);
        DeleteDC(hdc);
        return sz.cx;
    }
    int w = 0;
    const uint8_t* wt = width_helvPix;
    while (*s) {
        unsigned char c = (unsigned char)*s;
        if (c >= 32 && c <= 126) {
            w += wt[c - 32];
            w += 1;
        }
        ++s;
    }
    return w;
}

void U8G2_LS013B7DH03_128X128_F_4W_SW_SPI::drawStr(int x, int y, const char* s) {
    if (!s) return;

    if (is_big_font && font_direction == 0) {
        // Render big font using Windows GDI (always 36px regardless of screen DPI)
        HDC hdc = CreateCompatibleDC(NULL);
        // Use 48 rows to ensure enough room for the 36px font (ascent ~29, descent ~7)
        const int BITMAP_ROWS = 48;
        BITMAPINFO bmi = {0};
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = DISP_W;
        bmi.bmiHeader.biHeight = -BITMAP_ROWS; // top-down
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 32;
        bmi.bmiHeader.biCompression = BI_RGB;
        uint32_t* pixels;
        HBITMAP bmp = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, (void**)&pixels, NULL, 0);
        HGDIOBJ old_bmp = SelectObject(hdc, bmp);

        // Always use 36px font (negative = char height in device units)
        HFONT hFont = CreateFontA(-36, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                                  DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                                  CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
                                  DEFAULT_PITCH | FF_DONTCARE, "Arial");
        HGDIOBJ old_font = SelectObject(hdc, hFont);

        SetTextColor(hdc, RGB(255, 255, 255));
        SetBkColor(hdc, RGB(0, 0, 0));
        SetBkMode(hdc, OPAQUE);

        // Fill bitmap with black
        memset(pixels, 0, DISP_W * BITMAP_ROWS * 4);

        // Get font metrics to determine baseline position
        TEXTMETRICA tm;
        GetTextMetricsA(hdc, &tm);
        int ascent = tm.tmAscent;

        // Draw text with top at row 0 so the full character fits within the bitmap
        // (TA_TOP default: y specifies the top of the character cell)
        TextOutA(hdc, 0, 0, s, (int)strlen(s));

        // baseline_y maps bitmap row=ascent (the baseline) to target display y-position
        // The u8g2 drawStr y coordinate is the baseline
        int baseline_y = y - ascent;
        // Copy rendered pixels to display buffer
        for (int row = 0; row < BITMAP_ROWS; ++row) {
            int sy = baseline_y + row;
            if (sy < 0 || sy >= DISP_H) continue;
            for (int col = 0; col < DISP_W; ++col) {
                int sx = x + col;
                if (sx < 0 || sx >= DISP_W) continue;
                if (pixels[row * DISP_W + col] != 0) {
                    if (drawColor == 1) set_pixel(sx, sy);
                    else if (drawColor == 0) clear_pixel(sx, sy);
                    else if (drawColor == 2) xor_pixel(sx, sy);
                }
            }
        }

        SelectObject(hdc, old_font);
        SelectObject(hdc, old_bmp);
        DeleteObject(hFont);
        DeleteObject(bmp);
        DeleteDC(hdc);
        return;
    }

    int px = x;
    int py = y;
    const uint8_t* wt = width_helvPix;
    while (*s) {
        unsigned char c = (unsigned char)*s;
        if (c >= 32 && c <= 126) {
            int gw = wt[c - 32];
            const uint8_t* g = glyph_helvPix[c - 32];
            if (font_direction == 0) {
                int baseline_y = y - 7;
                for (int row = 0; row < 8; ++row) {
                    for (int col = 0; col < gw && col < 8; ++col) {
                        if (g[row] & (0x80 >> col)) {
                            int sx = px + col, sy = baseline_y + row;
                            if (drawColor == 1) set_pixel(sx, sy);
                            else if (drawColor == 0) clear_pixel(sx, sy);
                            else if (drawColor == 2) xor_pixel(sx, sy);
                        }
                    }
                }
                px += gw + 1;
            } else if (font_direction == 1) {
                // dir=1 (90°CW): x=fixed horizontal, y=vertical flow start
                int sx_base = x;
                int sy_flow = y - 7;
                for (int row = 0; row < 8; ++row) {
                    for (int col = 0; col < gw && col < 8; ++col) {
                        if (g[row] & (0x80 >> col)) {
                            int sx = sx_base + (7 - row), sy = sy_flow + col;
                            if (drawColor == 1) set_pixel(sx, sy);
                            else if (drawColor == 0) clear_pixel(sx, sy);
                            else if (drawColor == 2) xor_pixel(sx, sy);
                        }
                    }
                }
                sy_flow += gw + 1;
                y = sy_flow + 7;
            } else if (font_direction == 2) {
                int baseline_y = y - 7;
                for (int row = 0; row < 8; ++row) {
                    for (int col = 0; col < gw && col < 8; ++col) {
                        if (g[row] & (0x80 >> col)) {
                            int sx = px - col, sy = baseline_y + (7 - row);
                            if (drawColor == 1) set_pixel(sx, sy);
                            else if (drawColor == 0) clear_pixel(sx, sy);
                            else if (drawColor == 2) xor_pixel(sx, sy);
                        }
                    }
                }
                px -= gw + 1;
            } else if (font_direction == 3) {
                // dir=3 (270°CW): x=fixed horizontal, y=vertical flow start (upward)
                int sx_base = x;
                int sy_flow = y - 4;  // LIST_TEXT_S baseline
                for (int row = 0; row < 8; ++row) {
                    for (int col = 0; col < gw && col < 8; ++col) {
                        if (g[row] & (0x80 >> col)) {
                            int sx = sx_base + row, sy = sy_flow - col;
                            if (drawColor == 1) set_pixel(sx, sy);
                            else if (drawColor == 0) clear_pixel(sx, sy);
                            else if (drawColor == 2) xor_pixel(sx, sy);
                        }
                    }
                }
                sy_flow -= gw + 1;
                y = sy_flow + 4;
            }
        }
        ++s;
    }
}

void U8G2_LS013B7DH03_128X128_F_4W_SW_SPI::drawXBMP(int x, int y, int w, int h, const uint8_t* bitmap) {
    int bpr = (w + 7) / 8;
    for (int row = 0; row < h; ++row) {
        for (int col = 0; col < w; ++col) {
            int byte_idx = row * bpr + (col >> 3);
            // U8g2的drawXBMP使用LSB-first: bit0(0x01)=最左像素, bit7(0x80)=最右像素
            if (bitmap[byte_idx] & (1 << (col & 7))) {
                int sx = x + col, sy = y + row;
                if (sx < 0 || sx >= DISP_W || sy < 0 || sy >= DISP_H) continue;
                if (drawColor == 1) set_pixel(sx, sy);
                else if (drawColor == 0) clear_pixel(sx, sy);
                else if (drawColor == 2) xor_pixel(sx, sy);
            }
        }
    }
}

void U8G2_LS013B7DH03_128X128_F_4W_SW_SPI::setDrawColor(int c) { drawColor = c; }

void U8G2_LS013B7DH03_128X128_F_4W_SW_SPI::setFont(const uint8_t* f) {
    if (f == u8g2_font_helvB24_tr || f == (const uint8_t*)1) {
        is_big_font = true;
        glyph_w = 24; glyph_h = 24; glyph_baseline = 18;
    } else {
        is_big_font = false;
        glyph_w = 8; glyph_h = 8; glyph_baseline = 7;
    }
}

void U8G2_LS013B7DH03_128X128_F_4W_SW_SPI::setFontDirection(int d) { font_direction = d; }
void U8G2_LS013B7DH03_128X128_F_4W_SW_SPI::setCursor(int x, int y) { cursor_x = x; cursor_y = y; }
void U8G2_LS013B7DH03_128X128_F_4W_SW_SPI::setContrast(int) {}

void U8G2_LS013B7DH03_128X128_F_4W_SW_SPI::clearBuffer() { memset(buffer, 0, 2048); }
void U8G2_LS013B7DH03_128X128_F_4W_SW_SPI::sendBuffer() {}

void U8G2_LS013B7DH03_128X128_F_4W_SW_SPI::print(const char* s) {
    if (!s) return;
    drawStr(cursor_x, cursor_y, s);
    cursor_x += getStrWidth(s);
}

void U8G2_LS013B7DH03_128X128_F_4W_SW_SPI::print(int val) {
    char buf[16];
    snprintf(buf, sizeof(buf), "%d", val);
    print(buf);
}

void U8G2_LS013B7DH03_128X128_F_4W_SW_SPI::print(float val) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%.2f", (double)val);
    print(buf);
}

void U8G2_LS013B7DH03_128X128_F_4W_SW_SPI::begin() {
    clearBuffer();
    buf_ptr = buffer;
    buf_len = 2048;
}

void U8G2_LS013B7DH03_128X128_F_4W_SW_SPI::setClipWindow(int x1, int y1, int x2, int y2) {
    clip.x1 = (std::max)(0, x1);
    clip.y1 = (std::max)(0, y1);
    clip.x2 = (std::min)(DISP_W - 1, x2);
    clip.y2 = (std::min)(DISP_H - 1, y2);
}

void U8G2_LS013B7DH03_128X128_F_4W_SW_SPI::setMaxClipWindow() {
    clip = {0, 0, DISP_W - 1, DISP_H - 1};
}

// ============================================================
// Display init
// ============================================================
void lcd_init() {
    u8g2.begin();
    buf_ptr = u8g2.getBufferPtr();
    buf_len = 2048;
}

void lcd_reset_vcom() {}
