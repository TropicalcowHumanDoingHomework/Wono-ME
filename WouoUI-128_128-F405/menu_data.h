#ifndef MENU_DATA_H
#define MENU_DATA_H

#include "ui_types.h"

/************************************* 文字内容 *************************************/

//主菜单项数组：磁贴页面显示的大标题
extern Menu main_menu[];

//主菜单扩展描述数组：磁贴页面显示的小标题
extern Menu main_menu_exp[];

//编辑器菜单项数组：功能测试页面
extern Menu editor_menu[];

//旋钮设置菜单项数组：配置旋钮的旋转和按键功能
extern Menu knob_menu[];

//旋钮旋转功能选择菜单数组：禁用/音量/亮度
extern Menu krf_menu[];

//旋钮按键功能选择菜单数组：选择按键输出的HID键码
extern Menu kpf_menu[];

//电压测量菜单数组：列出可选择的ADC通道
extern Menu volt_menu[];

//设置菜单数组：系统参数配置页面
extern Menu setting_menu[];

//关于本机菜单数组：显示设备信息
extern Menu about_menu[];

//动画设置菜单数组：动画参数配置页面
extern Menu animition_menu[];

/************************************* 列表选择弹窗测试数据 *************************************/

//列表选择弹窗测试菜单项
#define WIN_LIST_TEST_ITEMS_NUM 8
extern const char* win_list_test_items[];

/************************************* 图片内容 *************************************/

//磁贴图标位图数据（存储在Flash/PROGMEM中），每个图标16*18字节（128*128像素）
extern PROGMEM const uint8_t main_icon_pic[][16 * 18];

#endif