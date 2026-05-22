
#include "display.h"
#include "ui_state.h"
#include <string.h>

/************************************* 裸寄存器SPI3驱动（参考F405工程） *************************************/

#define REG32(addr) (*(volatile uint32_t *)(addr))

#define RCC_BASE        0x40023800u
#define RCC_AHB1ENR     REG32(RCC_BASE + 0x30u)
#define RCC_APB1ENR     REG32(RCC_BASE + 0x40u)

#define GPIOC_BASE_RAW  0x40020800u
#define GPIO_MODER(b)   REG32((b) + 0x00u)
#define GPIO_OTYPER(b)  REG32((b) + 0x04u)
#define GPIO_OSPEEDR(b) REG32((b) + 0x08u)
#define GPIO_PUPDR(b)   REG32((b) + 0x0Cu)
#define GPIO_BSRR(b)    REG32((b) + 0x18u)
#define GPIO_AFRL(b)    REG32((b) + 0x20u)
#define GPIO_AFRH(b)    REG32((b) + 0x24u)

#define SPI3_BASE_RAW   0x40003C00u
#define SPI_CR1(b)      REG32((b) + 0x00u)
#define SPI_SR(b)       REG32((b) + 0x08u)
#define SPI_DR8(b)      (*(volatile uint8_t *)((b) + 0x0Cu))

static void reg_gpio_output_pp(uint32_t gpio, uint8_t pin) {
    uint32_t s2 = (uint32_t)pin * 2u;
    GPIO_MODER(gpio) &= ~(0x3u << s2);
    GPIO_MODER(gpio) |= (0x1u << s2);
    GPIO_OTYPER(gpio) &= ~(1u << pin);
    GPIO_OSPEEDR(gpio) &= ~(0x3u << s2);
    GPIO_OSPEEDR(gpio) |= (0x2u << s2);
    GPIO_PUPDR(gpio) &= ~(0x3u << s2);
}

