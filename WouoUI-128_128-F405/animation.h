#ifndef ANIMATION_H
#define ANIMATION_H

#include "config.h"

/************************************* 动画函数 *************************************/

//动画函数
void animation(float *a, float *a_trg, uint8_t n);
//弹簧动画函数
void animation_spring(float *a, float *a_trg, float *vel, float stiffness, float damping);
//弹跳动画函数（阻尼振荡，vel兼作速度变量）
void animation_bounce(float *a, float *a_trg, float *vel, uint8_t n);
//消失函数
void fade();

#endif