#include "led.h"
#include <Arduino.h>

/************************************* LED 裸寄存器 *************************************/

#define REG32(addr) (*(volatile uint32_t *)(addr))

#define RCC_BASE      0x40023800u
#define RCC_AHB1ENR   REG32(RCC_BASE + 0x30u)

#define GPIOA_BASE_RAW    0x40020000u
#define GPIOA_MODER   REG32(GPIOA_BASE_RAW + 0x00u)
#define GPIOA_OSPEEDR REG32(GPIOA_BASE_RAW + 0x08u)
#define GPIOA_BSRR    REG32(GPIOA_BASE_RAW + 0x18u)

/************************************* LED 全局变量 *************************************/

LedState led;
LedColor led_color;

/************************************* LED 初始化 *************************************/

void led_init() {
    // 使能 GPIOA 时钟
    RCC_AHB1ENR |= (1u << 0);
    
    // 配置 PA8、PA9、PA10 为通用输出模式
    uint32_t moder = GPIOA_MODER;
    moder &= ~((0x3u << 16) | (0x3u << 18) | (0x3u << 20));
    moder |= ((0x1u << 16) | (0x1u << 18) | (0x1u << 20));
    GPIOA_MODER = moder;
    
    // 配置为高速输出
    GPIOA_OSPEEDR |= ((0x3u << 16) | (0x3u << 18) | (0x3u << 20));
    
    // 初始化状态
    led.breathing = false;
    led.brightness = 255;
    led.direction = -1;
    led.last_update = millis();
    led.pwm_counter = 0;
    
    led_color.r = 255;
    led_color.g = 0;
    led_color.b = 0;
    
    // 上电显示红色
    led_set_red();
}

/************************************* 设置红色 *************************************/

void led_set_red() {
    led.breathing = false;
    led_color.r = 255;
    led_color.g = 0;
    led_color.b = 0;
    led_set_color(255, 0, 0);
}

/************************************* 设置白色 *************************************/

void led_set_white() {
    led.breathing = false;
    led_color.r = 255;
    led_color.g = 255;
    led_color.b = 255;
    led_set_color(255, 255, 255);
}

/************************************* 关闭 LED *************************************/

void led_off() {
    // 注意：不修改 led.breathing 状态，保持呼吸模式继续工作
    // 共阳极：高电平灭，使用 BSRR 低 16 位置位
    GPIOA_BSRR = (1u << 8) | (1u << 9) | (1u << 10);
}

/************************************* 完全关闭 LED *************************************/

void led_full_off() {
    led.breathing = false;
    // 共阳极：高电平灭，使用 BSRR 低 16 位置位
    GPIOA_BSRR = (1u << 8) | (1u << 9) | (1u << 10);
}

/************************************* 设置颜色 *************************************/

void led_set_color(uint8_t r, uint8_t g, uint8_t b) {
    // 共阳极：低电平亮（BSRR 高 16 位复位），高电平灭（BSRR 低 16 位置位）
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

void led_proc() {
    if (!led.breathing) {
        return;
    }
    
    // 快速更新 PWM 计数器
    led.pwm_counter++;
    if (led.pwm_counter >= 255) {
        led.pwm_counter = 0;
    }
    
    // PWM 输出
    if (led.pwm_counter < led.brightness) {
        led_set_color(led_color.r, led_color.g, led_color.b);
    } else {
        led_off();
    }
    
    // 缓慢调整亮度
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
