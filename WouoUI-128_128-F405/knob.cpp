#include "knob.h"
#include "ui_state.h"

/************************************* EC11编码器旋钮驱动与蜂鸣器音效控制 *************************************/

/*
 * 本文件实现：
 * 1. EC11机械编码器的旋转方向检测（A/B相正交信号中断驱动）
 * 2. 按键SW的硬件消抖状态机（短按/长按区分）
 * 3. TIM12定时器PWM输出驱动蜂鸣器（裸寄存器操作）
 * 4. 多种音效的ADSR包络控制（旋转/确认/退出/开机）
 * 
 * 蜂鸣器硬件路径：
 * PB14(TIM12_CH1) → 有源蜂鸣器 → GND
 * 通过调节ARR控制频率，CCR1控制音量（占空比）
 */

#include "knob.h"

//32位寄存器地址访问宏：将物理地址转换为可读写指针
#define REG32(addr) (*(volatile uint32_t *)(addr))

//RCC时钟控制寄存器基地址和使能寄存器
#define RCC_BASE      0x40023800u
#define RCC_AHB1ENR   REG32(RCC_BASE + 0x30u)  //AHB1外设时钟使能
#define RCC_APB1ENR   REG32(RCC_BASE + 0x40u)  //APB1外设时钟使能

//GPIOB寄存器基地址和配置寄存器（PB14用于TIM12_CH1输出）
#define GPIOB_BASE_RAW    0x40020400u
#define GPIOB_MODER   REG32(GPIOB_BASE_RAW + 0x00u)  //GPIO模式寄存器
#define GPIOB_OSPEEDR REG32(GPIOB_BASE_RAW + 0x08u)  //GPIO速度寄存器
#define GPIOB_AFRH    REG32(GPIOB_BASE_RAW + 0x24u)  //GPIO复用功能高位寄存器

//TIM12定时器寄存器基地址和PWM相关寄存器
#define TIM12_BASE    0x40001800u
#define TIM12_CR1     REG32(TIM12_BASE + 0x00u)  //控制寄存器1
#define TIM12_CNT     REG32(TIM12_BASE + 0x24u)  //计数器值
#define TIM12_PSC     REG32(TIM12_BASE + 0x28u)  //预分频器
#define TIM12_ARR     REG32(TIM12_BASE + 0x2Cu)  //自动重装载值
#define TIM12_CCR1    REG32(TIM12_BASE + 0x34u)  //捕获/比较值（占空比）
#define TIM12_CCMR1   REG32(TIM12_BASE + 0x18u)  //捕获/比较模式寄存器
#define TIM12_CCER    REG32(TIM12_BASE + 0x20u)  //捕获/比较使能寄存器

/************************************* 旋钮相关 *************************************/

//按钮状态全局实例
ButtonState btn;

/*
 * EC11编码器旋转中断处理函数（由AIO引脚上升沿触发）
 * 
 * 编码器工作原理：
 * - A相（AIO）连接到外部中断，上升沿触发
 * - 在A相上升沿采样B相（BIO）电平判断方向
 * - 使用flag状态机避免重复触发
 * 
 * 方向判断：
 * - CW_1 && CW_2 = 顺时针（A上升沿时B=高，然后回到低）
 * - !CW_1 && !CW_2 = 逆时针（A上升沿时B=低，然后回到高）
 */
void knob_inter() {
    btn.alv = digitalRead(AIO);  //读取A相当前电平
    btn.blv = digitalRead(BIO);  //读取B相当前电平
    if (!btn.flag && btn.alv == LOW) {
        btn.CW_1 = btn.blv;      //记录B相在A相下降沿时的状态
        btn.flag = true;          //设置状态机标志，防止重复触发
    }
    if (btn.flag && btn.alv) {
        btn.CW_2 = !btn.blv;     //记录B相在A相上升沿时的状态
        if (btn.CW_1 && btn.CW_2) {
            btn.id = ui.param[KNOB_DIR];  //顺时针（方向由KNOB_DIR参数决定）
            btn.pressed = true;
            btn.buzzer_trig = true;       //触发旋转提示音
        }
        if (btn.CW_1 == false && btn.CW_2 == false) {
            btn.id = !ui.param[KNOB_DIR]; //逆时针
            btn.pressed = true;
            btn.buzzer_trig = true;
        }
        btn.flag = false;  //复位状态机
    }
}

