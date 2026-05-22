#include "u8g2_adapter.h"
#include "oled.h"
#include <string.h>
#include <stdio.h>

u8g2_adapter_t u8g2_state = {1, 8, 0, 0, 0};

uint8_t *buf_ptr = (uint8_t *)OLED_GRAM;
uint16_t buf_len = 128 * 16;

const uint8_t u8g2_font_helvB24_tr = 24;
const uint8_t u8g2_font_HelvetiPixel_tr = 8;

static void setpixel(int16_t x, int16_t y) {
    uint8_t i, m;
    if (x < 0 || x >= 128 || y < 0 || y >= 128) return;
    if (u8g2_state.draw_color == 1) {
        OLED_DrawPoint((uint8_t)x, (uint8_t)y, 1);
    } else if (u8g2_state.draw_color == 0) {
        OLED_DrawPoint((uint8_t)x, (uint8_t)y, 0);
    } else if (u8g2_state.draw_color == 2) {
        i = (uint8_t)(y / 8);
        m = (uint8_t)(1 << (y % 8));
        OLED_GRAM[(uint8_t)x][i] ^= m;
    }
}

void u8g2_Begin(void) {
    u8g2_state.draw_color = 1;
    u8g2_state.font_id = 8;
    u8g2_state.cursor_x = 0;
    u8g2_state.cursor_y = 0;
    u8g2_state.font_direction = 0;
}

void u8g2_ClearBuffer(void) {
    memset(OLED_GRAM, 0, sizeof(OLED_GRAM));
}

void u8g2_SendBuffer(void) {
    OLED_Refresh();
}

void u8g2_DrawPixel(int16_t x, int16_t y) {
    setpixel(x, y);
}

void u8g2_DrawBox(int16_t x, int16_t y, int16_t w, int16_t h) {
    int16_t py, px;
    for (py = y; py < y + h; py++)
        for (px = x; px < x + w; px++)
            setpixel(px, py);
}

void u8g2_DrawFrame(int16_t x, int16_t y, int16_t w, int16_t h) {
    int16_t xe, ye, px, py;
    if (w < 1 || h < 1) return;
    xe = x + w - 1;
    ye = y + h - 1;
    for (px = x; px <= xe; px++) { setpixel(px, y); setpixel(px, ye); }
    for (py = y; py <= ye; py++) { setpixel(x, py); setpixel(xe, py); }
}

static void draw_circle_points(int16_t xc, int16_t yc, int16_t x, int16_t y) {
    if (x == 0) { setpixel(xc, yc+y); setpixel(xc, yc-y); setpixel(xc+y, yc); setpixel(xc-y, yc); }
    else if (x == y) { setpixel(xc+x, yc+y); setpixel(xc-x, yc+y); setpixel(xc+x, yc-y); setpixel(xc-x, yc-y); }
    else if (x < y) {
        setpixel(xc+x, yc+y); setpixel(xc-x, yc+y); setpixel(xc+x, yc-y); setpixel(xc-x, yc-y);
        setpixel(xc+y, yc+x); setpixel(xc-y, yc+x); setpixel(xc+y, yc-x); setpixel(xc-y, yc-x);
    }
}

void u8g2_DrawCircle(int16_t x0, int16_t y0, int16_t rad) {
    int16_t x = 0, y = rad, d = 3 - 2 * rad;
    if (rad < 0) return;
    while (x <= y) {
        draw_circle_points(x0, y0, x, y);
        if (d < 0) d += 4 * x + 6;
        else { d += 4 * (x - y) + 10; y--; }
        x++;
    }
}

void u8g2_DrawRBox(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r) {
    int16_t dx, dy;
    if (r < 1) { u8g2_DrawBox(x, y, w, h); return; }
    u8g2_DrawBox(x + r, y, w - 2 * r, h);
    if (h > 2 * r) {
        u8g2_DrawBox(x, y + r, r, h - 2 * r);
        u8g2_DrawBox(x + w - r, y + r, r, h - 2 * r);
    }
    for (dy = -r; dy <= r; dy++)
        for (dx = -r; dx <= r; dx++)
            if (dx * dx + dy * dy <= r * r) {
                if (dx >= 0 && dy >= 0) setpixel(x + w - r + dx, y + h - r + dy);
                if (dx <= 0 && dy >= 0) setpixel(x + r + dx, y + h - r + dy);
                if (dx >= 0 && dy <= 0) setpixel(x + w - r + dx, y + r + dy);
                if (dx <= 0 && dy <= 0) setpixel(x + r + dx, y + r + dy);
            }
}

