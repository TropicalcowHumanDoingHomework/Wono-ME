#ifndef ANIMATION_H
#define ANIMATION_H

#include "config.h"

/************************************* 动画函数 *************************************/

//缓动动画函数：将值a平滑过渡到目标值a_trg，n为参数索引决定动画速度
void animation(float *a, float *a_trg, uint8_t n);

//弹簧动画函数：使用弹簧质点模型（刚度stiffness，阻尼damping），vel为速度状态变量
void animation_spring(float *a, float *a_trg, float *vel, float stiffness, float damping);

//弹跳动画函数：阻尼振荡解析公式，vel兼作速度变量，n为参数索引
void animation_bounce(float *a, float *a_trg, float *vel, uint8_t n);

//消失函数：棋盘格或整体遮罩方式渐入/渐出画面
void fade();

#endif