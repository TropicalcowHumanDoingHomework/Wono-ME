#ifndef KNOB_H
#define KNOB_H

#include "ui_types.h"

/************************************* 旋钮与蜂鸣器控制 *************************************/

/*
 * 全局按钮状态实例
 * 管理EC11编码器旋钮的旋转/按键事件及蜂鸣器ADSR包络控制
 * 
 * EC11编码器特性：
 * - 机械旋转编码器，A/B两相正交输出
 * - 内置按键SW（按下时低电平）
 * - 需要软件消抖处理
 */
extern ButtonState btn;

/*
 * EC11编码器旋转中断处理函数
 * 由AIO引脚上升沿触发外部中断
 * 通过A/B相正交信号判断旋转方向（CW顺时针/CC逆时针）
 * 产生btn.pressed=true和btn.id=CW/CC事件
 * 同时触发btn.buzzer_trig旋转提示音
 */
void knob_inter();

/*
 * 按键扫描函数（非中断，主循环中调用）
 * 使用三级状态机实现硬件消抖：
 * 原始采样 → 5ms稳定确认 → 事件触发
 * 自动区分短按(BTN_ID_SP)和长按(BTN_ID_LP)
 * 长按阈值由ui.param[BTN_LPT]决定
 */
void btn_scan();

/*
 * 旋钮引脚和蜂鸣器定时器初始化
 * - AIO/BIO/SW配置为GPIO上拉输入
 * - PB14配置为TIM12_CH1 PWM输出驱动蜂鸣器
 * - TIM12定时器配置为2.5kHz PWM
 * - 附加外部中断到AIO引脚（上升沿+下降沿）
 */
void btn_init();

/*
 * 蜂鸣器处理函数（主循环中调用）
 * 根据事件类型（旋转/确认/退出/开机）播放不同音效
 * 每个音效使用ADSR包络（起音→保持→释音）
 * 音量由ui.param[BUZ_VOL]控制（0=静音）
 */
void buzzer_proc();

/*
 * 触发退出提示音
 * 退出菜单层级时调用
 * 播放两个交替低音
 */
void buzzer_exit_sound();

/*
 * 触发开机提示音
 * 设备启动时调用
 * 播放三段递进音序（高→中→低）
 */
void buzzer_boot_sound();

#endif