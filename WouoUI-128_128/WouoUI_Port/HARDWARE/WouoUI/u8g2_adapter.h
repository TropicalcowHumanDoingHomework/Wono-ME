#ifndef U8G2_ADAPTER_H
#define U8G2_ADAPTER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== C接口 ==================== */

/* 字体ID定义 */
#define FONT_SMALL   8
#define FONT_MEDIUM  16
#define FONT_LARGE   24

/* U8G2 适配器状态结构体 */
typedef struct {
    uint8_t draw_color;
    uint8_t font_id;
    int16_t cursor_x;
    int16_t cursor_y;
    uint8_t font_direction;
} u8g2_adapter_t;

/* 全局适配器实例 */
extern u8g2_adapter_t u8g2_state;

/* 初始化 */
void u8g2_Begin(void);

/* 缓冲区操作 */
void u8g2_ClearBuffer(void);
void u8g2_SendBuffer(void);

/* 基本绘图 */
void u8g2_DrawPixel(int16_t x, int16_t y);
void u8g2_DrawBox(int16_t x, int16_t y, int16_t w, int16_t h);
void u8g2_DrawFrame(int16_t x, int16_t y, int16_t w, int16_t h);
void u8g2_DrawRBox(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r);
void u8g2_DrawRFrame(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r);
void u8g2_DrawLine(int16_t x1, int16_t y1, int16_t x2, int16_t y2);
void u8g2_DrawCircle(int16_t x0, int16_t y0, int16_t rad);
void u8g2_DrawXBMP(int16_t x, int16_t y, int16_t w, int16_t h, const uint8_t *bitmap);
void u8g2_DrawHLine(int16_t x, int16_t y, int16_t w);
void u8g2_DrawVLine(int16_t x, int16_t y, int16_t h);

/* 文字 */
void u8g2_SetFont(const uint8_t *font);
void u8g2_DrawStr(int16_t x, int16_t y, const char *str);
int16_t u8g2_DrawStrRet(int16_t x, int16_t y, const char *str);
void u8g2_SetCursor(int16_t x, int16_t y);
void u8g2_SetFontDirection(uint8_t dir);
void u8g2_Print(uint32_t num);
void u8g2_PrintStr(const char *str);
void u8g2_PrintFloat(float val);
int16_t u8g2_GetStrWidth(const char *str);

/* 属性 */
void u8g2_SetDrawColor(uint8_t color);
void u8g2_SetContrast(uint8_t value);
void u8g2_SetPowerSave(uint8_t on);
uint8_t* u8g2_GetBufferPtr(void);
uint8_t u8g2_GetBufferTileHeight(void);
uint8_t u8g2_GetBufferTileWidth(void);
uint16_t u8g2_GetBufferLen(void);

/* === 字体常量(U8g2兼容,实际用font_id) === */
extern const uint8_t u8g2_font_helvB24_tr;
extern const uint8_t u8g2_font_HelvetiPixel_tr;

#ifdef __cplusplus
}

/* ==================== C++ 兼容类 ==================== */
/* 让 pages.cpp 可以继续使用 u8g2.drawBox() 语法 */

class U8G2 {
public:
    void begin() { u8g2_Begin(); }
    void clearBuffer() { u8g2_ClearBuffer(); }
    void sendBuffer() { u8g2_SendBuffer(); }

    void drawPixel(int16_t x, int16_t y) { u8g2_DrawPixel(x, y); }
    void drawBox(int16_t x, int16_t y, int16_t w, int16_t h) { u8g2_DrawBox(x, y, w, h); }
    void drawFrame(int16_t x, int16_t y, int16_t w, int16_t h) { u8g2_DrawFrame(x, y, w, h); }
    void drawRBox(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r) { u8g2_DrawRBox(x, y, w, h, r); }
    void drawRFrame(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r) { u8g2_DrawRFrame(x, y, w, h, r); }
    void drawLine(int16_t x1, int16_t y1, int16_t x2, int16_t y2) { u8g2_DrawLine(x1, y1, x2, y2); }
    void drawCircle(int16_t x0, int16_t y0, int16_t rad) { u8g2_DrawCircle(x0, y0, rad); }
    void drawXBMP(int16_t x, int16_t y, int16_t w, int16_t h, const uint8_t *bitmap) { u8g2_DrawXBMP(x, y, w, h, bitmap); }
    void drawHLine(int16_t x, int16_t y, int16_t w) { u8g2_DrawHLine(x, y, w); }
    void drawVLine(int16_t x, int16_t y, int16_t h) { u8g2_DrawVLine(x, y, h); }

    void setFont(const uint8_t *font) { u8g2_SetFont(font); }
    void drawStr(int16_t x, int16_t y, const char *str) { u8g2_DrawStr(x, y, str); }
    int16_t drawStrRet(int16_t x, int16_t y, const char *str) { return u8g2_DrawStrRet(x, y, str); }
    void setCursor(int16_t x, int16_t y) { u8g2_SetCursor(x, y); }
    void setFontDirection(uint8_t dir) { u8g2_SetFontDirection(dir); }
    void print(uint32_t num) { u8g2_Print(num); }
    void print(uint8_t val) { u8g2_Print((uint32_t)val); }
    void print(const char *str) { u8g2_PrintStr(str); }
    void print(float val) { u8g2_PrintFloat(val); }
    void print(char c) { char s[2] = {c, 0}; u8g2_PrintStr(s); }
    int16_t getStrWidth(const char *str) { return u8g2_GetStrWidth(str); }

    void setDrawColor(uint8_t color) { u8g2_SetDrawColor(color); }
    void setContrast(uint8_t value) { u8g2_SetContrast(value); }
    void setPowerSave(uint8_t on) { u8g2_SetPowerSave(on); }
    uint8_t* getBufferPtr() { return u8g2_GetBufferPtr(); }
    uint8_t getBufferTileHeight() { return u8g2_GetBufferTileHeight(); }
    uint8_t getBufferTileWidth() { return u8g2_GetBufferTileWidth(); }

    void setBusClock(uint32_t clock) { (void)clock; }
};

extern U8G2 u8g2;

#endif /* __cplusplus */

#endif
