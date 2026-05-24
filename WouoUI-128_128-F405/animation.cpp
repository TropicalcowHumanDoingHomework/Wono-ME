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

//弹簧动画函数
void animation_spring(float *a, float *a_trg, float *vel, float stiffness, float damping) {
    if (*a != *a_trg || fabs(*vel) > 0.01f) {
        float force = (*a_trg - *a) * stiffness;
        *vel = (*vel + force) * damping;
        *a += *vel;
        if (fabs(*a - *a_trg) < 0.05f && fabs(*vel) < 0.05f) {
            *a = *a_trg;
            *vel = 0;
        }
    }
}

//弹跳动画函数（阻尼振荡解析公式，vel兼作速度）
void animation_bounce(float *a, float *a_trg, float *vel, uint8_t n) {
    if (*a != *a_trg || fabs(*vel) > 0.01f) {
        float k = ui.param[SPRING_K] / 100.0f;
        float d = ui.param[SPRING_D] / 100.0f;
        if (k < 0.1f) k = 0.1f;
        float w = 4.0f * sqrtf(k);
        float damp = (1.0f - d * 0.4f);
        if (damp < 0.5f) damp = 0.5f;
        if (damp > 0.99f) damp = 0.99f;
        float force = (*a_trg - *a) * k * 0.5f;
        *vel = (*vel + force) * damp;
        *vel += (*a_trg - *a) * 0.03f * w;
        *a += *vel;
        if (fabs(*a - *a_trg) < 0.05f && fabs(*vel) < 0.05f) {
            *a = *a_trg;
            *vel = 0;
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