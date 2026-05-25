#ifndef UI_TYPES_H
#define UI_TYPES_H

#include "config.h"

struct Menu {
    char title[32];
};
typedef struct Menu Menu;

struct UiState {
    uint8_t init;
    uint8_t num[UI_MNUMB];
    uint8_t select[UI_DEPTH];
    uint8_t layer;
    uint8_t index;
    uint8_t state;
    uint8_t sleep;
    uint8_t fade;
    uint8_t param[UI_PARAM];
    uint8_t last_index;
    uint8_t last_select;
    uint8_t last_box_y_trg;
    uint8_t window_sleep;
    uint8_t wake_fade;
    uint32_t idle_timer;
};
typedef struct UiState UiState;

struct TileState {
    float rot;
    float rot_trg;
    float x;
    float y;
    float x_trg;
    float y_trg;
    float s;
    float s_trg;
    float u;
    float u_trg;
    uint8_t ufd;
    float icon_x;
    float icon_x_trg;
    float icon_y;
    float icon_y_trg;
    float indi_x;
    float indi_x_trg;
    float title_y;
    float title_y_trg;
    float title_y_calc;
    float title_y_trg_calc;
    int16_t temp;
    uint8_t select_flag;
};
typedef struct TileState TileState;

struct ListState {
    float y;
    float y_trg;
    float l;
    float l_trg;
    float w;
    float w_trg;
    float u;
    float u_trg;
    uint8_t ufd;
    float box_x;
    float box_x_trg;
    float box_y;
    float box_y_trg[UI_DEPTH];
    float bar_y;
    float bar_y_trg;
    uint8_t loop;
    int line_n;
    int16_t temp;
    float curve;
};
typedef struct ListState ListState;

struct VoltageState {
    float y;
    float y_trg;
    uint8_t select;
    uint8_t loop;
    float text_bg_l;
    float text_bg_l_trg;
    int val;
    int ch0_wave[128];
};
typedef struct VoltageState VoltageState;

struct CheckBoxState {
    uint8_t select[UI_MNUMB];
    uint8_t *v;
    uint8_t *m;
    uint8_t *s;
    uint8_t *s_p;
    uint8_t v_map[UI_MNUMB];
    uint8_t m_map[UI_MNUMB];
};
typedef struct CheckBoxState CheckBoxState;

struct WindowState {
    char title[32];
    char message[128];
    uint8_t select;
    uint8_t msg_mode;
    uint8_t *value;
    uint8_t max;
    uint8_t min;
    uint8_t step;
    Menu *bg;
    uint8_t index;
    float bar;
    float bar_trg;
    float y;
    float y_trg;
    float h;
    float h_trg;
    float w;
    float w_trg;
    float l;
    float u;
};
typedef struct WindowState WindowState;

struct SpotState {
    float x;
    float y;
    float x_trg;
    float y_trg;
};
typedef struct SpotState SpotState;

struct AboutState {
    uint8_t index;
    float indi_x;
    float indi_x_trg;
};
typedef struct AboutState AboutState;

struct KnobState {
    uint8_t param[KNOB_PARAM];
};
typedef struct KnobState KnobState;

struct ButtonState {
    uint8_t pressed;
    uint8_t id;
    uint8_t alv;
    uint8_t blv;
    uint8_t flag;
    uint8_t CW_1;
    uint8_t CW_2;
    uint8_t CC_1;
    uint8_t CC_2;
    uint8_t pressed_1;
    uint8_t pressed_2;
    uint8_t long_pressed;
    uint32_t spt;
    uint32_t lpt;
    uint32_t spt_cnt;
    uint32_t lpt_cnt;
};
typedef struct ButtonState ButtonState;

struct EepromState {
    uint8_t check;
    uint16_t address;
    uint8_t change;
    uint8_t check_param[EEPROM_CHECK];
};
typedef struct EepromState EepromState;

#endif
