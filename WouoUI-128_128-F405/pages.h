#ifndef PAGES_H
#define PAGES_H

#include "ui_types.h"

/************************************* 页面函数 *************************************/

//复选框数值初始化：将参数数组指针绑定到复选框状态
void check_box_v_init(uint8_t *param);

//复选框多选初始化：将多选框状态数组指针绑定到复选框状态
void check_box_m_init(uint8_t *param);

//复选框单选初始化：将单选框值和位置指针绑定到复选框状态
void check_box_s_init(uint8_t *param, uint8_t *param_p);

//复选框多选切换：切换指定参数的开关状态
void check_box_m_select(uint8_t param);

//复选框单选选择：设置单选框的值为指定值和位置
void check_box_s_select(uint8_t val, uint8_t pos);

//列表通用文字和控件绘制：根据行首符号绘制数值/复选框/单选等
void list_draw_text_and_check_box(Menu* arr, int i);

//列表数值显示：在列表项末尾显示数值
void list_draw_value(int n);

//列表复选框外框绘制：绘制方框
void list_draw_check_box_frame();

//列表复选框内部点绘制：绘制选中状态的圆点
void list_draw_check_box_dot();

//列表旋钮功能显示：显示旋转功能的文字（OFF/VOL/BRI）
void list_draw_krf(int n);

//列表按键键值显示：显示按键值的文字或"?"
void list_draw_kpf(int n);

//磁贴参数初始化：设置磁贴页面的初始动画位置
void tile_param_init();

//磁贴通用显示函数：绘制图标、大标题、小标题、指示器
void tile_show(Menu* arr_1, Menu* arr_2, const uint8_t icon_pic[][16 * 18]);

//磁贴旋转切换处理：处理编码器旋转事件对磁贴选择的影响
void tile_rotate_switch();

//列表旋转切换处理：处理编码器旋转事件对列表选择的影响
void list_rotate_switch();

//列表通用显示函数：绘制列表项、选择框、滚动条、弯曲效果
void list_show(Menu* arr, uint8_t ui_index);

//电压测量参数初始化：设置电压页面的初始动画位置
void volt_param_init();

//电压测量显示函数：绘制ADC波形和电压值
void volt_show();

//关于页面参数初始化：设置关于页面的初始动画位置
void about_param_init();

//关于页面显示函数：绘制设备信息列表
void about_show();

//层级进入动画初始化：进入子菜单时触发的动画
void layer_init_in();

//层级退出动画初始化：退出子菜单时触发的动画
void layer_init_out();

//主菜单处理函数：显示和控制主菜单磁贴页面
void main_proc();

//编辑器处理函数：显示和控制功能测试列表页面
void editor_proc();

//旋钮设置处理函数：显示和控制旋钮功能配置页面
void knob_proc();

//旋钮旋转功能处理函数：显示和控制旋转功能选择页面
void krf_proc();

//旋钮按键功能处理函数：显示和控制按键功能选择页面
void kpf_proc();

//电压测量处理函数：显示和控制电压测量页面
void volt_proc();

//设置处理函数：显示和控制系统设置页面
void setting_proc();

//USB设置处理函数：显示和控制USB存储配置页面
void usb_proc();

//关于页面处理函数：显示和控制关于本机页面
void about_proc();

//动画设置参数初始化：绑定动画设置页面的参数数组
void animition_param_init();

//动画设置处理函数：显示和控制动画参数页面
void animition_proc();

//UI总处理函数：根据当前状态调用对应的页面处理函数
void ui_proc();

//弹窗数值调节初始化：创建数值调节弹窗
void window_value_init(const char title[], uint8_t select, uint8_t *value, uint8_t max, uint8_t min, uint8_t step, Menu* bg, uint8_t index);

//弹窗消息初始化：创建消息提示弹窗
void window_message_init(const char title[], const char message[], Menu *bg, uint8_t index);

//UI参数初始化：从默认值初始化所有UI参数
void ui_param_init();

//UI初始化：初始化页面索引、状态、菜单数量、行数
void ui_init();

//画面消失动画处理：棋盘格或遮罩方式实现画面切换
void fade();

//弹窗显示函数：绘制弹窗所有UI元素
void window_show();

//弹窗处理函数：处理弹窗的输入和控制
void window_proc();

//弹窗参数初始化：设置弹窗的初始位置和尺寸
void window_param_init();

//旋钮参数初始化：绑定旋钮参数的数值显示
void knob_param_init();

//旋钮旋转功能初始化：绑定旋转功能单选框
void krf_param_init();

//旋钮按键功能初始化：绑定按键功能单选框
void kpf_param_init();

//设置页面初始化：绑定设置页面的参数和复选框
void setting_param_init();

//USB页面初始化：绑定USB页面的参数和复选框
void usb_param_init();

#endif