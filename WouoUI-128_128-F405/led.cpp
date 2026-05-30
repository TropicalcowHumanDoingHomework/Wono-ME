#include "led.h"
#include <Arduino.h>

/************************************* 寄存器地址宏 *************************************/

//32位寄存器读写宏
#define REG32(addr) (*(volatile uint32_t *)(addr))

//RCC（复位和时钟控制）外设基地址
#define RCC_BASE      0x40023800u
//AHB1外设时钟使能寄存器（控制GPIOA~GPIOI的时钟）
#define RCC_AHB1ENR   REG32(RCC_BASE + 0x30u)

//GPIOA外设基地址
#define GPIOA_BASE_RAW    0x40020000u
//GPIOA端口模式寄存器（配置引脚为输入/输出/复用/模拟）
#define GPIOA_MODER   REG32(GPIOA_BASE_RAW + 0x00u)
//GPIOA端口输出速度寄存器
#define GPIOA_OSPEEDR REG32(GPIOA_BASE_RAW + 0x08u)
//GPIOA端口置位/复位寄存器（低16位置位=输出高，高16位置位=输出低）
#define GPIOA_BSRR    REG32(GPIOA_BASE_RAW + 0x18u)

/************************************* RGB LED呼吸灯软件PWM驱动 *************************************/

/*
 * 本文件实现基于STM32F405裸寄存器操作的RGB LED控制：
 * 
 * 硬件连接：
 * - PA8 → 红色LED（共阳极，低电平点亮）
 * - PA9 → 绿色LED（共阳极，低电平点亮）
 * - PA10 → 蓝色LED（共阳极，低电平点亮）
 * 
 * 功能：
 * 1. 常亮模式：直接设置RGB通道为255或0
 * 2. 呼吸灯模式：使用软件PWM实现亮度渐变
 * 3. PWM原理：pwm_counter循环计数，与brightness比较决定亮灭
 * 
 * 呼吸灯时序：
 * - 快速PWM频率 = 主循环调用频率
 * - 亮度更新间隔 = 8ms
 * - 完整呼吸周期 ≈ 4秒
 */

/************************************* LED 全局变量 *************************************/

//LED状态结构体实例
LedState led;
//LED颜色结构体实例
LedColor led_color;

/************************************* LED 初始化 *************************************/

/*
 * LED GPIO初始化
 * 
 * 配置PA8（R）、PA9（G）、PA10（B）为通用推挽输出模式
 * 
 * 寄存器配置：
 * - RCC_AHB1ENR[0] = 1：使能GPIOA时钟
 * - GPIOA_MODER[16:21] = 0b010101：PA8、PA9、PA10设为通用输出
 * - GPIOA_OSPEEDR[16:21] = 0b111111：设为高速输出
 * 
 * 上电默认：红色常亮
 */
void led_init() {
    //使能GPIOA时钟（AHB1总线）
    RCC_AHB1ENR |= (1u << 0);
    
    //配置PA8、PA9、PA10为通用输出模式（MODER每两位控制一个引脚）
    //00=输入，01=输出，10=复用，11=模拟
    uint32_t moder = GPIOA_MODER;
    moder &= ~((0x3u << 16) | (0x3u << 18) | (0x3u << 20));
    moder |= ((0x1u << 16) | (0x1u << 18) | (0x1u << 20));
    GPIOA_MODER = moder;
    
    //配置为高速输出（OSPEEDR每两位控制一个引脚）
    GPIOA_OSPEEDR |= ((0x3u << 16) | (0x3u << 18) | (0x3u << 20));
    
    //初始化呼吸灯状态
    led.breathing = false;
    led.brightness = 255;
    led.direction = -1;
    led.last_update = millis();
    led.pwm_counter = 0;
    
    //上电显示红色
    led_color.r = 255;
    led_color.g = 0;
    led_color.b = 0;
    
    led_set_red();
}

/************************************* 设置红色 *************************************/

/*
 * 设置LED为红色（常亮模式）
 * 
 * 红色=R通道点亮（低电平），G和B通道关闭（高电平）
 * 同时退出呼吸灯模式
 */
void led_set_red() {
    led.breathing = false;
    led_color.r = 255;
    led_color.g = 0;
    led_color.b = 0;
    led_set_color(255, 0, 0);
}

/************************************* 设置绿色 *************************************/

void led_set_green() {
    led.breathing = false;
    led_color.r = 0;
    led_color.g = 255;
    led_color.b = 0;
    led_set_color(0, 255, 0);
}

/************************************* 设置白色 *************************************/