void u8g2_DrawRFrame(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r) {
    int16_t px, py, dx, dy, d2;
    if (r < 1) { u8g2_DrawFrame(x, y, w, h); return; }
    for (px = x + r; px < x + w - r; px++) { setpixel(px, y); setpixel(px, y + h - 1); }
    for (py = y + r; py < y + h - r; py++) { setpixel(x, py); setpixel(x + w - 1, py); }
    for (dy = -r; dy <= r; dy++)
        for (dx = -r; dx <= r; dx++) {
            d2 = dx * dx + dy * dy;
            if (d2 <= r * r + r && d2 >= r * r - r) {
                if (dx >= 0 && dy >= 0) setpixel(x + w - r - 1 + dx, y + h - r - 1 + dy);
                if (dx <= 0 && dy >= 0) setpixel(x + r + dx, y + h - r - 1 + dy);
                if (dx >= 0 && dy <= 0) setpixel(x + w - r - 1 + dx, y + r + dy);
                if (dx <= 0 && dy <= 0) setpixel(x + r + dx, y + r + dy);
            }
        }
}

void u8g2_DrawLine(int16_t x1, int16_t y1, int16_t x2, int16_t y2) {
    OLED_DrawLine((uint8_t)x1, (uint8_t)y1, (uint8_t)x2, (uint8_t)y2, u8g2_state.draw_color ? 1 : 0);
}

void u8g2_DrawHLine(int16_t x, int16_t y, int16_t w) {
    int16_t px;
    for (px = x; px < x + w; px++)
        setpixel(px, y);
}

void u8g2_DrawVLine(int16_t x, int16_t y, int16_t h) {
    int16_t py;
    for (py = y; py < y + h; py++)
        setpixel(x, py);
}

void u8g2_DrawXBMP(int16_t x, int16_t y, int16_t w, int16_t h, const uint8_t *bitmap) {
    int16_t bytes_per_row, py, px;
    uint8_t byte_val;
    bytes_per_row = (w + 7) / 8;
    for (py = 0; py < h; py++) {
        for (px = 0; px < w; px++) {
            byte_val = bitmap[py * bytes_per_row + px / 8];
            if (byte_val & (0x80 >> (px % 8)))
                setpixel(x + px, y + py);
        }
    }
}

void u8g2_SetFont(const uint8_t *font) {
    if (font == &u8g2_font_helvB24_tr)
        u8g2_state.font_id = 24;
    else
        u8g2_state.font_id = 8;
}

void u8g2_DrawStr(int16_t x, int16_t y, const char *str) {
    int16_t adjusted_y = y - (int16_t)u8g2_state.font_id;
    if (adjusted_y < 0) adjusted_y = 0;
    OLED_ShowString((uint8_t)x, (uint8_t)adjusted_y, (uint8_t *)str, u8g2_state.font_id, u8g2_state.draw_color ? 1 : 0);
}

int16_t u8g2_DrawStrRet(int16_t x, int16_t y, const char *str) {
    u8g2_DrawStr(x, y, str);
    return (int16_t)(x + strlen(str) * (u8g2_state.font_id / 2));
}

void u8g2_SetCursor(int16_t x, int16_t y) {
    u8g2_state.cursor_x = x;
    u8g2_state.cursor_y = y;
}

void u8g2_SetFontDirection(uint8_t dir) {
    u8g2_state.font_direction = dir;
}

void u8g2_Print(uint32_t num) {
    char buf[16];
    sprintf(buf, "%lu", (unsigned long)num);
    u8g2_DrawStr(u8g2_state.cursor_x, u8g2_state.cursor_y, buf);
}

void u8g2_SetDrawColor(uint8_t color) {
    u8g2_state.draw_color = color;
}

void u8g2_SetContrast(uint8_t value) {
    OLED_WR_Byte(0x81, OLED_CMD);
    OLED_WR_Byte(value, OLED_CMD);
}

void u8g2_SetPowerSave(uint8_t on) {
    if (on)
        OLED_WR_Byte(0xAE, OLED_CMD);
    else
        OLED_WR_Byte(0xAF, OLED_CMD);
}

void u8g2_PrintStr(const char *str) {
    u8g2_DrawStr(u8g2_state.cursor_x, u8g2_state.cursor_y, str);
    u8g2_state.cursor_x += u8g2_GetStrWidth(str);
}

void u8g2_PrintFloat(float val) {
    char buf[16];
    int whole, frac;
    whole = (int)val;
    frac = (int)((val - whole) * 100 + 0.5f);
    if (frac < 0) frac = 0;
    if (frac > 99) frac = 99;
    sprintf(buf, "%d.%02d", whole, frac);
    u8g2_DrawStr(u8g2_state.cursor_x, u8g2_state.cursor_y, buf);
    u8g2_state.cursor_x += u8g2_GetStrWidth(buf);
}

int16_t u8g2_GetStrWidth(const char *str) {
    int len = 0;
    while (*str) { len++; str++; }
    if (u8g2_state.font_id >= 24) return len * 14;
    if (u8g2_state.font_id >= 16) return len * 9;
    return len * 5;
}

uint8_t* u8g2_GetBufferPtr(void) {
    return (uint8_t *)OLED_GRAM;
}

uint8_t u8g2_GetBufferTileHeight(void) {
    return 16;
}

uint8_t u8g2_GetBufferTileWidth(void) {
    return 16;
}

uint16_t u8g2_GetBufferLen(void) {
    return 128 * 8;
}
