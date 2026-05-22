#include "ui_state.h"
#include "menu_data.h"
#include "eeprom_manager.h"

/************************************* UI状态变量定义 *************************************/

UiState ui;
TileState tile;
ListState list;
VoltageState volt;
CheckBoxState check_box;
WindowState win;
SpotState spot;
AboutState about;
KnobState knob;

/************************************* UI初始化函数 *************************************/

//在初始化EEPROM时，选择性初始化的默认设置
void ui_param_init() {
    ui.param[DISP_BRI] = 255;      //屏幕亮度
    ui.param[TILE_ANI] = 30;       //磁贴动画速度
    ui.param[LIST_ANI] = 60;       //列表动画速度
    ui.param[WIN_ANI] = 25;        //弹窗动画速度
    ui.param[SPOT_ANI] = 50;       //聚光动画速度
    ui.param[TAG_ANI] = 60;        //标签动画速度
    ui.param[FADE_ANI] = 30;       //消失动画速度
    ui.param[BTN_SPT] = 25;        //按键短按时长
    ui.param[BTN_LPT] = 150;       //按键长按时长
    ui.param[TILE_UFD] = 1;        //磁贴图标从头展开开关
    ui.param[LIST_UFD] = 1;        //菜单列表从头展开开关
    ui.param[TILE_LOOP] = 0;       //磁贴图标循环模式开关
    ui.param[LIST_LOOP] = 0;       //菜单列表循环模式开关
    ui.param[WIN_BOK] = 0;         //弹窗背景虚化开关
    ui.param[KNOB_DIR] = 0;        //旋钮方向切换开关
    ui.param[DARK_MODE] = 1;       //黑暗模式开关
    ui.param[ROTATE_SCR] = 0;      //屏幕旋转
    ui.param[LIST_CUR] = 0;        //列表弯曲程度
    ui.param[SLP_T] = 0;           //休眠延时 0=关
}

//列表类页面列表行数初始化，必须初始化的参数
void ui_init() {
    ui.num[M_MAIN] = 4;
    ui.num[M_EDITOR] = 12;
    ui.num[M_KNOB] = 3;
    ui.num[M_KRF] = 7;
    ui.num[M_KPF] = 82;
    ui.num[M_VOLT] = 10;
    ui.num[M_SETTING] = 21;
    ui.num[M_ABOUT] = 8;
}

void check_box_list_1_init() {
}

void check_box_list_1_select() {
}

void check_box_list_2_init() {
}

void check_box_list_2_select() {
}