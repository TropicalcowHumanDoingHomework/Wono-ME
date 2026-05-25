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
    bool init = false;
    uint8_t num[UI_MNUMB];
    uint8_t select[UI_DEPTH];
    uint8_t layer = 0;
    uint8_t index = M_MAIN;
    uint8_t state = S_LAYER_IN;
    bool sleep = false;
    uint8_t fade = 1;
    uint8_t param[UI_PARAM];
    uint8_t last_index = M_MAIN;
    uint8_t last_select = 0;
    uint8_t last_box_y_trg = 0;
    bool wake_fade = false;
    bool window_sleep = false;
    uint32_t idle_timer = 0;
};

//磁贴状态
struct TileState {
    float rot = 0;
    float rot_trg = 0;
    float x = 0;
    float y = 0;
    float x_trg = 0;
    float y_trg = 0;
    float s = 0;
    float s_trg = 0;
    float u = 0;
    float u_trg = 0;
    bool ufd = false;
    float icon_x = 0;
    float icon_x_trg = 0;
    float icon_y = 0;
    float icon_y_trg = 0;
    float indi_x = 0;
    float indi_x_trg = 0;
    float title_y = 0;
    float title_y_trg = 0;
    float title_y_calc = TILE_INDI_S + (TILE_INDI_H - TILE_B_TITLE_H) / 2 + TILE_B_TITLE_H * 2;
    float title_y_trg_calc = TILE_INDI_S + (TILE_INDI_H - TILE_B_TITLE_H) / 2 + TILE_B_TITLE_H;
    int16_t temp = 0;
    bool select_flag = false;
};

//列表状态
struct ListState {
    float y = 0;
    float y_trg = 0;
    float l = 0;
    float l_trg = 0;
    float w = 0;
    float w_trg = 0;
    float u = 0;
    float u_trg = 0;
    bool ufd = false;
    float box_x = 0;
    float box_x_trg = 0;
    float box_y = 0;
    float box_y_trg[UI_DEPTH];
    float bar_y = 0;
    float bar_y_trg = 0;
    bool loop = false;
    int line_n = DISP_H / LIST_LINE_H;
    int16_t temp = 0;
    float curve = 0;
};

//电压测量状态
struct VoltageState {
    float y;
    float y_trg;
    uint8_t select;
    bool loop;
    float text_bg_l;
    float text_bg_l_trg;
    int val;
    int ch0_wave[128];
};

//选择框状态
struct CheckBoxState {
    bool select[UI_MNUMB];
    uint8_t* v;
    uint8_t* m;
    uint8_t* s;
    uint8_t* s_p;
    uint8_t v_map[UI_MNUMB];
    uint8_t m_map[UI_MNUMB];
};

//弹窗状态
struct WindowState {
    char title[32];
    char message[128];
    uint8_t select = 0;
    uint8_t msg_mode = 0;
    uint8_t* value = nullptr;
    uint8_t max = 0;
    uint8_t min = 0;
    uint8_t step = 0;
    Menu* bg = nullptr;
    uint8_t index = 0;
    float bar = 0;
    float bar_trg = 0;
    float y = 0;
    float y_trg = 0;
    float h = WIN_H;
    float h_trg = WIN_H;
    float w = WIN_W;
    float w_trg = WIN_W;
    float l = (DISP_W - WIN_W) / 2;
    float u = (DISP_H - WIN_H) / 2;
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
    uint8_t param[KNOB_PARAM] = {KNOB_DISABLE, KNOB_DISABLE, 2, 2};
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
};

//EEPROM状态
struct EepromState {
    uint8_t check = 0;
    uint16_t address = 0;
    bool change = false;
    uint8_t check_param[EEPROM_CHECK] = {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k'};
};

#endif