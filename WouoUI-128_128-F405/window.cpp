#include "window.h"
#include "ui_state.h"
#include "animation.h"
#include "display.h"
#include "eeprom_manager.h"
#include "pages.h"
#include "knob.h"

/************************************* 弹窗相关 *************************************/

void window_value_init(const char title[], uint8_t select, uint8_t *value, uint8_t max, uint8_t min, uint8_t step, Menu *bg, uint8_t index) {
    strcpy(win.title, title);
    win.select = select;
    win.value = value;
    win.max = max;
    win.min = min;
    win.step = step;
    win.bg = bg;
    win.index = index;
    ui.index = M_WINDOW;
    window_param_init();

    if (ui.param[WIN_STYLE]) {
        win.box_H = WIN_H;
        win.box_h = 0;
        win.box_h_trg = WIN_H + ui.param[WIN_Y_OS];
        win.w = DISP_W;
        win.w_trg = WIN_W;
    }
}

void window_message_init(const char title[], const char message[], Menu *bg, uint8_t index) {
    strcpy(win.title, title);
    strcpy(win.message, message);
    win.msg_mode = 1;
    win.bg = bg;
    win.index = index;
    ui.index = M_WINDOW;

    u8g2.setFont(WIN_FONT);
    uint8_t max_w = u8g2.getStrWidth(win.title);
    char msg_copy[128];
    strcpy(msg_copy, win.message);
    char* line = strtok(msg_copy, "\n");
    uint8_t line_count = 0;
    while (line) {
        uint8_t line_w = u8g2.getStrWidth(line);
        if (line_w > max_w) max_w = line_w;
        line_count++;
        line = strtok(NULL, "\n");
    }

    win.w = win.w_trg = max_w + WIN_MSG_PAD * 2;
    if (win.w_trg < 60) win.w = win.w_trg = 60;
    win.h = win.h_trg = WIN_MSG_PAD * 2 + LIST_TEXT_H + line_count * LIST_LINE_H;

    if (ui.param[WIN_STYLE]) {
        win.box_H = win.h;
        win.box_h = 0;
        win.box_h_trg = win.h + ui.param[WIN_Y_OS];
        win.w = DISP_W;
    }

    win.l = (DISP_W - win.w) / 2;
    win.y = -win.h - 2;
    win.y_trg = (DISP_H - win.h) / 2;
    win.u = win.y_trg;
    ui.state = S_NONE;
}

void window_param_init() {
    win.msg_mode = 0;
    win.bokeh_step = 0;
    win.last_bokeh_time = 0;
    win.bar = 0;
    win.y = WIN_Y;
    win.y_trg = 50;
    win.u = win.y_trg;
    win.l = 13;

    if (ui.param[WIN_STYLE]) {
        win.box_H = WIN_H;
        win.box_h = 0;
        win.box_h_trg = WIN_H + ui.param[WIN_Y_OS];
        win.w = DISP_W;
        win.w_trg = WIN_W;
        win.l = (DISP_W - win.w) / 2;
    }

    ui.state = S_NONE;
}

