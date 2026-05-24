#ifndef LED_H
#define LED_H

#include "config.h"

/************************************* LED 控制 *************************************/

/*
 * LED状态结构体
 * 
 * 管理RGB LED的工作状态：
 * - 支持常亮模式和呼吸灯模式
 * - 呼吸模式使用软件PWM和亮度渐变
 * - 共阳极LED（低电平点亮）
 */
typedef struct {
    bool breathing;          // 呼吸灯模式标志（true=呼吸，false=常亮）
    uint8_t brightness;      // 当前亮度值（0-255，用于呼吸灯PWM）
    int8_t direction;        // 呼吸方向（+1=增亮，-1=变暗）
    uint32_t last_update;    // 上次亮度更新时间戳（ms）
    uint8_t pwm_counter;     // 软件PWM计数器（0-255循环）
} LedState;

/*
 * LED颜色结构体
 * 
 * 定义RGB三色通道的亮度值（0-255）
 * 0=最暗（共阳极时高电平），255=最亮（共阳极时低电平）
 */
typedef struct {
    uint8_t r;  //红色通道亮度
    uint8_t g;  //绿色通道亮度
    uint8_t b;  //蓝色通道亮度
} LedColor;

//LED状态全局变量
extern LedState led;
//LED颜色全局变量
extern LedColor led_color;

//LED GPIO初始化：配置PA8/PA9/PA10为推挽输出
void led_init();
//设置LED为红色（常亮模式）
void led_set_red();
//设置LED为白色（常亮模式）
void led_set_white();
//关闭LED（注意：不修改breathing状态，保持呼吸模式继续工作）
void led_off();
//强制关闭LED（完全关闭并退出呼吸模式）
void led_full_off();
//设置LED为指定颜色（0-255），共阳极控制
void led_set_color(uint8_t r, uint8_t g, uint8_t b);
//启动白色呼吸灯模式
void led_start_breathing_white();
//LED处理函数：需在主循环中周期性调用，实现呼吸效果
void led_proc();

#endif
