#include "ui_state.h"
#include "menu_data.h"
#include "eeprom_manager.h"

UiState ui;
TileState tile;
ListState list;
VoltageState volt;
CheckBoxState check_box;
WindowState win;
SpotState spot;
AboutState about;
KnobState knob;

void ui_param_init(void) {
    ui.param[DISP_BRI] = 255;
    ui.param[TILE_ANI] = 30;
    ui.param[LIST_ANI] = 60;
    ui.param[WIN_ANI] = 25;
    ui.param[SPOT_ANI] = 50;
    ui.param[TAG_ANI] = 60;
    ui.param[FADE_ANI] = 30;
    ui.param[BTN_SPT] = 25;
    ui.param[BTN_LPT] = 150;
    ui.param[TILE_UFD] = 1;
    ui.param[LIST_UFD] = 1;
    ui.param[TILE_LOOP] = 0;
    ui.param[LIST_LOOP] = 0;
    ui.param[WIN_BOK] = 0;
    ui.param[KNOB_DIR] = 0;
    ui.param[DARK_MODE] = 1;
    ui.param[ROTATE_SCR] = 0;
    ui.param[LIST_CUR] = 0;
    ui.param[SLP_T] = 0;
}

void ui_init(void) {
    ui.num[M_MAIN] = 4;
    ui.num[M_EDITOR] = 12;
    ui.num[M_KNOB] = 3;
    ui.num[M_KRF] = 7;
    ui.num[M_KPF] = 82;
    ui.num[M_VOLT] = 10;
    ui.num[M_SETTING] = 21;
    ui.num[M_ABOUT] = 8;
}

void check_box_list_1_init(void) {}
void check_box_list_1_select(void) {}
void check_box_list_2_init(void) {}
void check_box_list_2_select(void) {}
