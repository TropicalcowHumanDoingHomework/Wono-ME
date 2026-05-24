#include "animation.h"
#include "display.h"
#include "ui_state.h"
#include "pages.h"
#include "menu_data.h"
#include "u8g2_adapter.h"
#include "delay.h"

void animation(float *a, float *a_trg, uint8_t n) {
    if (*a != *a_trg) {
        if (fabs(*a - *a_trg) < 0.15f) {
            *a = *a_trg;
        } else {
            *a += (*a_trg - *a) / (ui.param[n] / 10.0f);
        }
    }
}

static uint8_t fade_timer_check(void) {
    static uint32_t last_fade_time = 0;
    uint32_t now = millis();
    if (now - last_fade_time < ui.param[FADE_ANI]) return 0;
    last_fade_time = now;
    return 1;
}

void fade(void) {
    uint16_t i;
    if (!fade_timer_check()) return;

    if (ui.param[DARK_MODE]) {
        switch (ui.fade) {
            case 1:
                for (i = 0; i < buf_len; ++i)
                    if (i % 2 != 0) buf_ptr[i] &= 0xAA;
                break;
            case 2:
                for (i = 0; i < buf_len; ++i)
                    if (i % 2 != 0) buf_ptr[i] &= 0x00;
                break;
            case 3:
                for (i = 0; i < buf_len; ++i)
                    if (i % 2 == 0) buf_ptr[i] &= 0x55;
                break;
            case 4:
                for (i = 0; i < buf_len; ++i)
                    if (i % 2 == 0) buf_ptr[i] &= 0x00;
                break;
            default:
                ui.state = S_NONE;
                ui.fade = 0;
                break;
        }
    } else {
        switch (ui.fade) {
            case 1:
                for (i = 0; i < buf_len; ++i)
                    if (i % 2 != 0) buf_ptr[i] |= 0xAA;
                break;
            case 2:
                for (i = 0; i < buf_len; ++i)
                    if (i % 2 != 0) buf_ptr[i] |= 0x00;
                break;
            case 3:
                for (i = 0; i < buf_len; ++i)
                    if (i % 2 == 0) buf_ptr[i] |= 0x55;
                break;
            case 4:
                for (i = 0; i < buf_len; ++i)
                    if (i % 2 == 0) buf_ptr[i] |= 0x00;
                break;
            default:
                ui.state = S_NONE;
                ui.fade = 0;
                break;
        }
    }
    ui.fade++;
}

void fade_sleep(void) {
    uint16_t i;
    if (!fade_timer_check()) return;

    switch (ui.fade) {
        case 1:
            for (i = 0; i < buf_len; ++i)
                if (i % 2 != 0) buf_ptr[i] &= 0xAA;
            break;
        case 2:
            for (i = 0; i < buf_len; ++i)
                if (i % 2 != 0) buf_ptr[i] &= 0x00;
            break;
        case 3:
            for (i = 0; i < buf_len; ++i)
                if (i % 2 == 0) buf_ptr[i] &= 0x55;
            break;
        case 4:
            for (i = 0; i < buf_len; ++i)
                if (i % 2 == 0) buf_ptr[i] &= 0x00;
            break;
        default:
            u8g2_SetPowerSave(1);
            ui.state = S_NONE;
            ui.fade = 0;
            break;
    }
    ui.fade++;
}

void fade_wake(void) {
    uint16_t i;
    if (!fade_timer_check()) return;

    /* redraw current page before fade mask (matching Arduino) */
    u8g2_ClearBuffer();
    switch (ui.index) {
        case M_WINDOW: window_show(); break;
        case M_MAIN:   tile_show(main_menu, main_menu_exp, main_icon_pic); break;
        case M_EDITOR: list_show(editor_menu, M_EDITOR); break;
        case M_KNOB:   list_show(knob_menu, M_KNOB); break;
        case M_KRF:    list_show(krf_menu, M_KRF); break;
        case M_KPF:    list_show(kpf_menu, M_KPF); break;
        case M_SETTING:list_show(setting_menu, M_SETTING); break;
        case M_VOLT:   volt_show(); break;
        case M_ABOUT:  about_show(); break;
        default: break;
    }

    switch (ui.fade) {
        case 1:
            for (i = 0; i < buf_len; ++i)
                if (i % 2 == 0) buf_ptr[i] = 0;
                else buf_ptr[i] &= 0x55;
            break;
        case 2:
            for (i = 0; i < buf_len; ++i)
                if (i % 2 == 0) buf_ptr[i] &= 0x55;
            break;
        case 3:
        case 4:
            break;
        default:
            ui.wake_fade = 0;
            ui.state = S_NONE;
            ui.fade = 0;
            break;
    }
    ui.fade++;
}
