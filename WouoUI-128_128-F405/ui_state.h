#ifndef UI_STATE_H
#define UI_STATE_H

#include "ui_types.h"

/************************************* UI状态变量 *************************************/

//UI主状态变量：管理页面切换、菜单层级、用户参数
extern UiState ui;

//磁贴状态变量：管理磁贴页面的图标、指示器、标题动画位置
extern TileState tile;

//列表状态变量：管理列表页面的滚动、选择框、进度条动画
extern ListState list;

//电压测量状态变量：管理ADC采样和波形数据
extern VoltageState volt;

//复选框状态变量：管理单选框和多选框的选中状态
extern CheckBoxState check_box;

//弹窗状态变量：管理数值调节、消息、列表选择、确认弹窗
extern WindowState win;

//聚光灯状态变量：管理聚光灯聚焦动画位置
extern SpotState spot;

//关于页面状态变量：管理关于本机页面信息滚动
extern AboutState about;

//旋钮状态变量：管理旋钮功能配置参数
extern KnobState knob;

/************************************* UI初始化函数 *************************************/

//在初始化EEPROM时，选择性初始化的默认设置
void ui_param_init();
//列表类页面列表行数初始化，必须初始化的参数
void ui_init();

#endif