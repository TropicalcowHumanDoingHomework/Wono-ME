#include "window.h"
#include "ui_state.h"
#include "animation.h"
#include "display.h"
#include "eeprom_manager.h"
#include "pages.h"
#include "knob.h"



void window_value_init(char title[], uint8_t select, uint8_t *value, uint8_t max, uint8_t min, uint8_t step, Menu *bg, uint8_t index) {
    strcpy(win.title, title);
    win.select = select;
    win.value = value;
    win.max = max;
    win.min = min;
    win.step = step;
    win.bg = bg;
    win.index = index;
    ui.index = M_WINDOW;
    ui.state = S_WINDOW;
}

void window_param_init(void) {
    win.bar = 0;
    win.y = WIN_Y;
    win.y_trg = 50;
    win.l = 13;
    ui.state = S_NONE;
}

void window_show(void) {
    list_show(win.bg, win.index);
    if (ui.param[WIN_BOK]) {
        uint16_t i;
        for (i = 0; i < buf_len; ++i)
            buf_ptr[i] = buf_ptr[i] & (i % 2 == 0 ? 0x55 : 0xAA);
    }

    u8g2_SetFont(WIN_FONT);
    win.bar_trg = (float)(*win.value - win.min) / (float)(win.max - win.min) * (WIN_BAR_W - 4);

    animation(&win.bar, &win.bar_trg, WIN_ANI);
    animation(&win.y, &win.y_trg, WIN_ANI);

    u8g2_SetDrawColor(0);
    u8g2_DrawRBox(win.l, (int16_t)win.y, WIN_W, WIN_H, 2);
    u8g2_SetDrawColor(1);
    u8g2_DrawRFrame(win.l, (int16_t)win.y, WIN_W, WIN_H, 2);
    u8g2_DrawRFrame(win.l + 5, (int16_t)win.y + 20, WIN_BAR_W, WIN_BAR_H, 1);
    u8g2_DrawBox(win.l + 7, (int16_t)win.y + 22, (int16_t)win.bar, WIN_BAR_H - 4);
    u8g2_SetCursor(win.l + 5, (int16_t)win.y + 14);
    u8g2_PrintStr(win.title);
    u8g2_SetCursor(win.l + 78, (int16_t)win.y + 14);
    u8g2_Print((uint32_t)(*win.value));

    if (strcmp(win.title, "Disp Bri") == 0) {
        u8g2_SetContrast(ui.param[DISP_BRI]);
    }

    u8g2_SetDrawColor(2);
    if (!ui.param[DARK_MODE]) {
        u8g2_DrawBox(0, 0, DISP_W, DISP_H);
    }
}

void window_proc(void) {
    window_show();
    if (win.y == WIN_Y_TRG) {
        ui.index = win.index;
    }
    if (btn.pressed && win.y == win.y_trg && win.y != WIN_Y_TRG) {
        btn.pressed = 0;
        switch (btn.id) {
            case BTN_ID_CW:
                if (*win.value < win.max) {
                    *win.value += win.step;
                    eeprom.change = 1;
                }
                break;
            case BTN_ID_CC:
                if (*win.value > win.min) {
                    *win.value -= win.step;
                    eeprom.change = 1;
                }
                break;
            case BTN_ID_SP:
            case BTN_ID_LP:
                win.y_trg = WIN_Y_TRG;
                break;
        }
    }
}
