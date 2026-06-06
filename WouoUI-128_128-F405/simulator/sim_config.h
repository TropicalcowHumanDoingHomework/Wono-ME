#ifndef SIM_CONFIG_H
#define SIM_CONFIG_H

// ============================================================
// 注意：此头文件通过 /FI 强制包含在所有源文件之前。
// 不得包含 C++ 标准库头文件（如 <cstdint>），因为
// force-include 发生在 <windows.h> 之前，可能引起冲突。
// 使用手动 typedef 替代 <stdint.h>/<cstdint>。
// ============================================================

// Manual fixed-width integer types (no stdint.h dependency)
typedef signed char        int8_t;
typedef unsigned char      uint8_t;
typedef short              int16_t;
typedef unsigned short     uint16_t;
typedef int                int32_t;
typedef unsigned int       uint32_t;
typedef long long          int64_t;
typedef unsigned long long uint64_t;

// Arduino-compatible types
typedef unsigned char byte;
typedef bool boolean;

// Config macros from original config.h (needed by UI source files)
// MCU info for About page
#define MCU_BOARD  "STM32F405"
#define MCU_RAM    "192k"
#define MCU_FLASH  "1024k"
#define MCU_FREQ   "168Mhz"
// PROGMEM: on AVR this places data in flash; on PC simulator, no-op
#define PROGMEM
// HID: enable USB HID functionality
#define HID_ENABLE 1

// USB HID Keyboard keycodes (from original config.h)
#ifndef KEY_ESC
#define KEY_ESC          41
#define KEY_F1           58
#define KEY_F2           59
#define KEY_F3           60
#define KEY_F4           61
#define KEY_F5           62
#define KEY_F6           63
#define KEY_F7           64
#define KEY_F8           65
#define KEY_F9           66
#define KEY_F10          67
#define KEY_F11          68
#define KEY_F12          69
#define KEY_LEFT_CTRL    0xE0
#define KEY_LEFT_SHIFT   0xE1
#define KEY_LEFT_ALT     0xE2
#define KEY_LEFT_GUI     0xE3
#define KEY_RIGHT_CTRL   0xE4
#define KEY_RIGHT_SHIFT  0xE5
#define KEY_RIGHT_ALT    0xE6
#define KEY_RIGHT_GUI    0xE7
#define KEY_CAPS_LOCK    57
#define KEY_BACKSPACE    42
#define KEY_RETURN       40
#define KEY_INSERT       73
#define KEY_DELETE       76
#define KEY_TAB          43
#define KEY_HOME         74
#define KEY_END          77
#define KEY_PAGE_UP      75
#define KEY_PAGE_DOWN    78
#define KEY_UP_ARROW     82
#define KEY_DOWN_ARROW   81
#define KEY_LEFT_ARROW   80
#define KEY_RIGHT_ARROW  79
#endif

// Display dimensions
#define DISP_H 128
#define DISP_W 128

// UI config
#define UI_DEPTH 20
#define UI_MNUMB 20
#define UI_PARAM 31

// Font declarations (defined in sim_display.cpp)
extern const unsigned char u8g2_font_helvB24_tr[];
extern const unsigned char u8g2_font_HelvetiPixel_tr[];

// Tile layout constants
#define TILE_B_FONT u8g2_font_helvB24_tr
#define TILE_S_FONT u8g2_font_HelvetiPixel_tr
#define TILE_B_TITLE_H 25
#define TILE_S_TITLE_H 8
#define TILE_ICON_H 48
#define TILE_ICON_W 48
#define TILE_ICON_S 57
#define TILE_INDI_H 40
#define TILE_INDI_W 10
#define TILE_INDI_S 57

// List layout constants
#define LIST_FONT u8g2_font_HelvetiPixel_tr
#define LIST_TEXT_H 8
#define LIST_LINE_H 16
#define LIST_TEXT_S 4
#define LIST_BAR_W 5
#define LIST_BOX_R 0.5f

// Voltage measurement layout
#define WAVE_SAMPLE 20
#define WAVE_W DISP_W
#define WAVE_L 0
#define WAVE_U 0
#define WAVE_MAX 43
#define WAVE_MIN 5
#define WAVE_BOX_H 49
#define WAVE_BOX_W DISP_W
#define VOLT_FONT u8g2_font_helvB24_tr
#define VOLT_LIST_U_S 94
#define VOLT_TEXT_BG_U_S 53
#define VOLT_TEXT_BG_H 33

// Checkbox layout
#define CHECK_BOX_L_S 95
#define CHECK_BOX_U_S 2
#define CHECK_BOX_F_W 12
#define CHECK_BOX_F_H 12
#define CHECK_BOX_D_S 2

// Window layout
#define WIN_FONT u8g2_font_HelvetiPixel_tr
#define WIN_H 32
#define WIN_W 102
#define WIN_BAR_W 92
#define WIN_BAR_H 7
#define WIN_Y (-WIN_H - 2)
#define WIN_Y_TRG (-WIN_H - 2)
#define WIN_MSG_PAD 6
#define WIN_LIST_MAX 20
#define WIN_LIST_ITEM_LEN 24

// About page layout
#define ABOUT_FONT u8g2_font_HelvetiPixel_tr
#define ABOUT_INDI_S 4
#define ABOUT_INDI_W 2

// Page index enum
enum PageIndex {
    M_WINDOW, M_SLEEP, M_MAIN, M_ANIMITION,
    M_EDITOR, M_KNOB, M_KRF, M_KPF,
    M_VOLT, M_USB, M_HID_KEY, M_SETTING, M_ABOUT,
};

