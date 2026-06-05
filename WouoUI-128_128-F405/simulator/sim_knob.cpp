#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>
#include <thread>
#include <cmath>
#include "sim_knob.h"
#include "ui_state.h"

#pragma comment(lib, "winmm.lib")

ButtonState btn;

static uint32_t g_press_start = 0;
static bool g_key_down = false;
static bool g_btn_pending = false;
static uint8_t g_pending_id = 0;
static uint32_t g_long_press_threshold = 500;

// 生成 PCM WAV 并用 PlaySound 播放（阻塞，在独立线程调用）
static void play_beep(int freq, int dur_ms) {
    if (freq <= 0 || dur_ms <= 0) return;
    int sampleRate = 22050;
    int numSamples = (sampleRate * dur_ms) / 1000;
    int dataSize = numSamples;

#pragma pack(push, 1)
    struct {
        char riff[4]; uint32_t chunkSize; char wave[4];
        char fmt[4];  uint32_t fmtSize;   uint16_t audioFormat;
        uint16_t numChannels; uint32_t sampleRate;
        uint32_t byteRate;    uint16_t blockAlign;
        uint16_t bitsPerSample;
        char data[4]; uint32_t dataSize;
    } hdr;
#pragma pack(pop)

    memset(&hdr, 0, sizeof(hdr));
    memcpy(hdr.riff, "RIFF", 4);
    memcpy(hdr.wave, "WAVE", 4);
    memcpy(hdr.fmt,  "fmt ", 4);
    hdr.fmtSize = 16;
    hdr.audioFormat = 1;
    hdr.numChannels = 1;
    hdr.sampleRate = sampleRate;
    hdr.bitsPerSample = 8;
    hdr.byteRate = sampleRate;
    hdr.blockAlign = 1;
    memcpy(hdr.data, "data", 4);
    hdr.dataSize = dataSize;
    hdr.chunkSize = 36 + dataSize;

    int totalSize = sizeof(hdr) + dataSize;
    uint8_t* buf = new uint8_t[totalSize];
    memcpy(buf, &hdr, sizeof(hdr));
    uint8_t* samples = buf + sizeof(hdr);
    for (int i = 0; i < numSamples; ++i) {
        double t = (double)i / sampleRate;
        samples[i] = (uint8_t)(128 + 120 * sin(2 * 3.1415926535 * freq * t));
    }
    PlaySoundA((LPCSTR)buf, NULL, SND_MEMORY | SND_SYNC);
    delete[] buf;
}

// 音量 → 频率映射（对应硬件 ARR→1MHz/(ARR+1)）
static const int k_buz_freq[5] = {0, 833, 1250, 1876, 2500};

void knob_inter() {}

void btn_scan() {
    btn.pressed = false;

    if (g_btn_pending) {
        btn.pressed = true;
        btn.id = g_pending_id;
        g_btn_pending = false;
        // 按键音效与硬件一致：旋转→trig，短按→confirm
        if (btn.id == BTN_ID_CW || btn.id == BTN_ID_CC)
            btn.buzzer_trig = true;
        else if (btn.id == BTN_ID_SP)
            btn.buzzer_confirm = true;
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

void buzzer_proc() {
    int vol = ui.param[BUZ_VOL];
    if (vol <= 0) return;
    if (vol > 4) vol = 4;

    // 防止并发音效重叠（硬件 buzzer_start 在 ADSR 结束后才清零）
    uint32_t now = sim_millis();
    if (now - btn.buzzer_start < 150) return;

    int freq = k_buz_freq[vol];

    if (btn.buzzer_trig) {
        btn.buzzer_trig = false;
        btn.buzzer_start = now;
        std::thread(play_beep, freq, 50).detach();
    }
    if (btn.buzzer_confirm) {
        btn.buzzer_confirm = false;
        btn.buzzer_start = now;
        std::thread(play_beep, freq * 3 / 2, 80).detach();
    }
    if (btn.buzzer_exit) {
        btn.buzzer_exit = false;
        btn.buzzer_start = now;
        int lo = freq * 2 / 3;
        std::thread([lo, now]() {
            play_beep(lo, 80); Sleep(60);
            play_beep(lo, 80);
        }).detach();
    }
    if (btn.buzzer_boot) {
        btn.buzzer_boot = false;
        btn.buzzer_start = now;
        int f1 = freq * 3 / 2;
        int f2 = freq;
        int f3 = freq * 2 / 3;
        std::thread([f1, f2, f3]() {
            play_beep(f1, 110); Sleep(50);
            play_beep(f2, 110); Sleep(50);
            play_beep(f3, 110);
        }).detach();
    }
}

void buzzer_exit_sound() { btn.buzzer_exit = true; }
void buzzer_boot_sound() { btn.buzzer_boot = true; }

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
