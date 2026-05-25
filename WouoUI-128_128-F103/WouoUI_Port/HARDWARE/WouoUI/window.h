#ifndef WINDOW_H
#define WINDOW_H

#include "ui_types.h"

/************************************* 弹窗相关 *************************************/

#ifdef __cplusplus
extern "C" {
#endif

extern WindowState win;

void window_value_init(const char title[], uint8_t select, uint8_t *value, uint8_t max, uint8_t min, uint8_t step, Menu *bg, uint8_t index);
void window_message_init(const char title[], const char message[], Menu *bg, uint8_t index);
void window_param_init(void);
void window_show(void);
void window_proc(void);

#ifdef __cplusplus
}
#endif

#endif
