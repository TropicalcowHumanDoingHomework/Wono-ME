#ifndef UI_STATE_H
#define UI_STATE_H

#include "ui_types.h"

/************************************* UI状态变量 *************************************/

extern UiState ui;
extern TileState tile;
extern ListState list;
extern VoltageState volt;
extern CheckBoxState check_box;
extern WindowState win;
extern SpotState spot;
extern AboutState about;
extern KnobState knob;

/************************************* UI初始化函数 *************************************/

//在初始化EEPROM时，选择性初始化的默认设置
void ui_param_init();
//列表类页面列表行数初始化，必须初始化的参数
void ui_init();

#endif