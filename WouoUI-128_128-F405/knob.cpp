#include "knob.h"
#include "ui_state.h"

/************************************* 嗡鸣器裸寄存器 *************************************/

#define REG32(addr) (*(volatile uint32_t *)(addr))

#define RCC_BASE      0x40023800u
#define RCC_AHB1ENR   REG32(RCC_BASE + 0x30u)
#define RCC_APB1ENR   REG32(RCC_BASE + 0x40u)

#define GPIOB_BASE_RAW    0x40020400u
#define GPIOB_MODER   REG32(GPIOB_BASE_RAW + 0x00u)
#define GPIOB_OSPEEDR REG32(GPIOB_BASE_RAW + 0x08u)
#define GPIOB_AFRH    REG32(GPIOB_BASE_RAW + 0x24u)

#define TIM12_BASE    0x40001800u
#define TIM12_CR1     REG32(TIM12_BASE + 0x00u)
#define TIM12_CNT     REG32(TIM12_BASE + 0x24u)
#define TIM12_PSC     REG32(TIM12_BASE + 0x28u)
#define TIM12_ARR     REG32(TIM12_BASE + 0x2Cu)
#define TIM12_CCR1    REG32(TIM12_BASE + 0x34u)
#define TIM12_CCMR1   REG32(TIM12_BASE + 0x18u)
#define TIM12_CCER    REG32(TIM12_BASE + 0x20u)

/************************************* 旋钮相关 *************************************/

ButtonState btn;

void knob_inter() {
    btn.alv = digitalRead(AIO);
    btn.blv = digitalRead(BIO);
    if (!btn.flag && btn.alv == LOW) {
        btn.CW_1 = btn.blv;
        btn.flag = true;
    }
    if (btn.flag && btn.alv) {
        btn.CW_2 = !btn.blv;
        if (btn.CW_1 && btn.CW_2) {
            btn.id = ui.param[KNOB_DIR];
            btn.pressed = true;
            btn.buzzer_trig = true;
        }
        if (btn.CW_1 == false && btn.CW_2 == false) {
            btn.id = !ui.param[KNOB_DIR];
            btn.pressed = true;
            btn.buzzer_trig = true;
        }
        btn.flag = false;
    }
}

void btn_scan() {
    static uint8_t  s_last_sample = 1;
    static uint8_t  s_stable      = 1;
    static uint32_t s_change_ms   = 0;
    static uint32_t s_press_ms    = 0;
    static bool     s_long_done   = false;

    uint8_t raw = digitalRead(SW);
    uint32_t now = millis();

    if (raw != s_last_sample) {
        s_last_sample = raw;
        s_change_ms = now;
    }

    if ((uint32_t)(now - s_change_ms) >= 5u) {
        if (s_stable != s_last_sample) {
            s_stable = s_last_sample;
            if (s_stable == LOW) {
                s_press_ms = now;
                s_long_done = false;
            } else {
                if (!s_long_done) {
                    btn.pressed = true;
                    btn.id = BTN_ID_SP;
                    btn.buzzer_confirm = true;
                }
            }
        }
    }

    if (s_stable == LOW && !s_long_done &&
        (uint32_t)(now - s_press_ms) >= (uint32_t)(ui.param[BTN_LPT] * 2u)) {
        s_long_done = true;
        btn.pressed = true;
        btn.id = BTN_ID_LP;
    }
}

void btn_init() {
    pinMode(AIO, INPUT_PULLUP);
    pinMode(BIO, INPUT_PULLUP);
    pinMode(SW, INPUT_PULLUP);

    RCC_AHB1ENR |= (1u << 1);
    uint32_t tmp = GPIOB_MODER;
    tmp &= ~(0x3u << 28);
    tmp |= (0x2u << 28);
    GPIOB_MODER = tmp;
    GPIOB_OSPEEDR |= (0x3u << 28);
    tmp = GPIOB_AFRH;
    tmp &= ~(0xFu << 24);
    tmp |= (9u << 24);
    GPIOB_AFRH = tmp;

    RCC_APB1ENR |= (1u << 6);
    TIM12_PSC = 83;
    TIM12_ARR = 399;
    TIM12_CCR1 = 0;
    tmp = TIM12_CCMR1;
    tmp &= ~(0x7u << 4);
    tmp |= (6u << 4);
    TIM12_CCMR1 = tmp;
    TIM12_CCER |= (1u << 0);
    TIM12_CR1 |= (1u << 7);

    attachInterrupt(digitalPinToInterrupt(AIO), knob_inter, CHANGE);
}

void buzzer_exit_sound() {
    btn.buzzer_exit = true;
}

