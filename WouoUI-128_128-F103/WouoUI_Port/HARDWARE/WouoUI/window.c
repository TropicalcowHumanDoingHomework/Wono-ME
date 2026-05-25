#include "window.h"
#include "ui_state.h"
#include "animation.h"
#include "display.h"
#include "eeprom_manager.h"
#include "pages.h"
#include "knob.h"
#include <string.h>
#include <stdbool.h>

void window_value_init(const char title[], uint8_t select, uint8_t *value, uint8_t max, uint8_t min, uint8_t step, Menu *bg, uint8_t index) {
    strcpy(win.title, title);
    win.select = select;
    win.value = value;
    win.max = max;
    win.min = min;
    win.step = step;
    win.bg = bg;
    win.index = index;
    win.msg_mode = 0;
    ui.index = M_WINDOW;
    window_param_init();
}

void window_message_init(const char title[], const char message[], Menu *bg, uint8_t index) {
    uint8_t max_w;
    uint8_t line_count;
    char msg_copy[128];
    char* line;

    strcpy(win.title, title);
    strcpy(win.message, message);
    win.msg_mode = 1;
    win.bg = bg;
    win.index = index;
    ui.index = M_WINDOW;

    u8g2_SetFont(WIN_FONT);
    max_w = u8g2_GetStrWidth(win.title);
    strcpy(msg_copy, win.message);
    line_count = 0;
    line = strtok(msg_copy, "\n");
    while (line) {
        uint8_t line_w = u8g2_GetStrWidth(line);
        if (line_w > max_w) max_w = line_w;
        line_count++;
        line = strtok(NULL, "\n");
    }

    win.w = win.w_trg = max_w + WIN_MSG_PAD * 2;
    if (win.w_trg < 60) win.w = win.w_trg = 60;
    win.h = win.h_trg = WIN_MSG_PAD * 2 + LIST_TEXT_H + line_count * LIST_LINE_H;

    win.l = (DISP_W - win.w) / 2;
    win.y = -win.h - 2;
    win.y_trg = (DISP_H - win.h) / 2;
    win.u = win.y_trg;
    ui.state = S_NONE;
}

void window_param_init(void) {
    win.msg_mode = 0;
    win.bar = 0;
    win.y = WIN_Y;
    win.y_trg = 50;
    win.u = win.y_trg;
    win.l = 13;
    win.h = WIN_H;
    win.h_trg = WIN_H;
    win.w = WIN_W;
    win.w_trg = WIN_W;
    ui.state = S_NONE;
}

void window_show(void) {
    uint16_t i;
    uint8_t title_w, text_w;
    char msg_copy[128];
    char* line;
    int line_y;

    list_show(win.bg, win.index);

    if (win.msg_mode) {
        animation(&win.y, &win.y_trg, WIN_ANI);

        u8g2_SetDrawColor(0);
        u8g2_DrawRBox(win.l, (int16_t)win.y, (int16_t)win.w, (int16_t)win.h, 2);
        u8g2_SetDrawColor(1);
        u8g2_DrawRFrame(win.l, (int16_t)win.y, (int16_t)win.w, (int16_t)win.h, 2);

        title_w = u8g2_GetStrWidth(win.title);
        u8g2_SetCursor((int16_t)win.l + ((int16_t)win.w - (int16_t)title_w) / 2, (int16_t)win.y + WIN_MSG_PAD + LIST_TEXT_H);
        u8g2_PrintStr(win.title);

        strcpy(msg_copy, win.message);
        line_y = (int16_t)win.y + WIN_MSG_PAD + LIST_TEXT_H + LIST_LINE_H;
        line = strtok(msg_copy, "\n");
        while (line) {
            text_w = u8g2_GetStrWidth(line);
            u8g2_SetCursor((int16_t)win.l + ((int16_t)win.w - (int16_t)text_w) / 2, line_y);
            u8g2_PrintStr(line);
            line_y += LIST_LINE_H;
            line = strtok(NULL, "\n");
        }

        u8g2_SetDrawColor(2);
        if (!ui.param[DARK_MODE]) {
            u8g2_DrawBox(0, 0, DISP_W, DISP_H);
        }
    } else {
        if (ui.param[WIN_BOK]) {
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
}

void window_proc(void) {
    window_show();
    if (win.y == win.y_trg && win.y_trg < 0) {
        ui.index = win.index;
        ui.state = S_NONE;
    }
    if (btn.pressed && win.y == win.y_trg && win.y_trg > 0) {
        btn.pressed = false;
        if (win.msg_mode) {
            win.y_trg = -win.h - 2;
        } else {
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
}
