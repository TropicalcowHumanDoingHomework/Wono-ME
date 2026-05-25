#include "knob.h"
#include "ui_state.h"
#include "delay.h"

ButtonState btn;

void knob_init(void) {
    GPIO_InitTypeDef gpio;
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    
    /* AIO(PB12), BIO(PB13) 上拉输入(匹配Arduino INPUT_PULLUP) */
    gpio.GPIO_Pin = GPIO_Pin_12 | GPIO_Pin_13;
    gpio.GPIO_Mode = GPIO_Mode_IPU;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &gpio);
    
    /* SW(PB14) 上拉输入 */
    gpio.GPIO_Pin = GPIO_Pin_14;
    gpio.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(GPIOB, &gpio);
}

void knob_inter(void) {
    /* 按键按住期间屏蔽旋钮事件,防止旋转与按键互相干扰 */
    if (btn.long_pressed) return;
    
    btn.alv = PBin(12);
    btn.blv = PBin(13);
    if (!btn.flag && btn.alv == 0) {
        btn.CW_1 = btn.blv;
        btn.flag = 1;
    }
    if (btn.flag && btn.alv) {
        btn.CW_2 = !btn.blv;
        if (btn.CW_1 && btn.CW_2) {
            btn.id = ui.param[KNOB_DIR];
            btn.pressed = 1;
        }
        if (btn.CW_1 == 0 && btn.CW_2 == 0) {
            btn.id = !ui.param[KNOB_DIR];
            btn.pressed = 1;
        }
        btn.flag = 0;
    }
}

/* 非阻塞按键扫描状态机(消除原版delay_ms阻塞) */
void btn_scan(void) {
    enum { BS_IDLE, BS_DEBOUNCE, BS_PRESSED };
    static uint8_t state = BS_IDLE;
    static uint32_t timer = 0;
    int val;

    val = PBin(14);

    switch (state) {
        case BS_IDLE:
            if (val == 0) {
                state = BS_DEBOUNCE;
                timer = millis();
            }
            break;

        case BS_DEBOUNCE:
            if (millis() - timer >= (uint32_t)(ui.param[BTN_SPT] * 2)) {
                if (PBin(14) == 0) {
                    /* 确认按下,标记按钮保持中(屏蔽旋钮事件) */
                    btn.long_pressed = 1;
                    timer = millis();
                    state = BS_PRESSED;
                } else {
                    state = BS_IDLE;
                }
            }
            break;

        case BS_PRESSED:
            if (PBin(14) != 0) {
                /* 按钮释放,根据保持时长判断短按/长按 */
                btn.pressed = 1;
                btn.long_pressed = 0;
                if (millis() - timer >= (uint32_t)(ui.param[BTN_LPT] * 2)) {
                    btn.id = BTN_ID_LP;
                } else {
                    btn.id = BTN_ID_SP;
                }
                state = BS_IDLE;
            }
            break;
    }
}

void btn_init(void) {
    uint8_t i;
    /* 初始化旋钮参数默认值(匹配Arduino: KNOB_DISABLE,KNOB_DISABLE,2,2) */
    for (i = 0; i < KNOB_PARAM; ++i)
        knob.param[i] = 0;
    knob.param[KNOB_ROT_P] = 2;
    knob.param[KNOB_COD_P] = 2;
    knob_init();
}