// Page state enum
enum PageState {
    S_FADE, S_WINDOW, S_LAYER_IN, S_LAYER_OUT, S_NONE
};

// Parameter index enum
enum ParamIndex {
    DISP_BRI, TILE_ANI, LIST_CUR,
    BOX_X_OS, BOX_Y_OS, WIN_Y_OS,
    LIST_ANI, WIN_ANI, SPOT_ANI,
    TAG_ANI, FADE_ANI, BTN_SPT, BTN_LPT,
    TILE_UFD, LIST_UFD, TILE_LOOP, LIST_LOOP,
    WIN_BOK, KNOB_DIR, DARK_MODE, ROTATE_SCR,
    BUZ_VOL, USB_ENABLE, USB_WP, HID_ENABLE_SW,
    WIN_STYLE, FADE_MODE, HL_ANI_MODE,
    SPRING_K, SPRING_D, CDC_ENABLE_SW
};

// Knob config
#define KNOB_PARAM 4
#define KNOB_DISABLE 0
#define KNOB_ROT_VOL 1
#define KNOB_ROT_BRI 2
#define BTN_PARAM_TIMES 2

enum ButtonId { BTN_ID_CC, BTN_ID_CW, BTN_ID_SP, BTN_ID_LP };
enum KnobParamIndex { KNOB_ROT, KNOB_COD, KNOB_ROT_P, KNOB_COD_P, KNOB_CASE };

#define EEPROM_CHECK 11

// Arduino compat macros
#define millis() sim_millis()
#define delay(ms) sim_delay(ms)
// Note: Do NOT define INPUT, OUTPUT, LOW, HIGH here as they conflict
// with Windows SDK types (e.g., INPUT is a typedef in winuser.h).
// These pin-mode constants are not needed by the UI logic source files.

extern unsigned int sim_millis();
extern void sim_delay(unsigned int ms);

// ============================================================
// Arduino/STM32 GPIO pin defines (for pages.cpp analog_pin[])
// These are just symbolic constants, not actual GPIO pins.
// ============================================================
#define PA0  0
#define PA1  1
#define PA2  2
#define PA3  3
#define PA4  4
#define PA5  5
#define PA6  6
#define PA7  7
#define PB0  8
#define PB1  9

// ============================================================
// Arduino function stubs (for pages.cpp)
// ============================================================
static inline int analogRead(int pin) { return 0; }
static inline long map(long x, long in_min, long in_max, long out_min, long out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

// ============================================================
// Display rotation stubs (for pages.cpp rot_table[])
// u8g2_cb_t is an opaque struct type (only used as pointer)
// ============================================================
typedef struct u8g2_cb_t u8g2_cb_t;
#define U8G2_R0 ((const u8g2_cb_t*)0)
#define U8G2_R1 ((const u8g2_cb_t*)1)
#define U8G2_R2 ((const u8g2_cb_t*)2)
#define U8G2_R3 ((const u8g2_cb_t*)3)

// ============================================================
// SimU8g2 display class
// ============================================================
class U8G2_LS013B7DH03_128X128_F_4W_SW_SPI {
private:
    unsigned char buffer[2048];
    int drawColor;
    int glyph_w, glyph_h;
    int glyph_baseline;
    bool is_big_font;
    int font_direction;
    int cursor_x, cursor_y;
    uint8_t rotation;
    struct { int x1, y1, x2, y2; } clip;

    void set_pixel(int x, int y);
    void clear_pixel(int x, int y);
    void xor_pixel(int x, int y);
    void hline(int x, int y, int w);
    void vline(int x, int y, int h);

public:
    U8G2_LS013B7DH03_128X128_F_4W_SW_SPI();

    void drawBox(int x, int y, int w, int h);
    void drawFrame(int x, int y, int w, int h);
    void drawRBox(int x, int y, int w, int h, float r);
    void drawRFrame(int x, int y, int w, int h, float r);
    void drawHLine(int x, int y, int w);
    void drawVLine(int x, int y, int h);
    void drawLine(int x1, int y1, int x2, int y2);
    void drawStr(int x, int y, const char* s);
    void drawXBMP(int x, int y, int w, int h, const unsigned char* bitmap);
    void drawPixel(int x, int y);

    void setDrawColor(int c);
    void setFont(const unsigned char* f);
    void setFontDirection(int d);
    void setCursor(int x, int y);
    void setContrast(int val);
    void setPowerSave(int mode) {}
    void setDisplayRotation(const u8g2_cb_t* rot);
    int  getRotation() const { return rotation; }

    void clearBuffer();
    void sendBuffer();
    unsigned char* getBufferPtr() { return buffer; }
    int getBufferTileHeight() { return 8; }
    int getBufferTileWidth() { return 16; }
    int getStrWidth(const char* s);

    void print(const char* s);
    void print(int val);
    void print(float val);
    void print(char c) { char buf[2] = {c, 0}; print(buf); }
    void println(const char* s) { print(s); }
    void println(int val) { print(val); }
    void println(float val) { print(val); }

    void begin();
    void setClipWindow(int x1, int y1, int x2, int y2);
    void setMaxClipWindow();
};

// Global display objects
extern U8G2_LS013B7DH03_128X128_F_4W_SW_SPI u8g2;
extern unsigned char* buf_ptr;
extern uint16_t buf_len;

// Display init
void lcd_init();
void lcd_reset_vcom();

// Simulator global state
extern bool sim_running;

#endif
