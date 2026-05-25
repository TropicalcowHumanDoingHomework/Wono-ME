#ifndef KNOB_H
#define KNOB_H

#include "ui_types.h"

/************************************* 旋钮相关 *************************************/

//按钮变量
extern ButtonState btn;

void knob_inter();
void btn_scan();
void btn_init();

#endif