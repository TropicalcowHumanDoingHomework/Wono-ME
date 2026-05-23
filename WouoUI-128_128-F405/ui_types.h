#ifndef UI_TYPES_H
#define UI_TYPES_H

#include "config.h"

/************************************* 定义内容 *************************************/

//菜单结构体
struct Menu {
    char title[32];
};

/************************************* 页面变量 *************************************/

//UI状态
struct UiState {
    bool init;
    uint8_t num[UI_MNUMB];
    uint8_t select[UI_DEPTH];
    uint8_t layer;
    uint8_t index;
    uint8_t state;
    bool sleep;
    uint8_t fade = 1;
    uint8_t param[UI_PARAM];
};

//磁贴状态
struct TileState {
    float title_y_calc   = TILE_INDI_S + (TILE_INDI_H - TILE_B_TITLE_H) / 2 + TILE_B_TITLE_H * 2;
    float title_y_trg_calc = TILE_INDI_S + (TILE_INDI_H - TILE_B_TITLE_H) / 2 + TILE_B_TITLE_H;
    int16_t temp;
    bool select_flag;
    float icon_x;
    float icon_x_trg;
    float icon_y;
    float icon_y_trg;
    float indi_x;
    float indi_x_trg;
    float title_y;
    float title_y_trg;
};

//列表状态
struct ListState {
    int line_n;
    int16_t temp;
    bool loop;
    float curve;
    float y;
    float y_trg;
    float box_x;
    float box_x_trg;
    float box_y;
    float box_y_trg[UI_DEPTH];
    float bar_y;
    float bar_y_trg;
    float box_W;
    float box_w;
    float box_w_trg;
    float box_H;
    float box_h;
    float box_h_trg;
};

//电压测量状态
struct VoltageState {
    int ch0_adc[WAVE_SAMPLE * WAVE_W];
    int val;
    int ch0_wave[128];
    float text_bg_l;
    float text_bg_l_trg;
};

//选择框状态
struct CheckBoxState {
    uint8_t* v;
    uint8_t* m;
    uint8_t* s;
    uint8_t* s_p;
    uint8_t* map;
};

//弹窗状态
struct WindowState {
    char title[32];
    char message[128];
    uint8_t select;
    uint8_t* value;
    uint8_t max;
    uint8_t min;
    uint8_t step;
    Menu* bg;
    uint8_t index;
    uint8_t msg_mode;
    uint8_t list_mode;
    char list_items[WIN_LIST_MAX][WIN_LIST_ITEM_LEN];
    uint8_t list_count;
    uint8_t list_select;
    float hl_sel_cur;
    float hl_sel_trg;
    float list_y;
    float list_y_trg;
    uint8_t bokeh_step;
    uint32_t last_bokeh_time;
    float bar;
    float bar_trg;
    float y;
    float y_trg;
    float l;
    float u;
    float w;
    float w_trg;
    float h;
    float h_trg;
    float box_H;
    float box_h;
    float box_h_trg;
};

//聚光灯状态
struct SpotState {
    float x;
    float y;
    float x_trg;
    float y_trg;
};

//关于页面状态
struct AboutState {
    uint8_t index;
    float indi_x;
    float indi_x_trg;
};

//旋钮状态
struct KnobState {
    uint8_t param[KNOB_PARAM];
};

//按钮状态
struct ButtonState {
    bool pressed;
    uint8_t id;
    bool alv;
    bool blv;
    bool flag;
    bool CW_1;
    bool CW_2;
    bool CC_1;
    bool CC_2;
    bool pressed_1;
    bool pressed_2;
    bool long_pressed;
    uint32_t spt;
    uint32_t lpt;
    uint32_t spt_cnt;
    uint32_t lpt_cnt;
    bool buzzer_trig;
    bool buzzer_confirm;
    bool buzzer_exit;
    bool buzzer_boot;
    uint32_t buzzer_start;
};

//EEPROM状态
struct EepromState {
    uint8_t check;
    uint16_t address;
    bool change;
    uint8_t check_param[EEPROM_CHECK];
};

#endif