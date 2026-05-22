#ifndef DISPLAY_H
#define DISPLAY_H

#include "config.h"

/************************************* 屏幕变量 *************************************/

#ifdef USE_I2C
extern U8G2_SH1107_SEEED_128X128_F_HW_I2C u8g2;
#else
extern U8G2_SH1107_SEEED_128X128_F_4W_HW_SPI u8g2;
#endif
extern uint8_t *buf_ptr;                 //指向屏幕缓冲的指针
extern uint16_t buf_len;                  //缓冲长度

/************************************* 显示函数 *************************************/

void oled_init();

#endif