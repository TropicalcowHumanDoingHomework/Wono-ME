#ifndef WINDOW_H
#define WINDOW_H

#include "ui_types.h"

/************************************* 弹窗相关 *************************************/

#ifdef __cplusplus
extern "C" {
#endif

extern WindowState win;

void window_value_init(char title[], uint8_t select, uint8_t *value, uint8_t max, uint8_t min, uint8_t step, Menu *bg, uint8_t index);
void window_param_init();
void window_show();
void window_proc();

#ifdef __cplusplus
}
#endif

#endif