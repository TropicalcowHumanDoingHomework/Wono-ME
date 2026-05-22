#include "knob.h"
#include "ui_state.h"
#include "delay.h"

ButtonState btn;

void knob_init(void) {
    GPIO_InitTypeDef gpio;
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    
    /* AIO(PB12), BIO(PB13) 浮空输入 */
    gpio.GPIO_Pin = GPIO_Pin_12 | GPIO_Pin_13;
    gpio.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &gpio);
    
    /* SW(PB14) 上拉输入 */
    gpio.GPIO_Pin = GPIO_Pin_14;
    gpio.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(GPIOB, &gpio);
}

void knob_inter(void) {
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

void btn_scan(void) {
    static int val = 1;
    static int val_last = 1;
    static int count = 0;
    val = PBin(14);
    if (val != val_last) {
        val_last = val;
        delay_ms(ui.param[BTN_SPT] * 2);
        val = PBin(14);
        if (val == 0) {
            btn.pressed = 1;
            count = 0;
            while (PBin(14) == 0) {
                count++;
                delay_ms(1);
            }
            if (count < ui.param[BTN_LPT] * 2) {
                btn.id = BTN_ID_SP;
            } else {
                btn.id = BTN_ID_LP;
            }
        }
    }
}

void btn_init(void) {
    knob_init();
}
