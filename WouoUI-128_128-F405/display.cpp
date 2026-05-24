
#include "display.h"
#include "ui_state.h"
#include <string.h>

/************************************* SPI3显示驱动与VCOM翻转控制 *************************************/

/*
 * 本文件实现基于STM32F405裸寄存器操作的SPI3驱动
 * 用于驱动夏普LS013B7DH03 128x128 Memory LCD屏幕
 * 
 * 核心功能：
 * 1. 裸寄存器SPI3初始化与字节收发
 * 2. U8g2库回调替换（u8x8_byte_spi3_hw）
 * 3. VCOM极性自动翻转（防止屏幕灼伤）
 * 4. 双对比度模式（高/低亮度）的VCOM逻辑切换
 * 
 * Memory LCD与TFT LCD关键差异：
 * - 无需持续刷新，静态时几乎不耗电
 * - 写入时必须通过VCOM命令（0x80/0xC0）设置极性
 * - VCOM不翻转会导致直流偏置，永久损坏屏幕
 * - 每帧必须切换VCOM极性
 */

#include "display.h"

//寄存器地址访问宏，将绝对地址转换为32位指针进行读写
#define REG32(addr) (*(volatile uint32_t *)(addr))

//RCC（Reset and Clock Control）外设基地址及寄存器偏移
#define RCC_BASE        0x40023800u
#define RCC_AHB1ENR     REG32(RCC_BASE + 0x30u)  //AHB1外设时钟使能寄存器
#define RCC_APB1ENR     REG32(RCC_BASE + 0x40u)  //APB1外设时钟使能寄存器

//GPIOC外设基地址及寄存器偏移
#define GPIOC_BASE_RAW  0x40020800u
#define GPIO_MODER(b)   REG32((b) + 0x00u)  //端口模式寄存器（输入/输出/复用/模拟）
#define GPIO_OTYPER(b)  REG32((b) + 0x04u)  //端口输出类型寄存器（推挽/开漏）
#define GPIO_OSPEEDR(b) REG32((b) + 0x08u)  //端口输出速度寄存器
#define GPIO_PUPDR(b)   REG32((b) + 0x0Cu)  //端口上拉/下拉寄存器
#define GPIO_BSRR(b)    REG32((b) + 0x18u)  //端口位设置/复位寄存器
#define GPIO_AFRL(b)    REG32((b) + 0x20u)  //复用功能低位寄存器（引脚0-7）
#define GPIO_AFRH(b)    REG32((b) + 0x24u)  //复用功能高位寄存器（引脚8-15）

//SPI3外设基地址及寄存器偏移
#define SPI3_BASE_RAW   0x40003C00u
#define SPI_CR1(b)      REG32((b) + 0x00u)  //SPI控制寄存器1
#define SPI_SR(b)       REG32((b) + 0x08u)  //SPI状态寄存器
#define SPI_DR8(b)      (*(volatile uint8_t *)((b) + 0x0Cu))  //SPI数据寄存器（8位）

//配置GPIO引脚为推挽输出模式，用于CS和DISP控制信号
static void reg_gpio_output_pp(uint32_t gpio, uint8_t pin) {
    uint32_t s2 = (uint32_t)pin * 2u;
    GPIO_MODER(gpio) &= ~(0x3u << s2);  //清除模式位
    GPIO_MODER(gpio) |= (0x1u << s2);   //设置为通用输出模式
    GPIO_OTYPER(gpio) &= ~(1u << pin);   //推挽输出
    GPIO_OSPEEDR(gpio) &= ~(0x3u << s2); //清除速度位
    GPIO_OSPEEDR(gpio) |= (0x2u << s2);  //设置为高速输出
    GPIO_PUPDR(gpio) &= ~(0x3u << s2);   //无上拉/下拉
}