/*
 * 按键扫描函数（非中断，在主循环中调用）
 * 
 * 使用三级消抖状态机（原始采样→稳定状态→事件触发）：
 * 1. 读取SW引脚电平
 * 2. 连续5ms稳定后才确认电平变化
 * 3. 检测按下时间，区分短按和长按事件
 * 
 * 短按：按下后抬起且持续时间小于长按阈值
 * 长按：按下持续时间超过BTN_LPT阈值，产生一次长按事件
 */
void btn_scan() {
    static uint8_t  s_last_sample = 1;  //上次采样值
    static uint8_t  s_stable      = 1;  //稳定后的电平值
    static uint32_t s_change_ms   = 0;  //上次电平变化的时间戳
    static uint32_t s_press_ms    = 0;  //按键按下的时间戳
    static bool     s_long_done   = false;  //长按事件是否已触发

    uint8_t raw = digitalRead(SW);  //读取SW引脚（低电平=按下）
    uint32_t now = millis();

    //检测电平变化
    if (raw != s_last_sample) {
        s_last_sample = raw;
        s_change_ms = now;  //记录变化时间
    }

    //连续5ms稳定才确认电平变化（硬件消抖）
    if ((uint32_t)(now - s_change_ms) >= 5u) {
        if (s_stable != s_last_sample) {
            s_stable = s_last_sample;
            if (s_stable == LOW) {
                s_press_ms = now;
                s_long_done = false;
            } else {
                if (!s_long_done) {
                    btn.pressed = true;
                    btn.id = BTN_ID_SP;      //短按事件
                    btn.buzzer_confirm = true; //确认提示音
                }
            }
        }
    }

    //检测长按（按下时间超过BTN_LPT阈值）
    if (s_stable == LOW && !s_long_done &&
        (uint32_t)(now - s_press_ms) >= (uint32_t)(ui.param[BTN_LPT] * 2u)) {
        s_long_done = true;
        btn.pressed = true;
        btn.id = BTN_ID_LP;  //长按事件
    }
}

/*
 * 按钮引脚和蜂鸣器定时器初始化
 * 
 * 初始化内容：
 * 1. AIO/BIO/SW配置为GPIO上拉输入（EC11编码器内部无上拉）
 * 2. PB14配置为TIM12_CH1的PWM输出
 * 3. TIM12定时器配置：84MHz/84=1MHz→PSC=83，ARR=399→约2.5kHz
 * 4. 附加中断到AIO引脚的上升沿和下降沿
 */
void btn_init() {
    //编码器引脚配置为上拉输入（AIO=PC15, BIO=PC13, SW=PC14）
    pinMode(AIO, INPUT_PULLUP);
    pinMode(BIO, INPUT_PULLUP);
    pinMode(SW, INPUT_PULLUP);

    //PB14配置为TIM12_CH1的复用功能（AF9）
    RCC_AHB1ENR |= (1u << 1);  //使能GPIOB时钟
    uint32_t tmp = GPIOB_MODER;
    tmp &= ~(0x3u << 28);      //清除PB14模式位
    tmp |= (0x2u << 28);       //设置为复用功能模式
    GPIOB_MODER = tmp;
    GPIOB_OSPEEDR |= (0x3u << 28);  //高速输出
    tmp = GPIOB_AFRH;
    tmp &= ~(0xFu << 24);      //清除PB14复用功能选择
    tmp |= (9u << 24);         //AF9=TIM12_CH1
    GPIOB_AFRH = tmp;

    //TIM12定时器PWM配置
    RCC_APB1ENR |= (1u << 6); //使能TIM12时钟
    TIM12_PSC = 83;            //预分频：84MHz/(83+1)=1MHz
    TIM12_ARR = 399;           //PWM周期：1MHz/(399+1)=2.5kHz
    TIM12_CCR1 = 0;            //初始占空比0（静音）
    tmp = TIM12_CCMR1;
    tmp &= ~(0x7u << 4);       //清除OC1M位
    tmp |= (6u << 4);          //PWM模式1（向上计数时OC1低电平）
    TIM12_CCMR1 = tmp;
    TIM12_CCER |= (1u << 0);   //使能CH1输出
    TIM12_CR1 |= (1u << 7);    //使能自动重装载预装载

    //附加AIO中断（上升沿和下降沿触发）
    attachInterrupt(digitalPinToInterrupt(AIO), knob_inter, CHANGE);
}

