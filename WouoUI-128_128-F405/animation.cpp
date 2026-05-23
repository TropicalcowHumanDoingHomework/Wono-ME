#include "animation.h"
#include "display.h"
#include "ui_state.h"

/************************************* 动画函数 *************************************/

//动画函数
void animation(float *a, float *a_trg, uint8_t n) {
    if (*a != *a_trg) {
        if (fabs(*a - *a_trg) < 0.15f) {
            *a = *a_trg;
        } else {
            *a += (*a_trg - *a) / (ui.param[n] / 10.0f);
        }
    }
}

//消失函数
void fade() {
    static uint32_t last_fade_time = 0;
    uint32_t now = millis();

    if (now - last_fade_time < ui.param[FADE_ANI]) {
        return;
    }
    last_fade_time = now;

    bool dark = ui.param[DARK_MODE];

    if (ui.param[FADE_MODE] == 0) {
        if (dark) {
            switch (ui.fade) {
                case 1:
                    for (uint16_t y = 0; y < 128; ++y)
                        if (y % 2 == 0)
                            for (uint16_t x = 0; x < 16; ++x)
                                buf_ptr[y * 16 + x] &= 0xAA;
                    break;
                case 2:
                    for (uint16_t y = 0; y < 128; ++y)
                        if (y % 2 == 1)
                            for (uint16_t x = 0; x < 16; ++x)
                                buf_ptr[y * 16 + x] &= 0x55;
                    break;
                case 3:
                    for (uint16_t y = 0; y < 128; ++y)
                        if (y % 2 == 0)
                            for (uint16_t x = 0; x < 16; ++x)
                                buf_ptr[y * 16 + x] &= 0x55;
                    break;
                case 4:
                    for (uint16_t i = 0; i < buf_len; ++i)
                        buf_ptr[i] = 0x00;
                    ui.state = S_NONE;
                    ui.fade = 0;
                    break;
                default:
                    ui.state = S_NONE;
                    ui.fade = 0;
                    break;
            }
        } else {
            switch (ui.fade) {
                case 1:
                    for (uint16_t y = 0; y < 128; ++y)
                        if (y % 2 == 0)
                            for (uint16_t x = 0; x < 16; ++x)
                                buf_ptr[y * 16 + x] |= 0x55;
                    break;
                case 2:
                    for (uint16_t y = 0; y < 128; ++y)
                        if (y % 2 == 1)
                            for (uint16_t x = 0; x < 16; ++x)
                                buf_ptr[y * 16 + x] |= 0xAA;
                    break;
                case 3:
                    for (uint16_t y = 0; y < 128; ++y)
                        if (y % 2 == 0)
                            for (uint16_t x = 0; x < 16; ++x)
                                buf_ptr[y * 16 + x] |= 0xAA;
                    break;
                case 4:
                    for (uint16_t i = 0; i < buf_len; ++i)
                        buf_ptr[i] = 0xFF;
                    ui.state = S_NONE;
                    ui.fade = 0;
                    break;
                default:
                    ui.state = S_NONE;
                    ui.fade = 0;
                    break;
            }
        }
    } else {
        switch (ui.fade) {
            case 1:
                for (uint16_t y = 0; y < 128; y += 2)
                    for (uint16_t x = 0; x < 16; ++x)
                        buf_ptr[y * 16 + x] = 0xAA;
                break;
            case 2:
                for (uint16_t y = 1; y < 128; y += 2)
                    for (uint16_t x = 0; x < 16; ++x)
                        buf_ptr[y * 16 + x] = 0x55;
                break;
            case 3:
                for (uint16_t y = 0; y < 128; y += 2)
                    for (uint16_t x = 0; x < 16; ++x)
                        buf_ptr[y * 16 + x] = 0xFF;
                break;
            case 4:
                for (uint16_t y = 1; y < 128; y += 2)
                    for (uint16_t x = 0; x < 16; ++x)
                        buf_ptr[y * 16 + x] = 0xFF;
                ui.state = S_NONE;
                ui.fade = 0;
                break;
            default:
                ui.state = S_NONE;
                ui.fade = 0;
                break;
        }
    }
    ui.fade++;
}