void buzzer_proc() {
    static bool  s_is_confirm = false;
    static bool  s_is_exit = false;
    static uint8_t s_exit_phase = 0;
    static const uint32_t k_rot_arr[5] = {0, 1199, 799, 532, 399};
    static const uint32_t k_rot_ccr[5] = {0, 360, 320, 266, 260};
    const uint32_t rot_total_ms  = 120;
    const uint32_t rot_attack_ms = 10;
    const uint32_t rot_release_ms = 20;
    const uint32_t cnf_total_ms  = 80;
    const uint32_t cnf_attack_ms = 3;
    const uint32_t cnf_release_ms = 15;
    const uint32_t exit_single_ms = 80;
    const uint32_t exit_gap_ms = 60;
    const uint32_t exit_attack_ms = 3;
    const uint32_t exit_release_ms = 15;

    if (btn.buzzer_exit) {
        s_is_exit = true;
        s_exit_phase = 0;
        btn.buzzer_exit = false;
        uint8_t vol = ui.param[BUZ_VOL];
        if (vol == 0) return;
        uint32_t arr = k_rot_arr[vol] * 2 / 3;
        TIM12_CNT = 0;
        TIM12_ARR = arr;
        TIM12_CCR1 = 0;
        btn.buzzer_start = millis();
        TIM12_CR1 |= (1u << 0);
    }

    if (btn.buzzer_trig || btn.buzzer_confirm) {
        s_is_confirm = btn.buzzer_confirm;
        s_is_exit = false;
        btn.buzzer_trig = false;
        btn.buzzer_confirm = false;
        uint8_t vol = ui.param[BUZ_VOL];
        if (vol == 0) return;
        uint32_t arr = k_rot_arr[vol];
        if (s_is_confirm) {
            arr = k_rot_arr[vol] * 3 / 2;
        }
        TIM12_CNT = 0;
        TIM12_ARR = arr;
        TIM12_CCR1 = 0;
        btn.buzzer_start = millis();
        TIM12_CR1 |= (1u << 0);
    }

    if (btn.buzzer_start == 0) return;

    uint32_t total_ms   = s_is_confirm ? cnf_total_ms  : (s_is_exit ? (exit_single_ms + exit_gap_ms + exit_single_ms) : rot_total_ms);
    uint32_t attack_ms  = s_is_confirm ? cnf_attack_ms : (s_is_exit ? exit_attack_ms : rot_attack_ms);
    uint32_t release_ms = s_is_confirm ? cnf_release_ms : (s_is_exit ? exit_release_ms : rot_release_ms);
    uint32_t elapsed_ms = (uint32_t)(millis() - btn.buzzer_start);

    if (s_is_exit) {
        uint8_t vol = ui.param[BUZ_VOL];
        uint32_t target_ccr = k_rot_ccr[vol];
        uint32_t arr = k_rot_arr[vol] * 2 / 3;

        if (s_exit_phase == 0) {
            if (elapsed_ms < exit_single_ms) {
                uint32_t current_ccr = target_ccr;
                if (elapsed_ms < attack_ms)
                    current_ccr = current_ccr * elapsed_ms / attack_ms;
                else if (elapsed_ms > exit_single_ms - release_ms)
                    current_ccr = current_ccr * (exit_single_ms - elapsed_ms) / release_ms;
                TIM12_CCR1 = current_ccr;
            } else {
                s_exit_phase = 1;
                TIM12_CCR1 = 0;
            }
        } else if (s_exit_phase == 1) {
            if (elapsed_ms < exit_single_ms + exit_gap_ms) {
                TIM12_CCR1 = 0;
            } else {
                s_exit_phase = 2;
                TIM12_ARR = arr;
            }
        } else if (s_exit_phase == 2) {
            uint32_t phase_elapsed = elapsed_ms - (exit_single_ms + exit_gap_ms);
            if (phase_elapsed < exit_single_ms) {
                uint32_t current_ccr = target_ccr;
                if (phase_elapsed < attack_ms)
                    current_ccr = current_ccr * phase_elapsed / attack_ms;
                else if (phase_elapsed > exit_single_ms - release_ms)
                    current_ccr = current_ccr * (exit_single_ms - phase_elapsed) / release_ms;
                TIM12_CCR1 = current_ccr;
            } else {
                TIM12_CCR1 = 0;
                TIM12_CR1 &= ~(1u << 0);
                btn.buzzer_start = 0;
                s_is_exit = false;
                s_exit_phase = 0;
            }
        }
        return;
    }

    if (elapsed_ms >= total_ms) {
        TIM12_CCR1 = 0;
        TIM12_CR1 &= ~(1u << 0);
        btn.buzzer_start = 0;
        return;
    }

    uint8_t vol = ui.param[BUZ_VOL];
    uint32_t target_ccr = k_rot_ccr[vol];

    uint32_t current_ccr = target_ccr;
    if (elapsed_ms < attack_ms)
        current_ccr = current_ccr * elapsed_ms / attack_ms;
    else if (elapsed_ms > total_ms - release_ms)
        current_ccr = current_ccr * (total_ms - elapsed_ms) / release_ms;

    TIM12_CCR1 = current_ccr;
}
