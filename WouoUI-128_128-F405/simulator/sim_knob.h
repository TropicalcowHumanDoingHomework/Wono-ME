#ifndef SIM_KNOB_H
#define SIM_KNOB_H

#include "sim_config.h"
#include "ui_types.h"

extern ButtonState btn;

void knob_inter();
void btn_scan();
void btn_init();
void buzzer_proc();
void buzzer_exit_sound();
void buzzer_boot_sound();

// 模拟器专用：处理 SDL2 事件，映射键盘到按钮事件
void sim_knob_handle_key(int key, bool down);

#endif