//配置GPIO引脚为复用功能模式，用于SPI3的SCK和MOSI信号
static void reg_gpio_set_af(uint32_t gpio, uint8_t pin, uint8_t af) {
    uint32_t s2 = (uint32_t)pin * 2u;
    uint32_t s4;
    GPIO_MODER(gpio) &= ~(0x3u << s2);  //清除模式位
    GPIO_MODER(gpio) |= (0x2u << s2);   //设置为复用功能模式
    GPIO_OTYPER(gpio) &= ~(1u << pin);   //推挽输出
    GPIO_OSPEEDR(gpio) &= ~(0x3u << s2); //清除速度位
    GPIO_OSPEEDR(gpio) |= (0x2u << s2);  //设置为高速输出
    GPIO_PUPDR(gpio) &= ~(0x3u << s2);   //无上拉/下拉
    //根据引脚号选择AFRL或AFRH寄存器，设置复用功能编号
    if (pin < 8u) {
        s4 = (uint32_t)pin * 4u;
        GPIO_AFRL(gpio) &= ~(0xFu << s4);
        GPIO_AFRL(gpio) |= ((uint32_t)af << s4);
    } else {
        s4 = ((uint32_t)pin - 8u) * 4u;
        GPIO_AFRH(gpio) &= ~(0xFu << s4);
        GPIO_AFRH(gpio) |= ((uint32_t)af << s4);
    }
}

//通过BSRR寄存器快速设置或清除GPIO引脚输出电平
static void reg_gpio_write(uint32_t gpio, uint8_t pin, uint8_t high) {
    //BSRR低16位用于置位，高16位用于复位
    if (high) GPIO_BSRR(gpio) = (1u << pin);
    else      GPIO_BSRR(gpio) = (1u << (pin + 16u));
}

//通过SPI3数据寄存器发送多个字节数据
static void spi3_send_bytes(const uint8_t *data, uint16_t len) {
    uint16_t i;
    volatile uint32_t v;
    //如果接收缓冲器非空或有溢出错误，先清除
    if ((SPI_SR(SPI3_BASE_RAW) & ((1u << 0) | (1u << 6))) != 0u) {
        v = SPI_DR8(SPI3_BASE_RAW);
        v = SPI_SR(SPI3_BASE_RAW);
        (void)v;
    }
    //循环发送每个字节：等待发送缓冲区空→写入数据→等待接收缓冲区非空→读取清除
    for (i = 0u; i < len; ++i) {
        while ((SPI_SR(SPI3_BASE_RAW) & (1u << 1)) == 0u) {}  //等待TXE（发送缓冲区空）
        SPI_DR8(SPI3_BASE_RAW) = data[i];                       //写入发送数据
        while ((SPI_SR(SPI3_BASE_RAW) & (1u << 0)) == 0u) {}  //等待RXNE（接收缓冲区非空）
        v = SPI_DR8(SPI3_BASE_RAW);                             //读取清除RXNE标志
        (void)v;
    }
    //等待BSY标志清除，确保发送完成
    while ((SPI_SR(SPI3_BASE_RAW) & (1u << 7)) != 0u) {}
    //检查是否有溢出错误，如有则清除
    if ((SPI_SR(SPI3_BASE_RAW) & (1u << 6)) != 0u) {
        v = SPI_DR8(SPI3_BASE_RAW);
        v = SPI_SR(SPI3_BASE_RAW);
        (void)v;
    }
}

//初始化SPI3外设的裸寄存器配置
static void spi3_init_raw() {
    RCC_AHB1ENR |= (1u << 2);   //使能GPIOC时钟
    RCC_APB1ENR |= (1u << 15);  //使能SPI3时钟
    reg_gpio_set_af(GPIOC_BASE_RAW, 10u, 6u);  //PC10=SPI3_SCK，复用功能6
    reg_gpio_set_af(GPIOC_BASE_RAW, 12u, 6u);  //PC12=SPI3_MOSI，复用功能6
    SPI_CR1(SPI3_BASE_RAW) = 0u;  //先清零所有配置
    //BR=100=fPCLK/32≈1.31MHz (LS013B7DH03最高2MHz,U8g2默认1MHz)
    //配置：SSM=1(软件从机管理) | BR=100(分频32) | LSBFIRST=0(MSB先行) | MSTR=1(主机模式) | SPE=1(使能)
    SPI_CR1(SPI3_BASE_RAW) = (1u << 2) | (4u << 3) | (1u << 9) | (1u << 8) | (1u << 6);
}

