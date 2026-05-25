#ifndef WINDOW_H
#define WINDOW_H

#include "ui_types.h"

/************************************* 弹窗相关 *************************************/

extern WindowState win;

void window_value_init(char title[], uint8_t select, uint8_t *value, uint8_t max, uint8_t min, uint8_t step, Menu *bg, uint8_t index);
void window_message_init(const char title[], const char message[], Menu *bg, uint8_t index);
void window_param_init();
void window_show();
void window_proc();

#endif