void window_show() {
    list_show(win.bg, win.index);

    if (ui.param[WIN_BOK]) {
        float start_y = WIN_Y;
        float target_y = win.y_trg > start_y ? win.y_trg : win.u;
        float range = target_y - start_y;
        float entry_progress = range > 0 ? (win.y - start_y) / range : 1.0f;
        if (entry_progress < 0) entry_progress = 0;
        if (entry_progress > 1) entry_progress = 1;

        uint32_t now = millis();
        if (now - win.last_bokeh_time >= ui.param[FADE_ANI]) {
            uint8_t target = 0;
            if (entry_progress >= 0.75f) target = 3;
            else if (entry_progress >= 0.50f) target = 2;
            else if (entry_progress >= 0.25f) target = 1;
            if (target > win.bokeh_step) win.bokeh_step++;
            else if (target < win.bokeh_step) win.bokeh_step--;
            win.last_bokeh_time = now;
        }

        if (win.bokeh_step) {
            if (ui.param[FADE_MODE] == 0) {
                if (ui.param[DARK_MODE]) {
                    if (win.bokeh_step >= 3) {
                        for (uint16_t y = 0; y < 128; ++y)
                            for (uint16_t x = 0; x < 16; ++x)
                                if (y % 2 == 0) buf_ptr[y * 16 + x] &= 0x55;
                                else buf_ptr[y * 16 + x] &= 0xAA;
                    } else if (win.bokeh_step >= 2) {
                        for (uint16_t y = 0; y < 128; ++y)
                            for (uint16_t x = 0; x < 16; ++x)
                                if (y % 2 == 0) buf_ptr[y * 16 + x] &= 0xAA;
                                else buf_ptr[y * 16 + x] &= 0x55;
                    } else {
                        for (uint16_t y = 0; y < 128; y += 2)
                            for (uint16_t x = 0; x < 16; ++x)
                                buf_ptr[y * 16 + x] &= 0x55;
                    }
                } else {
                    if (win.bokeh_step >= 3) {
                        for (uint16_t y = 0; y < 128; ++y)
                            for (uint16_t x = 0; x < 16; ++x)
                                if (y % 2 == 0) buf_ptr[y * 16 + x] |= 0xAA;
                                else buf_ptr[y * 16 + x] |= 0x55;
                    } else if (win.bokeh_step >= 2) {
                        for (uint16_t y = 0; y < 128; ++y)
                            for (uint16_t x = 0; x < 16; ++x)
                                if (y % 2 == 0) buf_ptr[y * 16 + x] |= 0x55;
                                else buf_ptr[y * 16 + x] |= 0xAA;
                    } else {
                        for (uint16_t y = 0; y < 128; y += 2)
                            for (uint16_t x = 0; x < 16; ++x)
                                buf_ptr[y * 16 + x] |= 0x55;
                    }
                }
            } else {
                if (win.bokeh_step >= 2) {
                    for (uint16_t y = 1; y < 128; y += 2)
                        for (uint16_t x = 0; x < 16; ++x)
                            buf_ptr[y * 16 + x] = 0x55;
                }
                for (uint16_t y = 0; y < 128; y += 2)
                    for (uint16_t x = 0; x < 16; ++x)
                        buf_ptr[y * 16 + x] = 0xAA;
            }
        }
    }

    uint8_t bg_color = ui.param[DARK_MODE] ? 0 : 1;
    uint8_t fg_color = ui.param[DARK_MODE] ? 1 : 0;

    u8g2.setFont(WIN_FONT);

    if (win.msg_mode) {
        animation(&win.y, &win.y_trg, WIN_ANI);

        if (ui.param[WIN_STYLE]) {
            animation(&win.box_h, &win.box_h_trg, WIN_ANI);
            animation(&win.box_h_trg, &win.box_H, WIN_ANI);
            animation(&win.w, &win.w_trg, WIN_ANI);
            win.l = (DISP_W - win.w) / 2;

            if (win.box_h > 2) {
                u8g2.setDrawColor(bg_color);
                u8g2.drawBox((int16_t)win.l + 1, (int16_t)win.y + 1, (int16_t)win.w - 2, (int16_t)win.box_h - 2);
                u8g2.setDrawColor(fg_color);
                u8g2.drawRFrame((int16_t)win.l, (int16_t)win.y, (int16_t)win.w, (int16_t)win.box_h, 2);
            }
        } else {
            u8g2.setDrawColor(bg_color);
            u8g2.drawBox((int16_t)win.l + 1, (int16_t)win.y + 1, (int16_t)win.w - 2, (int16_t)win.h - 2);
            u8g2.setDrawColor(fg_color);
            u8g2.drawRFrame((int16_t)win.l, (int16_t)win.y, (int16_t)win.w, (int16_t)win.h, 2);
        }

        if (!ui.param[WIN_STYLE] || win.box_h > WIN_MSG_PAD + LIST_TEXT_H) {
            uint8_t title_w = u8g2.getStrWidth(win.title);
            u8g2.setCursor((int16_t)win.l + ((int16_t)win.w - title_w) / 2, (int16_t)win.y + WIN_MSG_PAD + LIST_TEXT_H);
            u8g2.print(win.title);

            char msg_copy[128];
            strcpy(msg_copy, win.message);
            char* line = strtok(msg_copy, "\n");
            int line_y = (int16_t)win.y + WIN_MSG_PAD + LIST_TEXT_H + LIST_LINE_H;
            while (line) {
                uint8_t text_w = u8g2.getStrWidth(line);
                u8g2.setCursor((int16_t)win.l + ((int16_t)win.w - text_w) / 2, line_y);
                u8g2.print(line);
                line_y += LIST_LINE_H;
                line = strtok(NULL, "\n");
            }
        }
    } else {
        win.bar_trg = (float)(*win.value - win.min) / (float)(win.max - win.min) * (WIN_BAR_W - 4);

        animation(&win.bar, &win.bar_trg, WIN_ANI);
        animation(&win.y, &win.y_trg, WIN_ANI);

        if (ui.param[WIN_STYLE]) {
            animation(&win.box_h, &win.box_h_trg, WIN_ANI);
            animation(&win.box_h_trg, &win.box_H, WIN_ANI);
            animation(&win.w, &win.w_trg, WIN_ANI);
            win.l = (DISP_W - win.w) / 2;

            if (win.box_h > 2) {
                u8g2.setDrawColor(bg_color);
                u8g2.drawRBox(win.l, (int16_t)win.y, win.w, win.box_h, 2);
                u8g2.setDrawColor(fg_color);
                u8g2.drawRFrame(win.l, (int16_t)win.y, win.w, win.box_h, 2);
            }
        } else {
            u8g2.setDrawColor(bg_color);
            u8g2.drawRBox(win.l, (int16_t)win.y, WIN_W, WIN_H, 2);
            u8g2.setDrawColor(fg_color);
            u8g2.drawRFrame(win.l, (int16_t)win.y, WIN_W, WIN_H, 2);
        }

        if (!ui.param[WIN_STYLE] || win.box_h > 16) {
            u8g2.drawRFrame(win.l + 5, (int16_t)win.y + 20, WIN_BAR_W, WIN_BAR_H, 1);
            u8g2.drawBox(win.l + 7, (int16_t)win.y + 22, win.bar, WIN_BAR_H - 4);
            u8g2.setCursor(win.l + 5, (int16_t)win.y + 14);
            u8g2.print(win.title);
            u8g2.setCursor(win.l + 78, (int16_t)win.y + 14);
            u8g2.print(*win.value);
        }

        if (!strcmp(win.title, "Disp Bri")) {
            u8g2.setContrast(ui.param[DISP_BRI]);
        }
    }
}

