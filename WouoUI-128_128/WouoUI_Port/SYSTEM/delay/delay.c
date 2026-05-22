#include "delay.h"
#include "stm32f10x.h"

static volatile uint32_t sys_millis = 0;

void SysTick_Handler(void) {
    sys_millis++;
}

void delay_init(void) {
    /* SysTick: 72MHz/8 = 9MHz, 每1ms中断一次 */
    SystemCoreClock = 72000000;
    if (SysTick_Config(SystemCoreClock / 8000)) {
        while(1);
    }
    NVIC_SetPriority(SysTick_IRQn, 0x0F);
}

uint32_t millis(void) {
    uint32_t val;
    do { val = sys_millis; } while (val != sys_millis);
    return val;
}

void delay_us(uint32_t nus) {
    /* TIM2: 72MHz, 72分频 => 1MHz (1us/tick) */
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
    TIM2->PSC = 72 - 1;
    TIM2->ARR = 0xFFFF;
    TIM2->CR1 |= TIM_CR1_CEN;
    TIM2->CNT = 0;
    while (TIM2->CNT < nus);
    TIM2->CR1 &= ~TIM_CR1_CEN;
}

void delay_ms(uint16_t nms) {
    uint32_t start = millis();
    while (millis() - start < nms);
}
