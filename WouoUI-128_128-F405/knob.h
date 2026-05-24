#ifndef KNOB_H
#define KNOB_H

#include "ui_types.h"

/************************************* 旋钮相关 *************************************/

//按钮状态全局变量，管理EC11编码器旋钮的旋转/按键事件及蜂鸣器控制
extern ButtonState btn;

//EC11编码器旋转中断处理函数：通过A/B相信号判断旋转方向，产生CW/CC事件
void knob_inter();

//按键扫描函数：使用状态机实现SW引脚消抖，检测短按和长按事件
void btn_scan();

//按钮引脚和蜂鸣器定时器初始化：配置GPIO上拉输入和TIM12 PWM输出
void btn_init();

//蜂鸣器处理函数：在主循环中调用，根据事件类型播放不同音调（旋转/确认/退出/开机）
void buzzer_proc();

//触发退出提示音：退出菜单层级时调用
void buzzer_exit_sound();

//触发开机提示音：设备启动时调用（三段音序）
void buzzer_boot_sound();

#endif