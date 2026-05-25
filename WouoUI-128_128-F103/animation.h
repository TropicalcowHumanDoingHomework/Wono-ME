#ifndef ANIMATION_H
#define ANIMATION_H

#include "config.h"

/************************************* 动画函数 *************************************/

//动画函数
void animation(float *a, float *a_trg, uint8_t n);
void fade();
void fade_sleep();
void fade_wake();

#endif