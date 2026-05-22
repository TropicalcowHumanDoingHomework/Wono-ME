#include "animation.h"
#include "display.h"
#include "ui_state.h"
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

void fade() {
    uint16_t i;
    delay_ms(ui.param[FADE_ANI]);
    if (ui.param[DARK_MODE]) {
        switch (ui.fade) {
            case 1:
                for (i = 0; i < buf_len; ++i)
                    if (i % 2 != 0) buf_ptr[i] = buf_ptr[i] & 0xAA;
                break;
            case 2:
                for (i = 0; i < buf_len; ++i)
                    if (i % 2 != 0) buf_ptr[i] = buf_ptr[i] & 0x00;
                break;
            case 3:
                for (i = 0; i < buf_len; ++i)
                    if (i % 2 == 0) buf_ptr[i] = buf_ptr[i] & 0x55;
                break;
            case 4:
                for (i = 0; i < buf_len; ++i)
                    if (i % 2 == 0) buf_ptr[i] = buf_ptr[i] & 0x00;
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
                    if (i % 2 != 0) buf_ptr[i] = buf_ptr[i] | 0xAA;
                break;
            case 2:
                for (i = 0; i < buf_len; ++i)
                    if (i % 2 != 0) buf_ptr[i] = buf_ptr[i] | 0x00;
                break;
            case 3:
                for (i = 0; i < buf_len; ++i)
                    if (i % 2 == 0) buf_ptr[i] = buf_ptr[i] | 0x55;
                break;
            case 4:
                for (i = 0; i < buf_len; ++i)
                    if (i % 2 == 0) buf_ptr[i] = buf_ptr[i] | 0x00;
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
    uint16_t i;
    delay_ms(ui.param[FADE_ANI]);
    switch (ui.fade) {
        case 1:
            for (i = 0; i < buf_len; ++i)
                if (i % 2 != 0) buf_ptr[i] = buf_ptr[i] & 0xAA;
            break;
        case 2:
            for (i = 0; i < buf_len; ++i)
                if (i % 2 != 0) buf_ptr[i] = buf_ptr[i] & 0x00;
            break;
        case 3:
            for (i = 0; i < buf_len; ++i)
                if (i % 2 == 0) buf_ptr[i] = buf_ptr[i] & 0x55;
            break;
        case 4:
            for (i = 0; i < buf_len; ++i)
                if (i % 2 == 0) buf_ptr[i] = buf_ptr[i] & 0x00;
            break;
        default:
            ui.state = S_NONE;
            ui.fade = 0;
            break;
    }
    ui.fade++;
}

void fade_wake() {
    uint16_t i;
    delay_ms(ui.param[FADE_ANI]);
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
