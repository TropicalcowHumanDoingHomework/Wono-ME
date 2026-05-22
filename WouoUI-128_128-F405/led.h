#ifndef LED_H
#define LED_H

#include "config.h"

/************************************* LED 控制 *************************************/

//LED 状态
typedef struct {
    bool breathing;          // 是否在呼吸灯模式
    uint8_t brightness;      // 当前亮度（0-255）
    int8_t direction;        // 呼吸方向（+1增亮，-1变暗）
    uint32_t last_update;    // 上次更新时间
    uint8_t pwm_counter;     // PWM 计数器
} LedState;

//LED 颜色
typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} LedColor;

//全局变量声明
extern LedState led;
extern LedColor led_color;

//LED 初始化
void led_init();
//设置红色
void led_set_red();
//设置白色
void led_set_white();
//关闭 LED
void led_off();
//设置指定颜色（0-255）
void led_set_color(uint8_t r, uint8_t g, uint8_t b);
//启动白色呼吸灯
void led_start_breathing_white();
//LED 处理函数（需在主循环调用）
void led_proc();

#endif
