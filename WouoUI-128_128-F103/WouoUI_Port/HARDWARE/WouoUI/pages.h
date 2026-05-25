#ifndef PAGES_H
#define PAGES_H

#include "ui_types.h"

#ifdef __cplusplus
extern "C" {
#endif

void check_box_v_init(uint8_t *param);
void check_box_m_init(uint8_t *param);
void check_box_s_init(uint8_t *param, uint8_t *param_p);
void check_box_m_select(uint8_t param);
void check_box_s_select(uint8_t val, uint8_t pos);
void list_draw_text_and_check_box(Menu* arr, int i);
void list_draw_value(int n);
void list_draw_check_box_frame(void);
void list_draw_check_box_dot(void);
void list_draw_krf(int n);
void list_draw_kpf(int n);
void tile_param_init(void);
void tile_show(Menu* arr_1, Menu* arr_2, const uint8_t icon_pic[][16 * 18]);
void list_rotate_switch(void);
void list_show(Menu* arr, uint8_t ui_index);
void volt_param_init(void);
void volt_show(void);
void sleep_param_init(void);
void about_param_init(void);
void about_show(void);
void layer_init_in(void);
void layer_init_out(void);
void tile_rotate_switch(void);
void main_proc(void);
void editor_proc(void);
void knob_proc(void);
void krf_proc(void);
void kpf_proc(void);
void volt_proc(void);
void setting_proc(void);
void about_proc(void);
void ui_proc(void);
void window_value_init(const char title[], uint8_t select, uint8_t *value, uint8_t max, uint8_t min, uint8_t step, Menu* bg, uint8_t index);
void ui_param_init(void);
void ui_init(void);
void fade(void);
void fade_sleep(void);
void fade_wake(void);
void window_show(void);
void window_proc(void);
void window_param_init(void);
void knob_param_init(void);
void krf_param_init(void);
void kpf_param_init(void);
void setting_param_init(void);

#ifdef __cplusplus
}
#endif

#endif
