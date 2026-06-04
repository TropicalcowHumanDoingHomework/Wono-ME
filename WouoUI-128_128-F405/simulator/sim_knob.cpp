#include "sim_knob.h"

ButtonState btn;

static uint32_t g_press_start = 0;
static bool g_key_down = false;
static bool g_btn_pending = false;
static uint8_t g_pending_id = 0;
static uint32_t g_long_press_threshold = 500;

void knob_inter() {}

void btn_scan() {
    btn.pressed = false;

    if (g_btn_pending) {
        btn.pressed = true;
        btn.id = g_pending_id;
        g_btn_pending = false;
        return;
    }

    if (g_key_down) {
        uint32_t now = sim_millis();
        if (now - g_press_start >= g_long_press_threshold) {
            btn.pressed = true;
            btn.id = BTN_ID_LP;
            g_press_start = now;
            g_key_down = false;
        }
    }
}

void btn_init() {
    btn.pressed = false;
    btn.id = 0;
    btn.alv = false;
    btn.blv = false;
    btn.flag = false;
    btn.CW_1 = false;
    btn.CW_2 = false;
    btn.CC_1 = false;
    btn.CC_2 = false;
    btn.pressed_1 = false;
    btn.pressed_2 = false;
    btn.long_pressed = false;
    btn.spt = 25;
    btn.lpt = 150;
    btn.spt_cnt = 0;
    btn.lpt_cnt = 0;
    btn.buzzer_trig = false;
    btn.buzzer_confirm = false;
    btn.buzzer_exit = false;
    btn.buzzer_boot = false;
    btn.buzzer_start = 0;

    g_press_start = 0;
    g_key_down = false;
    g_btn_pending = false;
}

void buzzer_proc() {}
void buzzer_exit_sound() {}
void buzzer_boot_sound() {}

void sim_knob_handle_key(int key, bool down) {
    if (down) {
        switch (key) {
            case 82: // Up arrow (SDL scancode)
            case 26: // W
                g_btn_pending = true;
                g_pending_id = BTN_ID_CW;
                break;
            case 81: // Down arrow (SDL scancode)
            case 22: // S
                g_btn_pending = true;
                g_pending_id = BTN_ID_CC;
                break;
            case 40: // Enter (SDL scancode)
            case 44: // Space (SDL scancode)
                g_key_down = true;
                g_press_start = sim_millis();
                break;
            case 41: // Escape (SDL scancode)
                g_btn_pending = true;
                g_pending_id = BTN_ID_LP;
                break;
        }
    } else {
        if (key == 40 || key == 44) {
            if (g_key_down) {
                uint32_t held = sim_millis() - g_press_start;
                if (held < g_long_press_threshold) {
                    g_btn_pending = true;
                    g_pending_id = BTN_ID_SP;
                }
                g_key_down = false;
            }
        }
    }
}
