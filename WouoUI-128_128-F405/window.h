#ifndef WINDOW_H
#define WINDOW_H

#include "ui_types.h"

/************************************* 弹窗相关 *************************************/

//弹窗状态全局变量：管理所有弹窗类型（数值调节/消息/列表选择/确认框）
extern WindowState win;

//数值调节弹窗初始化：创建带进度条的数值调节弹窗
void window_value_init(const char title[], uint8_t select, uint8_t *value, uint8_t max, uint8_t min, uint8_t step, Menu *bg, uint8_t index);
//消息弹窗初始化：创建纯文本消息提示弹窗
void window_message_init(const char title[], const char message[], Menu *bg, uint8_t index);
//列表选择弹窗初始化：创建可选择项的列表弹窗
void window_list_select_init(const char title[], const char* items[], uint8_t item_count, Menu *bg, uint8_t index);
//确认弹窗初始化：创建Yes/No确认对话框
void window_confirm_init(const char title[], const char message[], Menu *bg, uint8_t index, void (*on_close)(bool));
//弹窗参数初始化：重置弹窗位置、尺寸和模式标志
void window_param_init();
//设置列表选择弹窗关闭回调函数
void window_set_list_callback(void (*cb)(uint8_t));
//弹窗显示函数：绘制弹窗所有UI元素（背景、标题、内容、控件）
void window_show();
//弹窗处理函数：处理弹窗的输入事件和状态更新
void window_proc();

#endif