/************************************* U8g2字节回调（裸SPI3，含VCOM翻转） *************************************/

/*
 * 夏普LS013B7DH03 Memory LCD的特殊性：
 * 1. 需要周期性翻转VCOM信号来防止屏幕灼伤，每帧需要交替VCOM极性
 * 2. 通过命令字节的bit6（0x40）控制VCOM：0x80=VCOM=0, 0xC0=VCOM=1
 * 3. 屏幕没有传统LCD的数据/命令区分，DC引脚在此用作DISP（显示使能）
 * 4. 低对比度模式（DISP_BRI=0）使用不同的VCOM翻转逻辑，产生较浅的显示效果
 */

//VCOM翻转状态和帧计数，LS013B7DH03需要每帧交替VCOM极性
static uint8_t  s_vcom_toggle;          //VCOM极性翻转标志：0或1
static uint8_t  s_frame_page_cnt;       //基于页面的帧计数器（16页=1整屏）
static uint16_t s_vcom_frame_cnt;       //基于tile的帧计数器（256 tile=1整屏），用于低对比度模式
static uint8_t  s_transfer_is_update;   //标记当前传输是否为有效的画面更新
static uint8_t  s_dc_is_data;           //标记DC引脚状态：0=命令，1=数据

extern "C" uint8_t u8x8_byte_spi3_hw(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr) {
    static uint8_t buf[256];  //临时缓冲区，用于修改命令字节（插入VCOM翻转）
    switch(msg) {
        //处理U8g2的字节发送消息：将数据通过SPI3发送到屏幕
        case U8X8_MSG_BYTE_SEND: {
            uint8_t *src = (uint8_t *)arg_ptr;
            if (arg_int == 0u) break;
            
            if (ui.param[DISP_BRI] >= 1u) {
                //高对比度模式：使用基于页面计数的VCOM翻转（原工程逻辑）
                if (s_dc_is_data) {
                    //数据字节直接发送，不修改VCOM
                    spi3_send_bytes(src, (uint16_t)arg_int);
                } else if (src[0] == 0x80u || src[0] == 0xC0u) {
                    //命令字节：如果是VCOM命令（0x80或0xC0），修改VCOM极性
                    s_frame_page_cnt++;
                    if (s_frame_page_cnt >= 16u) {
                        s_frame_page_cnt = 0u;
                        s_vcom_toggle ^= 1u;  //每16页翻转一次VCOM
                    }
                    //根据当前VCOM状态替换命令字节
                    if (s_vcom_toggle) {
                        buf[0] = 0xC0u;
                    } else {
                        buf[0] = 0x80u;
                    }
                    if (arg_int > 1u) memcpy(buf + 1, src + 1, arg_int - 1u);
                    s_transfer_is_update = 1u;
                    spi3_send_bytes(buf, (uint16_t)arg_int);
                } else {
                    //其他命令直接发送
                    spi3_send_bytes(src, (uint16_t)arg_int);
                }
            } else {
                //低对比度模式：使用基于tile计数的VCOM翻转（桌面代码逻辑）
                if (arg_int > 0 && src[0] == 0x80u && s_vcom_toggle) {
                    buf[0] = 0xC0u;
                    if (arg_int > 1u) memcpy(buf + 1, src + 1, arg_int - 1u);
                    spi3_send_bytes(buf, (uint16_t)arg_int);
                } else {
                    spi3_send_bytes(src, (uint16_t)arg_int);
                }
            }
            break;
        }

        //初始化SPI字节传输通道：复位所有状态变量
        case U8X8_MSG_BYTE_INIT:
            s_vcom_toggle = 0u;
            s_frame_page_cnt = 0u;
            s_vcom_frame_cnt = 0u;
            s_transfer_is_update = 0u;
            s_dc_is_data = 0u;
            if (u8x8->bus_clock == 0)
                u8x8->bus_clock = u8x8->display_info->sck_clock_hz;
            u8x8_gpio_SetCS(u8x8, u8x8->display_info->chip_disable_level);  //CS置为无效电平
            u8x8_gpio_SetDC(u8x8, 1);  //DC置高
            break;

        //设置DC引脚电平：arg_int非零表示数据模式，零表示命令模式
        case U8X8_MSG_BYTE_SET_DC:
            s_dc_is_data = (arg_int != 0u);
            if (ui.param[DISP_BRI] >= 1u) {
                //高对比度模式：DC始终为高（DISP使能）
                u8x8_gpio_SetDC(u8x8, 1);
            } else {
                //低对比度模式：DC跟随arg_int
                u8x8_gpio_SetDC(u8x8, arg_int);
            }
            break;

        //开始传输：置低CS使能屏幕通信
        case U8X8_MSG_BYTE_START_TRANSFER:
            if (ui.param[DISP_BRI] >= 1u) {
                s_transfer_is_update = 0u;
            } else {
                //低对比度模式：每256个tile翻转一次VCOM
                s_vcom_frame_cnt++;
                if (s_vcom_frame_cnt >= 256u) {
                    s_vcom_frame_cnt = 0u;
                    s_vcom_toggle ^= 1u;
                }
            }
            u8x8_gpio_SetCS(u8x8, u8x8->display_info->chip_enable_level);  //CS置低使能
            u8x8->gpio_and_delay_cb(u8x8, U8X8_MSG_DELAY_NANO, u8x8->display_info->post_chip_enable_wait_ns, NULL);
            break;

        //结束传输：置高CS禁用屏幕通信
        case U8X8_MSG_BYTE_END_TRANSFER:
            u8x8->gpio_and_delay_cb(u8x8, U8X8_MSG_DELAY_NANO, u8x8->display_info->pre_chip_disable_wait_ns, NULL);
            u8x8_gpio_SetCS(u8x8, u8x8->display_info->chip_disable_level);  //CS置高禁用
            break;

        default:
            return 0;
    }
    return 1;
}

