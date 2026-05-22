#ifndef DISPLAY_H
#define DISPLAY_H

#include "config.h"
#include <SPI.h>

extern U8G2_LS013B7DH03_128X128_F_4W_SW_SPI u8g2;
extern uint8_t *buf_ptr;
extern uint16_t buf_len;

void lcd_init();

#endif