static void reg_gpio_set_af(uint32_t gpio, uint8_t pin, uint8_t af) {
    uint32_t s2 = (uint32_t)pin * 2u;
    uint32_t s4;
    GPIO_MODER(gpio) &= ~(0x3u << s2);
    GPIO_MODER(gpio) |= (0x2u << s2);
    GPIO_OTYPER(gpio) &= ~(1u << pin);
    GPIO_OSPEEDR(gpio) &= ~(0x3u << s2);
    GPIO_OSPEEDR(gpio) |= (0x2u << s2);
    GPIO_PUPDR(gpio) &= ~(0x3u << s2);
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

static void reg_gpio_write(uint32_t gpio, uint8_t pin, uint8_t high) {
    if (high) GPIO_BSRR(gpio) = (1u << pin);
    else      GPIO_BSRR(gpio) = (1u << (pin + 16u));
}

static void spi3_send_bytes(const uint8_t *data, uint16_t len) {
    uint16_t i;
    volatile uint32_t v;
    if ((SPI_SR(SPI3_BASE_RAW) & ((1u << 0) | (1u << 6))) != 0u) {
        v = SPI_DR8(SPI3_BASE_RAW);
        v = SPI_SR(SPI3_BASE_RAW);
        (void)v;
    }
    for (i = 0u; i < len; ++i) {
        while ((SPI_SR(SPI3_BASE_RAW) & (1u << 1)) == 0u) {}
        SPI_DR8(SPI3_BASE_RAW) = data[i];
        while ((SPI_SR(SPI3_BASE_RAW) & (1u << 0)) == 0u) {}
        v = SPI_DR8(SPI3_BASE_RAW);
        (void)v;
    }
    while ((SPI_SR(SPI3_BASE_RAW) & (1u << 7)) != 0u) {}
    if ((SPI_SR(SPI3_BASE_RAW) & (1u << 6)) != 0u) {
        v = SPI_DR8(SPI3_BASE_RAW);
        v = SPI_SR(SPI3_BASE_RAW);
        (void)v;
    }
}

static void spi3_init_raw() {
    RCC_AHB1ENR |= (1u << 2);
    RCC_APB1ENR |= (1u << 15);
    reg_gpio_set_af(GPIOC_BASE_RAW, 10u, 6u);
    reg_gpio_set_af(GPIOC_BASE_RAW, 12u, 6u);
    SPI_CR1(SPI3_BASE_RAW) = 0u;
    //BR=100=fPCLK/32≈1.31MHz (LS013B7DH03最高2MHz,U8g2默认1MHz)
    SPI_CR1(SPI3_BASE_RAW) = (1u << 2) | (4u << 3) | (1u << 9) | (1u << 8) | (1u << 6);
}

/************************************* U8g2字节回调（裸SPI3，含VCOM翻转） *************************************/

//VCOM翻转状态和帧计数，LS013B7DH03需要每帧交替VCOM极性
static uint8_t  s_vcom_toggle;
static uint8_t  s_frame_page_cnt;  // 工程文件原来的逻辑（16页一整屏）
static uint16_t s_vcom_frame_cnt;  // 桌面代码的逻辑（256个tile一整帧）
static uint8_t  s_transfer_is_update;
static uint8_t  s_dc_is_data;

extern "C" uint8_t u8x8_byte_spi3_hw(u8x8_t *u8x8, uint8_t msg, uint8_t arg_int, void *arg_ptr) {
    static uint8_t buf[256];
    switch(msg) {
        case U8X8_MSG_BYTE_SEND: {
            uint8_t *src = (uint8_t *)arg_ptr;
            if (arg_int == 0u) break;
            
            if (ui.param[DISP_BRI] >= 1u) {
                // 工程文件原来的逻辑（高对比度）
                if (s_dc_is_data) {
                    spi3_send_bytes(src, (uint16_t)arg_int);
                } else if (src[0] == 0x80u || src[0] == 0xC0u) {
                    s_frame_page_cnt++;
                    if (s_frame_page_cnt >= 16u) {
                        s_frame_page_cnt = 0u;
                        s_vcom_toggle ^= 1u;
                    }
                    if (s_vcom_toggle) {
                        buf[0] = 0xC0u;
                    } else {
                        buf[0] = 0x80u;
                    }
                    if (arg_int > 1u) memcpy(buf + 1, src + 1, arg_int - 1u);
                    s_transfer_is_update = 1u;
                    spi3_send_bytes(buf, (uint16_t)arg_int);
                } else {
                    spi3_send_bytes(src, (uint16_t)arg_int);
                }
            } else {
                // 桌面代码的逻辑（低对比度）
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

        case U8X8_MSG_BYTE_INIT:
            s_vcom_toggle = 0u;
            s_frame_page_cnt = 0u;
            s_vcom_frame_cnt = 0u;
            s_transfer_is_update = 0u;
            s_dc_is_data = 0u;
            if (u8x8->bus_clock == 0)
                u8x8->bus_clock = u8x8->display_info->sck_clock_hz;
            u8x8_gpio_SetCS(u8x8, u8x8->display_info->chip_disable_level);
            u8x8_gpio_SetDC(u8x8, 1);
            break;

        case U8X8_MSG_BYTE_SET_DC:
            s_dc_is_data = (arg_int != 0u);
            if (ui.param[DISP_BRI] >= 1u) {
                u8x8_gpio_SetDC(u8x8, 1);
            } else {
                u8x8_gpio_SetDC(u8x8, arg_int);
            }
            break;

        case U8X8_MSG_BYTE_START_TRANSFER:
            if (ui.param[DISP_BRI] >= 1u) {
                s_transfer_is_update = 0u;
            } else {
                s_vcom_frame_cnt++;
                if (s_vcom_frame_cnt >= 256u) {
                    s_vcom_frame_cnt = 0u;
                    s_vcom_toggle ^= 1u;
                }
            }
            u8x8_gpio_SetCS(u8x8, u8x8->display_info->chip_enable_level);
            u8x8->gpio_and_delay_cb(u8x8, U8X8_MSG_DELAY_NANO, u8x8->display_info->post_chip_enable_wait_ns, NULL);
            break;

        case U8X8_MSG_BYTE_END_TRANSFER:
            u8x8->gpio_and_delay_cb(u8x8, U8X8_MSG_DELAY_NANO, u8x8->display_info->pre_chip_disable_wait_ns, NULL);
            u8x8_gpio_SetCS(u8x8, u8x8->display_info->chip_disable_level);
            break;

        default:
            return 0;
    }
    return 1;
}

/************************************* 屏幕变量定义 *************************************/

U8G2_LS013B7DH03_128X128_F_4W_SW_SPI u8g2(U8G2_R0, SCL, SDA, CS, DC, RES);
uint8_t *buf_ptr;
uint16_t buf_len;

/************************************* 显示函数 *************************************/

void lcd_init() {
    //CS=PC4 DISP=PC5 先行配置为推挽输出
    reg_gpio_output_pp(GPIOC_BASE_RAW, 4u);
    reg_gpio_output_pp(GPIOC_BASE_RAW, 5u);
    reg_gpio_write(GPIOC_BASE_RAW, 4u, 0u);
    reg_gpio_write(GPIOC_BASE_RAW, 5u, 1u);
    //先启SPI3，让u8g2.begin()内的ALL_CLEAR能通过
    spi3_init_raw();
    u8g2.getU8x8()->byte_cb = u8x8_byte_spi3_hw;
    u8g2.begin();
    //u8g2.begin()的pinMode会覆盖PC10/PC12的AF6为GPIO_OUTPUT，必须恢复
    spi3_init_raw();
    reg_gpio_write(GPIOC_BASE_RAW, 5u, 1u);
    u8g2.setContrast(ui.param[DISP_BRI]);
    buf_ptr = u8g2.getBufferPtr();
    buf_len = 8 * u8g2.getBufferTileHeight() * u8g2.getBufferTileWidth();
}