/************************************* 屏幕变量定义 *************************************/

//U8g2屏幕对象实例化：使用U8G2_R0（正常方向），软件SPI引脚（SCL, SDA, CS, DC, RES）
U8G2_LS013B7DH03_128X128_F_4W_SW_SPI u8g2(U8G2_R0, SCL, SDA, CS, DC, RES);

//屏幕帧缓冲区指针，指向U8g2内部的显存区域
uint8_t *buf_ptr;

//屏幕帧缓冲区字节长度，用于直接操作显存
uint16_t buf_len;

/************************************* 显示初始化 *************************************/

/*
 * LCD初始化流程：
 * 1. 先配置CS和DISP引脚为推挽输出，确保初始电平正确
 * 2. 初始化SPI3硬件
 * 3. 替换U8g2的字节发送回调为自定义的硬件SPI3回调
 * 4. 调用U8g2的begin()初始化
 * 5. 恢复SPI3配置（begin()可能覆盖了引脚复用设置）
 * 6. 设置对比度和获取缓冲区信息
 */
void lcd_init() {
    //CS=PC4, DISP=PC5 先行配置为推挽输出
    reg_gpio_output_pp(GPIOC_BASE_RAW, 4u);
    reg_gpio_output_pp(GPIOC_BASE_RAW, 5u);
    reg_gpio_write(GPIOC_BASE_RAW, 4u, 0u);  //CS置低
    reg_gpio_write(GPIOC_BASE_RAW, 5u, 1u);  //DISP置高（显示使能）
    //先启动SPI3，让u8g2.begin()内的ALL_CLEAR能通过SPI发送
    spi3_init_raw();
    u8g2.getU8x8()->byte_cb = u8x8_byte_spi3_hw;  //替换U8g2的字节发送回调
    u8g2.begin();
    //u8g2.begin()的pinMode会覆盖PC10/PC12的AF6为GPIO_OUTPUT，必须恢复SPI3配置
    spi3_init_raw();
    reg_gpio_write(GPIOC_BASE_RAW, 5u, 1u);  //确保DISP使能
    u8g2.setContrast(ui.param[DISP_BRI]);  //设置初始对比度
    buf_ptr = u8g2.getBufferPtr();        //获取帧缓冲区指针
    buf_len = 8 * u8g2.getBufferTileHeight() * u8g2.getBufferTileWidth();  //计算缓冲区长度
}