void window_proc() {
    window_show();
    if (win.y == win.y_trg && win.y_trg < 0) {
        if (!ui.param[WIN_STYLE] || win.box_h == 0) {
            ui.index = win.index;
            ui.state = S_NONE;
        }
    }
    if (btn.pressed && win.y == win.y_trg && win.y_trg > 0) {
        btn.pressed = false;
        if (win.msg_mode) {
            win.y_trg = -win.h - 2;
            if (ui.param[WIN_STYLE]) {
                win.box_H = 0;
                win.box_h_trg = 0;
                win.w_trg = DISP_W;
            }
            buzzer_exit_sound();
        } else {
            switch (btn.id) {
                case BTN_ID_CW:
                    if (*win.value < win.max) {
                        *win.value += win.step;
                        eeprom.change = true;
                    }
                    break;
                case BTN_ID_CC:
                    if (*win.value > win.min) {
                        *win.value -= win.step;
                        eeprom.change = true;
                    }
                    break;
                case BTN_ID_SP:
                case BTN_ID_LP:
                    win.y_trg = WIN_Y_TRG;
                    if (ui.param[WIN_STYLE]) {
                        win.box_H = 0;
                        win.box_h_trg = 0;
                        win.w_trg = DISP_W;
                    }
                    buzzer_exit_sound();
                    break;
            }
        }
    }
}
