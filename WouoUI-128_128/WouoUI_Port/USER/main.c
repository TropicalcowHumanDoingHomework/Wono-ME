#include "delay.h"
#include "sys.h"
#include "oled.h"
#include "u8g2_adapter.h"
#include "ui_state.h"
#include "menu_data.h"
#include "animation.h"
#include "pages.h"
#include "knob.h"
#include "window.h"
#include "eeprom_manager.h"

int main(void)
{
    /* 系统初始化 */
    delay_init();
    OLED_Init();
    OLED_ColorTurn(0);

    /* 硬件初始化 */
    btn_init();

    /* UI状态初始化 */
    u8g2_Begin();
    eeprom_init();
    ui_init();

    /* 初始化动态计算值(Arduino原版公式: TILE_INDI_S + (TILE_INDI_H - TILE_B_TITLE_H)/2 + TILE_B_TITLE_H * N) */
    tile.title_y_calc = TILE_INDI_S + (TILE_INDI_H - TILE_B_TITLE_H) / 2 + TILE_B_TITLE_H * 2;
    tile.title_y_trg_calc = TILE_INDI_S + (TILE_INDI_H - TILE_B_TITLE_H) / 2 + TILE_B_TITLE_H;
    list.line_n = DISP_H / LIST_LINE_H;

    /* 启动到主页面 */
    ui.select[0] = 0;
    ui.index = M_MAIN;
    ui.state = S_LAYER_IN;
    layer_init_in();

    while (1)
    {
        knob_inter();   /* 先处理旋钮中断(轮询) */
        btn_scan();     /* 后处理按键(覆盖btn.id,保证事件优先) */

        ui_proc();
    }
}
