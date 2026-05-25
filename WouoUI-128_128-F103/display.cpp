
#include "display.h"
#include "ui_state.h"

/************************************* 屏幕变量定义 *************************************/

#ifdef USE_I2C
U8G2_SH1107_SEEED_128X128_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);
#else
U8G2_SH1107_SEEED_128X128_F_4W_HW_SPI u8g2(U8G2_R0, CS, DC, RES);
#endif
uint8_t *buf_ptr;                 //指向屏幕缓冲的指针
uint16_t buf_len;                  //缓冲长度

/************************************* 显示函数 *************************************/

void oled_init() {
#ifdef USE_I2C
    u8g2.setBusClock(400000);
#else
    u8g2.setBusClock(10000000);
#endif
    u8g2.begin();
    u8g2.setContrast(ui.param[DISP_BRI]);
    buf_ptr = u8g2.getBufferPtr();
    buf_len = 8 * u8g2.getBufferTileHeight() * u8g2.getBufferTileWidth();
}

