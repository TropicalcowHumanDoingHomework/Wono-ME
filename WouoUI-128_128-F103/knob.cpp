#include "knob.h"
#include "ui_state.h"

/************************************* 旋钮相关 *************************************/

//按钮变量
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
        }
        if (btn.CW_1 == false && btn.CW_2 == false) {
            btn.id = !ui.param[KNOB_DIR];
            btn.pressed = true;
        }
        btn.flag = false;
    }
}

void btn_init() {
    pinMode(AIO, INPUT_PULLUP);
    pinMode(BIO, INPUT_PULLUP);
    pinMode(SW, INPUT_PULLUP);
    pinMode(PB15, OUTPUT);
    digitalWrite(PB15, LOW);
    attachInterrupt(digitalPinToInterrupt(AIO), knob_inter, CHANGE);
}

void btn_scan() {
    static int val = 1;
    static int val_last = 1;
    static int count = 0;
    val = digitalRead(SW);
    if (val != val_last) {
        val_last = val;
        delay(ui.param[BTN_SPT] * 2);
        val = digitalRead(SW);
        if (val == LOW) {
            btn.pressed = true;
            count = 0;
            while (!digitalRead(SW)) {
                count++;
                delay(1);
            }
            if (count < ui.param[BTN_LPT] * 2) {
                btn.id = BTN_ID_SP;
            } else {
                btn.id = BTN_ID_LP;
            }
        }
    }
}