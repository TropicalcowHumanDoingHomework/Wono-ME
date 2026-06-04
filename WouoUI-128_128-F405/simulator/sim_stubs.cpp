#include "sim_config.h"
#include "ui_types.h"
#include "hid_manager.h"

// ============================================================
// LED stubs
// ============================================================
void led_init() {}
void led_proc() {}
void led_set_red() {}
void led_set_green() {}
void led_set_yellow() {}
void led_set_white() {}
void led_off() {}
void led_full_off() {}
void led_set_color(uint8_t, uint8_t, uint8_t) {}
void led_start_breathing_white() {}

// ============================================================
// USB Debug stubs
// ============================================================
void usb_debug_reset() {}
void usb_debug_refresh() {}
void usb_debug_set_msg(const char*) {}
void usb_debug_set_step(int, int) {}

// ============================================================
// HID stubs
// ============================================================
void hid_init() {}

HIDConsumer Consumer;
void HIDConsumer::press(uint16_t usage) { (void)usage; }
void HIDConsumer::release() {}

HIDKeyboard Keyboard;
HIDKeyboard::HIDKeyboard() : _modifier(0) { memset(_keys, 0, sizeof(_keys)); }
void HIDKeyboard::press(uint8_t keycode) { (void)keycode; }
void HIDKeyboard::release(uint8_t keycode) { (void)keycode; }
void HIDKeyboard::releaseAll() {}
void HIDKeyboard::_send_report() {}

// ============================================================
// Time functions
// ============================================================
#include <chrono>
static auto g_start_time = std::chrono::steady_clock::now();

uint32_t sim_millis() {
    auto now = std::chrono::steady_clock::now();
    return (uint32_t)std::chrono::duration_cast<std::chrono::milliseconds>(now - g_start_time).count();
}

void sim_delay(uint32_t ms) {
    uint32_t target = sim_millis() + ms;
    while (sim_millis() < target) {}
}