/*
 * 设置LED为白色（常亮模式）
 * 
 * 白色=RGB三通道同时点亮
 * 同时退出呼吸灯模式
 */
void led_set_white() {
    led.breathing = false;
    led_color.r = 255;
    led_color.g = 255;
    led_color.b = 255;
    led_set_color(255, 255, 255);
}

/************************************* 关闭 LED *************************************/

/*
 * 关闭LED（不修改breathing状态）
 * 
 * 共阳极LED控制原理：
 * - BSRR低16位置位对应引脚=输出高电平（LED灭）
 * - BSRR高16位置位对应引脚=输出低电平（LED亮）
 * 
 * PA8=R（红色），PA9=G（绿色），PA10=B（蓝色）
 */
void led_off() {
    GPIOA_BSRR = (1u << 8) | (1u << 9) | (1u << 10);
}

/************************************* 完全关闭 LED *************************************/

/*
 * 完全关闭LED
 * 
 * 与led_off()的区别：
 * - led_off()仅关闭输出，保留breathing状态
 * - led_full_off()同时退出呼吸灯模式
 */
void led_full_off() {
    led.breathing = false;
    GPIOA_BSRR = (1u << 8) | (1u << 9) | (1u << 10);
}

/************************************* 设置颜色 *************************************/

/*
 * 设置LED为指定颜色
 * 
 * 共阳极控制逻辑：
 * - BSRR低16位置位（BSx）= 输出高电平 → LED灭
 * - BSRR高16位置位（BRx）= 输出低电平 → LED亮
 * 
 * 参数：
 *   r - 红色通道（0=灭，255=亮）
 *   g - 绿色通道
 *   b - 蓝色通道
 * 
 * 注：由于是共阳极，设置255=低电平=LED亮
 *     BSRR[8+16]复位PA8(R) → 亮
 *     BSRR[8]置位PA8(R) → 灭
 */
void led_set_color(uint8_t r, uint8_t g, uint8_t b) {
    if (r == 255) {
        GPIOA_BSRR = (1u << (8 + 16));
    } else if (r == 0) {
        GPIOA_BSRR = (1u << 8);
    }
    
    if (g == 255) {
        GPIOA_BSRR = (1u << (9 + 16));
    } else if (g == 0) {
        GPIOA_BSRR = (1u << 9);
    }
    
    if (b == 255) {
        GPIOA_BSRR = (1u << (10 + 16));
    } else if (b == 0) {
        GPIOA_BSRR = (1u << 10);
    }
}

/************************************* 启动白色呼吸灯 *************************************/

/*
 * 启动白色呼吸灯模式
 * 
 * 呼吸灯原理：
 * 1. 使用软件PWM（pwm_counter从0-255循环）
 * 2. brightness由暗（0）渐变到亮（255）再渐变到暗
 * 3. PWM占空比 = brightness/255
 * 4. 亮度每8ms更新一次（direction控制增减方向）
 * 
 * 视觉效果：白色呼吸灯缓慢明暗变化
 */
void led_start_breathing_white() {
    led.breathing = true;
    led.brightness = 255;
    led.direction = -1;
    led.last_update = millis();
    led.pwm_counter = 0;
    led_color.r = 255;
    led_color.g = 255;
    led_color.b = 255;
}

/************************************* LED 处理函数 *************************************/

/*
 * LED处理函数（需在主循环中周期性调用）
 * 
 * 呼吸灯算法：
 * 1. 快速PWM循环：pwm_counter每调用一次+1
 * 2. 当pwm_counter < brightness时LED亮，否则LED灭
 * 3. 慢速亮度更新：每8ms调整一次brightness
 * 4. direction控制增减方向，到达边界时反转
 * 
 * 快速PWM频率 = 主循环频率
 * 慢速更新间隔 = 8ms
 * 完整呼吸周期 ≈ 255 * 2 * 8ms ≈ 4秒
 */
void led_proc() {
    if (!led.breathing) {
        return;
    }
    
    led.pwm_counter++;
    if (led.pwm_counter >= 255) {
        led.pwm_counter = 0;
    }
    
    if (led.pwm_counter < led.brightness) {
        led_set_color(led_color.r, led_color.g, led_color.b);
    } else {
        led_off();
    }
    
    uint32_t now = millis();
    if (now - led.last_update >= 8) {
        led.last_update = now;
        
        led.brightness += led.direction;
        
        if (led.brightness <= 0) {
            led.brightness = 0;
            led.direction = 1;
        } else if (led.brightness >= 255) {
            led.brightness = 255;
            led.direction = -1;
        }
    }
}
