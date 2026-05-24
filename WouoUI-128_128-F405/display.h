#ifndef DISPLAY_H
#define DISPLAY_H

#include "config.h"
#include <SPI.h>

/************************************* 显示模块 *************************************/

//U8g2屏幕对象，夏普LS013B7DH03 128x128 Memory LCD，4线软件SPI
extern U8G2_LS013B7DH03_128X128_F_4W_SW_SPI u8g2;

//屏幕帧缓冲区指针，用于直接操作显存实现特殊效果
extern uint8_t *buf_ptr;

//屏幕帧缓冲区长度（字节数）
extern uint16_t buf_len;

/************************************* 显示初始化 *************************************/

//初始化LCD屏幕，配置SPI3硬件驱动和U8g2回调
void lcd_init();

#endif
