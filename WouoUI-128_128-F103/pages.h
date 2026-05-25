#ifndef PAGES_H
#define PAGES_H

#include "ui_types.h"

void check_box_v_init(uint8_t *param);
void check_box_m_init(uint8_t *param);
void check_box_s_init(uint8_t *param, uint8_t *param_p);
void check_box_m_select(uint8_t param);
void check_box_s_select(uint8_t val, uint8_t pos);
void list_draw_text_and_check_box(Menu* arr, int i);
void list_draw_value(int n);
void list_draw_check_box_frame();
void list_draw_check_box_dot();
void list_draw_krf(int n);
void list_draw_kpf(int n);
void tile_param_init();
void tile_show(Menu* arr_1, Menu* arr_2, const uint8_t icon_pic[][16 * 18]);
void list_rotate_switch();
void list_show(Menu* arr, uint8_t ui_index);
void volt_param_init();
void volt_show();
void sleep_param_init();
void about_param_init();
void about_show();
void layer_init_in();
void layer_init_out();
void tile_rotate_switch();
void main_proc();
void editor_proc();
void knob_proc();
void krf_proc();
void kpf_proc();
void volt_proc();
void setting_proc();
void about_proc();
void ui_proc();
void window_value_init(char title[], uint8_t select, uint8_t *value, uint8_t max, uint8_t min, uint8_t step, Menu* bg, uint8_t index);
void ui_param_init();
void ui_init();
void fade();
void window_show();
void window_proc();
void window_param_init();
void knob_param_init();
void krf_param_init();
void kpf_param_init();
void setting_param_init();

#endif