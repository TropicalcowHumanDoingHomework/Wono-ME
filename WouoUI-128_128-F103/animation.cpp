#include "animation.h"
#include "display.h"
#include "ui_state.h"

/************************************* 动画函数 *************************************/

void animation(float *a, float *a_trg, uint8_t n) {
    if (*a != *a_trg) {
        if (fabs(*a - *a_trg) < 0.15f) {
            *a = *a_trg;
        } else {
            *a += (*a_trg - *a) / (ui.param[n] / 10.0f);
        }
    }
}

static bool fade_timer_check() {
    static uint32_t last_fade_time = 0;
    uint32_t now = millis();
    if (now - last_fade_time < ui.param[FADE_ANI]) return false;
    last_fade_time = now;
    return true;
}

void fade() {
    if (!fade_timer_check()) return;

    if (ui.param[DARK_MODE]) {
        switch (ui.fade) {
            case 1:
                for (uint16_t i = 0; i < buf_len; ++i)
                    if (i % 2 != 0) buf_ptr[i] &= 0xAA;
                break;
            case 2:
                for (uint16_t i = 0; i < buf_len; ++i)
                    if (i % 2 != 0) buf_ptr[i] &= 0x00;
                break;
            case 3:
                for (uint16_t i = 0; i < buf_len; ++i)
                    if (i % 2 == 0) buf_ptr[i] &= 0x55;
                break;
            case 4:
                for (uint16_t i = 0; i < buf_len; ++i)
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
                for (uint16_t i = 0; i < buf_len; ++i)
                    if (i % 2 != 0) buf_ptr[i] |= 0xAA;
                break;
            case 2:
                for (uint16_t i = 0; i < buf_len; ++i)
                    if (i % 2 != 0) buf_ptr[i] |= 0x00;
                break;
            case 3:
                for (uint16_t i = 0; i < buf_len; ++i)
                    if (i % 2 == 0) buf_ptr[i] |= 0x55;
                break;
            case 4:
                for (uint16_t i = 0; i < buf_len; ++i)
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

void fade_sleep() {
    if (!fade_timer_check()) return;

    switch (ui.fade) {
        case 1:
            for (uint16_t i = 0; i < buf_len; ++i)
                if (i % 2 != 0) buf_ptr[i] &= 0xAA;
            break;
        case 2:
            for (uint16_t i = 0; i < buf_len; ++i)
                if (i % 2 != 0) buf_ptr[i] &= 0x00;
            break;
        case 3:
            for (uint16_t i = 0; i < buf_len; ++i)
                if (i % 2 == 0) buf_ptr[i] &= 0x55;
            break;
        case 4:
            for (uint16_t i = 0; i < buf_len; ++i)
                if (i % 2 == 0) buf_ptr[i] &= 0x00;
            break;
        default:
            u8g2.setPowerSave(1);
            ui.state = S_NONE;
            ui.fade = 0;
            break;
    }
    ui.fade++;
}
