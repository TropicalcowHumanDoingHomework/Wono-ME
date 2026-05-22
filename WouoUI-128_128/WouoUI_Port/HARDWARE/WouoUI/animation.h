#ifndef ANIMATION_H
#define ANIMATION_H

#include "config.h"

/************************************* 动画函数 *************************************/

#ifdef __cplusplus
extern "C" {
#endif

//动画函数
void animation(float *a, float *a_trg, uint8_t n);
//消失函数
void fade();
void fade_sleep();
void fade_wake();

#ifdef __cplusplus
}
#endif

#endif