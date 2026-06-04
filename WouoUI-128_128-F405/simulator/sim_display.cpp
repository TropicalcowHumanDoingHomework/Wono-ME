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

// HelvetiPixel character widths (ASCII 32-126)
static const uint8_t width_helvPix[95] = {
     3,  3,  4,  8,  7, 10,  9,  3,  4,  4,  6,  7,  3,  4,  3,  6,
     7,  7,  7,  7,  7,  7,  7,  7,  7,  7,  3,  3,  7,  7,  7,  6,
    11,  8,  7,  8,  8,  7,  6,  8,  8,  4,  6,  7,  6,  9,  8,  8,
     7,  8,  7,  7,  7,  8,  8, 11,  8,  8,  7,  4,  7,  4,  7,  7,
     4,  7,  7,  6,  7,  7,  5,  7,  7,  3,  4,  7,  3, 10,  7,  7,
     7,  7,  5,  6,  5,  7,  7, 10,  7,  7,  6,  5,  3,  5,  7
};

// HelvetiPixel font bitmaps (8x8 per character, ASCII 32-126)
static const uint8_t glyph_helvPix[95][8] = {
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00},
    {0x40,0x40,0x40,0x40,0x40,0x00,0x40,0x00},
    {0x50,0x50,0x50,0x00,0x00,0x00,0x00,0x00},
    {0x00,0x24,0x7E,0x24,0x24,0x7E,0x24,0x00},
    {0x08,0x3E,0x48,0x3E,0x12,0x7E,0x08,0x00},
    {0x42,0xA4,0x48,0x10,0x24,0x4A,0x84,0x00},
    {0x30,0x48,0x30,0x28,0x44,0x44,0x38,0x00},
    {0x40,0x40,0x40,0x00,0x00,0x00,0x00,0x00},
    {0x20,0x10,0x10,0x10,0x10,0x10,0x20,0x00},
    {0x40,0x20,0x20,0x20,0x20,0x20,0x40,0x00},
    {0x10,0x54,0x38,0x10,0x38,0x54,0x10,0x00},
    {0x00,0x10,0x10,0x7C,0x10,0x10,0x00,0x00},
    {0x00,0x00,0x00,0x00,0x00,0x40,0x40,0x80},
    {0x00,0x00,0x00,0x70,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x00,0x00,0x00,0x40,0x40,0x00},
    {0x04,0x04,0x08,0x10,0x20,0x40,0x80,0x00},
    {0x38,0x44,0x44,0x54,0x44,0x44,0x38,0x00},
    {0x10,0x30,0x10,0x10,0x10,0x10,0x38,0x00},
    {0x38,0x44,0x04,0x18,0x20,0x40,0x7C,0x00},
    {0x38,0x44,0x04,0x18,0x04,0x44,0x38,0x00},
    {0x08,0x18,0x28,0x48,0x7C,0x08,0x08,0x00},
    {0x7C,0x40,0x78,0x04,0x04,0x44,0x38,0x00},
    {0x38,0x40,0x78,0x44,0x44,0x44,0x38,0x00},
    {0x7E,0x04,0x08,0x10,0x20,0x40,0x40,0x00},
    {0x38,0x44,0x44,0x38,0x44,0x44,0x38,0x00},
    {0x38,0x44,0x44,0x44,0x3C,0x04,0x38,0x00},
    {0x00,0x40,0x40,0x00,0x00,0x40,0x40,0x00},
    {0x00,0x40,0x40,0x00,0x00,0x40,0x40,0x80},
    {0x04,0x08,0x10,0x20,0x10,0x08,0x04,0x00},
    {0x00,0x00,0x7E,0x00,0x7E,0x00,0x00,0x00},
    {0x20,0x10,0x08,0x04,0x08,0x10,0x20,0x00},
    {0x38,0x44,0x04,0x08,0x10,0x00,0x10,0x00},
    {0x38,0x44,0x5C,0x54,0x5E,0x40,0x38,0x00},
    {0x10,0x28,0x44,0x44,0x7C,0x44,0x44,0x00},
    {0x78,0x44,0x44,0x78,0x44,0x44,0x78,0x00},
    {0x38,0x44,0x40,0x40,0x40,0x44,0x38,0x00},
    {0x78,0x44,0x44,0x44,0x44,0x44,0x78,0x00},
    {0x7C,0x40,0x40,0x78,0x40,0x40,0x7C,0x00},
    {0x7C,0x40,0x40,0x78,0x40,0x40,0x40,0x00},
    {0x38,0x44,0x40,0x5C,0x44,0x44,0x3C,0x00},
    {0x44,0x44,0x44,0x7C,0x44,0x44,0x44,0x00},
    {0x70,0x20,0x20,0x20,0x20,0x20,0x70,0x00},
    {0x04,0x04,0x04,0x04,0x44,0x44,0x38,0x00},
    {0x44,0x48,0x50,0x60,0x50,0x48,0x44,0x00},
    {0x40,0x40,0x40,0x40,0x40,0x40,0x7C,0x00},
    {0x44,0x6C,0x54,0x54,0x44,0x44,0x44,0x00},
    {0x44,0x64,0x54,0x4C,0x44,0x44,0x44,0x00},
    {0x38,0x44,0x44,0x44,0x44,0x44,0x38,0x00},
    {0x78,0x44,0x44,0x78,0x40,0x40,0x40,0x00},
    {0x38,0x44,0x44,0x44,0x54,0x48,0x34,0x00},
    {0x78,0x44,0x44,0x78,0x48,0x44,0x44,0x00},
    {0x38,0x44,0x40,0x38,0x04,0x44,0x38,0x00},
    {0x7C,0x10,0x10,0x10,0x10,0x10,0x10,0x00},
    {0x44,0x44,0x44,0x44,0x44,0x44,0x38,0x00},
    {0x44,0x44,0x44,0x28,0x28,0x10,0x10,0x00},
    {0x44,0x44,0x54,0x54,0x54,0x54,0x28,0x00},
    {0x44,0x44,0x28,0x10,0x28,0x44,0x44,0x00},
    {0x44,0x44,0x28,0x10,0x10,0x10,0x10,0x00},
    {0x7C,0x04,0x08,0x10,0x20,0x40,0x7C,0x00},
    {0xC0,0x80,0x80,0x80,0x80,0x80,0xC0,0x00},
    {0x80,0x40,0x20,0x10,0x08,0x04,0x02,0x00},
    {0x30,0x10,0x10,0x10,0x10,0x10,0x30,0x00},
    {0x10,0x28,0x44,0x00,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x00,0x00,0x00,0x00,0x7E,0x00},
    {0x40,0x20,0x10,0x00,0x00,0x00,0x00,0x00},
    {0x00,0x00,0x38,0x04,0x3C,0x44,0x3C,0x00},
    {0x40,0x40,0x58,0x64,0x44,0x64,0x58,0x00},
    {0x00,0x00,0x38,0x44,0x40,0x44,0x38,0x00},
    {0x04,0x04,0x3C,0x44,0x44,0x44,0x3C,0x00},
    {0x00,0x00,0x38,0x44,0x7C,0x40,0x38,0x00},
    {0x0C,0x10,0x3C,0x10,0x10,0x10,0x10,0x00},
    {0x00,0x00,0x3C,0x44,0x44,0x3C,0x04,0x38},
    {0x40,0x40,0x58,0x64,0x44,0x44,0x44,0x00},
    {0x40,0x00,0x40,0x40,0x40,0x40,0x40,0x00},
    {0x40,0x00,0x40,0x40,0x40,0x40,0xC0,0x80},
    {0x40,0x40,0x44,0x48,0x70,0x48,0x44,0x00},
    {0x80,0x80,0x80,0x80,0x80,0x80,0xE0,0x00},
    {0x00,0x00,0x68,0x54,0x54,0x44,0x44,0x00},
    {0x00,0x00,0x58,0x64,0x44,0x44,0x44,0x00},
    {0x00,0x00,0x38,0x44,0x44,0x44,0x38,0x00},
    {0x00,0x00,0x58,0x64,0x44,0x64,0x58,0x40},
    {0x00,0x00,0x3C,0x44,0x44,0x3C,0x04,0x04},
    {0x00,0x00,0x58,0xC8,0x40,0x40,0x40,0x00},
    {0x00,0x00,0x3C,0x40,0x38,0x04,0x78,0x00},
    {0x20,0x20,0x78,0x20,0x20,0x20,0x18,0x00},
    {0x00,0x00,0x44,0x44,0x44,0x44,0x3C,0x00},
    {0x00,0x00,0x44,0x44,0x44,0x28,0x10,0x00},
    {0x00,0x00,0x44,0x44,0x54,0x54,0x28,0x00},
    {0x00,0x00,0x44,0x28,0x10,0x28,0x44,0x00},
    {0x00,0x00,0x44,0x44,0x44,0x3C,0x04,0x38},
    {0x00,0x00,0x7C,0x08,0x10,0x20,0x7C,0x00},
    {0x08,0x10,0x10,0x60,0x10,0x10,0x08,0x00},
    {0x40,0x40,0x40,0x00,0x40,0x40,0x40,0x00},
    {0x40,0x20,0x20,0x18,0x20,0x20,0x40,0x00},
    {0x00,0x00,0x30,0x48,0x00,0x00,0x00,0x00},
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
{
    clip = {0, 0, DISP_W - 1, DISP_H - 1};
    memset(buffer, 0, 2048);
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
    drawBox(x + 1, y + 1, w - 2, h - 2);
    drawColor = (saved == 2) ? 1 : saved;
    if (saved == 2) {
        drawColor = 1;
        drawRBox(x, y, w, h, r);
        drawColor = 0;
        drawBox(x + 1, y + 1, w - 2, h - 2);
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
    const uint8_t* wt = width_helvPix;
    while (*s) {
        unsigned char c = (unsigned char)*s;
        if (c >= 32 && c <= 126) {
            int gw = wt[c - 32];
            int baseline_y = y - 7;
            const uint8_t* g = glyph_helvPix[c - 32];
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
// Compat stub structures for getU8x8/getU8g2
// ============================================================
struct u8x8_dummy { uint8_t x; };
struct u8g2_dummy { int tile_curr_row; };
static u8x8_dummy g_u8x8;
static u8g2_dummy g_u8g2;

u8x8_t* U8G2_LS013B7DH03_128X128_F_4W_SW_SPI::getU8x8() {
    return reinterpret_cast<u8x8_t*>(&g_u8x8);
}

u8g2_t* U8G2_LS013B7DH03_128X128_F_4W_SW_SPI::getU8g2() {
    return reinterpret_cast<u8g2_t*>(&g_u8g2);
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