//触发退出提示音
void buzzer_exit_sound() {
    btn.buzzer_exit = true;
}

//触发开机提示音
void buzzer_boot_sound() {
    btn.buzzer_boot = true;
}

/*
 * 蜂鸣器处理函数（在主循环中调用）
 * 
 * 支持四种音效类型，每种包含ADSR包络：
 * - 旋转音：单音渐入渐出，约120ms
 * - 确认音：单音，音调稍高，约80ms
 * - 退出音：两个低音交替，约220ms
 * - 开机音：三个递进音调，约430ms
 * 
 * 音量通过BUZ_VOL参数（0-4）控制ARR和CCR1的值
 * 通过定时器PWM输出控制蜂鸣器频率和占空比
 */
void buzzer_proc() {
    static bool  s_is_confirm = false;  //是否为确认音
    static bool  s_is_exit = false;     //是否为退出音
    static bool  s_is_boot = false;     //是否为开机音
    static uint8_t s_exit_phase = 0;    //退出音阶段
    static uint8_t s_boot_phase = 0;    //开机音阶段
    
    //音量等级对应的ARR和CCR值（频率和占空比）
    static const uint32_t k_rot_arr[5] = {0, 1199, 799, 532, 399};  //各音量等级ARR
    static const uint32_t k_rot_ccr[5] = {0, 360, 320, 266, 260};   //各音量等级CCR
    
    //音效时间参数（毫秒）
    const uint32_t rot_total_ms   = 120;   //旋转音总时长
    const uint32_t rot_attack_ms  = 10;    //旋转音起音时间
    const uint32_t rot_release_ms = 20;    //旋转音释音时间
    const uint32_t cnf_total_ms   = 80;    //确认音总时长
    const uint32_t cnf_attack_ms  = 3;     //确认音起音时间
    const uint32_t cnf_release_ms = 15;    //确认音释音时间
    const uint32_t exit_single_ms = 80;    //退出音每段时长
    const uint32_t exit_gap_ms    = 60;    //退出音段间间隔
    const uint32_t exit_attack_ms = 3;     //退出音起音时间
    const uint32_t exit_release_ms= 15;    //退出音释音时间
    const uint32_t boot_single_ms = 110;   //开机音每段时长
    const uint32_t boot_gap_ms    = 50;    //开机音段间间隔
    const uint32_t boot_attack_ms = 5;     //开机音起音时间
    const uint32_t boot_release_ms= 20;    //开机音释音时间

    //处理开机音请求（优先级最高，覆盖其他音效）
    if (btn.buzzer_boot) {
        s_is_boot = true;
        s_boot_phase = 0;
        s_is_exit = false;
        s_is_confirm = false;
        btn.buzzer_boot = false;
        uint8_t vol = ui.param[BUZ_VOL];
        if (vol == 0) return;
        uint32_t arr = k_rot_arr[vol] * 3 / 2;  //开机音较高音调
        TIM12_CNT = 0;
        TIM12_ARR = arr;
        TIM12_CCR1 = 0;
        btn.buzzer_start = millis();
        TIM12_CR1 |= (1u << 0);  //使能定时器
    }

    //处理退出音请求
    if (btn.buzzer_exit) {
        s_is_exit = true;
        s_exit_phase = 0;
        s_is_boot = false;
        btn.buzzer_exit = false;
        uint8_t vol = ui.param[BUZ_VOL];
        if (vol == 0) return;
        uint32_t arr = k_rot_arr[vol] * 2 / 3;  //退出音较低音调
        TIM12_CNT = 0;
        TIM12_ARR = arr;
        TIM12_CCR1 = 0;
        btn.buzzer_start = millis();
        TIM12_CR1 |= (1u << 0);
    }

    //处理旋转音和确认音请求
    if (btn.buzzer_trig || btn.buzzer_confirm) {
        s_is_confirm = btn.buzzer_confirm;
        s_is_boot = false;
        s_is_exit = false;
        btn.buzzer_trig = false;
        btn.buzzer_confirm = false;
        uint8_t vol = ui.param[BUZ_VOL];
        if (vol == 0) return;
        uint32_t arr = k_rot_arr[vol];
        if (s_is_confirm) {
            arr = k_rot_arr[vol] * 3 / 2;  //确认音较高音调
        }
        TIM12_CNT = 0;
        TIM12_ARR = arr;
        TIM12_CCR1 = 0;
        btn.buzzer_start = millis();
        TIM12_CR1 |= (1u << 0);
    }

    //无正在播放的音效
    if (btn.buzzer_start == 0) return;

    //根据音效类型确定总时长、起音时间和释音时间
    uint32_t total_ms   = s_is_confirm ? cnf_total_ms  : (s_is_exit ? (exit_single_ms + exit_gap_ms + exit_single_ms) : (s_is_boot ? (boot_single_ms * 3 + boot_gap_ms * 2) : rot_total_ms));
    uint32_t attack_ms  = s_is_confirm ? cnf_attack_ms : (s_is_exit ? exit_attack_ms : (s_is_boot ? boot_attack_ms : rot_attack_ms));
    uint32_t release_ms = s_is_confirm ? cnf_release_ms : (s_is_exit ? exit_release_ms : (s_is_boot ? boot_release_ms : rot_release_ms));
    uint32_t elapsed_ms = (uint32_t)(millis() - btn.buzzer_start);

    //开机音（三段音序：高-中-低）
    if (s_is_boot) {
        uint8_t vol = ui.param[BUZ_VOL];
        uint32_t target_ccr = k_rot_ccr[vol];

        if (s_boot_phase == 0) {
            if (elapsed_ms < boot_single_ms) {
                uint32_t current_ccr = target_ccr;
                if (elapsed_ms < attack_ms)
                    current_ccr = current_ccr * elapsed_ms / attack_ms;
                else if (elapsed_ms > boot_single_ms - release_ms)
                    current_ccr = current_ccr * (boot_single_ms - elapsed_ms) / release_ms;
                TIM12_CCR1 = current_ccr;
            } else {
                s_boot_phase = 1;
                TIM12_CCR1 = 0;
            }
        } else if (s_boot_phase == 1) {
            if (elapsed_ms < boot_single_ms + boot_gap_ms) {
                TIM12_CCR1 = 0;
            } else {
                s_boot_phase = 2;
                TIM12_ARR = k_rot_arr[vol];
                TIM12_CNT = 0;
            }
        } else if (s_boot_phase == 2) {
            uint32_t phase_elapsed = elapsed_ms - (boot_single_ms + boot_gap_ms);
            if (phase_elapsed < boot_single_ms) {
                uint32_t current_ccr = target_ccr;
                if (phase_elapsed < attack_ms)
                    current_ccr = current_ccr * phase_elapsed / attack_ms;
                else if (phase_elapsed > boot_single_ms - release_ms)
                    current_ccr = current_ccr * (boot_single_ms - phase_elapsed) / release_ms;
                TIM12_CCR1 = current_ccr;
            } else {
                s_boot_phase = 3;
                TIM12_CCR1 = 0;
            }
        } else if (s_boot_phase == 3) {
            if (elapsed_ms < boot_single_ms * 2 + boot_gap_ms) {
                TIM12_CCR1 = 0;
            } else {
                s_boot_phase = 4;
                TIM12_ARR = k_rot_arr[vol] * 2 / 3;
                TIM12_CNT = 0;
            }
        } else if (s_boot_phase == 4) {
            uint32_t phase_elapsed = elapsed_ms - (boot_single_ms * 2 + boot_gap_ms * 2);
            if (phase_elapsed < boot_single_ms) {
                uint32_t current_ccr = target_ccr;
                if (phase_elapsed < attack_ms)
                    current_ccr = current_ccr * phase_elapsed / attack_ms;
                else if (phase_elapsed > boot_single_ms - release_ms)
                    current_ccr = current_ccr * (boot_single_ms - phase_elapsed) / release_ms;
                TIM12_CCR1 = current_ccr;
            } else {
                TIM12_CCR1 = 0;
                TIM12_CR1 &= ~(1u << 0);
                btn.buzzer_start = 0;
                s_is_boot = false;
                s_boot_phase = 0;
            }
        }
        return;
    }

    if (s_is_exit) {
        uint8_t vol = ui.param[BUZ_VOL];
        uint32_t target_ccr = k_rot_ccr[vol];
        uint32_t arr = k_rot_arr[vol] * 2 / 3;

        if (s_exit_phase == 0) {
            if (elapsed_ms < exit_single_ms) {
                uint32_t current_ccr = target_ccr;
                if (elapsed_ms < attack_ms)
                    current_ccr = current_ccr * elapsed_ms / attack_ms;
                else if (elapsed_ms > exit_single_ms - release_ms)
                    current_ccr = current_ccr * (exit_single_ms - elapsed_ms) / release_ms;
                TIM12_CCR1 = current_ccr;
            } else {
                s_exit_phase = 1;
                TIM12_CCR1 = 0;
            }
        } else if (s_exit_phase == 1) {
            if (elapsed_ms < exit_single_ms + exit_gap_ms) {
                TIM12_CCR1 = 0;
            } else {
                s_exit_phase = 2;
                TIM12_ARR = arr;
            }
        } else if (s_exit_phase == 2) {
            uint32_t phase_elapsed = elapsed_ms - (exit_single_ms + exit_gap_ms);
            if (phase_elapsed < exit_single_ms) {
                uint32_t current_ccr = target_ccr;
                if (phase_elapsed < attack_ms)
                    current_ccr = current_ccr * phase_elapsed / attack_ms;
                else if (phase_elapsed > exit_single_ms - release_ms)
                    current_ccr = current_ccr * (exit_single_ms - phase_elapsed) / release_ms;
                TIM12_CCR1 = current_ccr;
            } else {
                TIM12_CCR1 = 0;
                TIM12_CR1 &= ~(1u << 0);
                btn.buzzer_start = 0;
                s_is_exit = false;
                s_exit_phase = 0;
            }
        }
        return;
    }

    if (elapsed_ms >= total_ms) {
        TIM12_CCR1 = 0;
        TIM12_CR1 &= ~(1u << 0);
        btn.buzzer_start = 0;
        return;
    }

    uint8_t vol = ui.param[BUZ_VOL];
    uint32_t target_ccr = k_rot_ccr[vol];

    uint32_t current_ccr = target_ccr;
    if (elapsed_ms < attack_ms)
        current_ccr = current_ccr * elapsed_ms / attack_ms;
    else if (elapsed_ms > total_ms - release_ms)
        current_ccr = current_ccr * (total_ms - elapsed_ms) / release_ms;

    TIM12_CCR1 = current_ccr;
}
