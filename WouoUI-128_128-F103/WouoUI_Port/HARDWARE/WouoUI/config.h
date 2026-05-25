#ifndef CONFIG_H
#define CONFIG_H

#include "stm32f10x.h"
#include "u8g2_adapter.h"
#include "hw_abstraction.h"
#include "delay.h"
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>

/************************************* 屏幕驱动 *************************************/
#define DISP_H 128
#define DISP_W 128

/************************************* UI配置 *************************************/
#define UI_DEPTH 20
#define UI_MNUMB 100
#define UI_PARAM 19
#define UI_NUMB_MAX 255

/************************************* 磁贴配置 *************************************/
#define TILE_B_FONT (&u8g2_font_helvB24_tr)
#define TILE_S_FONT (&u8g2_font_HelvetiPixel_tr)
#define TILE_B_TITLE_H 25
#define TILE_S_TITLE_H 8
#define TILE_ICON_H 48
#define TILE_ICON_W 48
#define TILE_ICON_S 57
#define TILE_INDI_H 40
#define TILE_INDI_W 10
#define TILE_INDI_S 57

/************************************* 列表配置 *************************************/
#define LIST_FONT (&u8g2_font_HelvetiPixel_tr)
#define LIST_TEXT_H 8
#define LIST_LINE_H 16
#define LIST_TEXT_S 4
#define LIST_BAR_W 5
#define LIST_BOX_R 0.5f

/************************************* 电压测量配置 *************************************/
#define WAVE_SAMPLE 20
#define WAVE_W DISP_W
#define WAVE_L 0
#define WAVE_U 0
#define WAVE_MAX 43
#define WAVE_MIN 5
#define WAVE_BOX_H 49
#define WAVE_BOX_W DISP_W
#define VOLT_FONT (&u8g2_font_helvB24_tr)
#define VOLT_LIST_U_S 94
#define VOLT_TEXT_BG_U_S 53
#define VOLT_TEXT_BG_H 33

/************************************* 复选框配置 *************************************/
#define CHECK_BOX_L_S 95
#define CHECK_BOX_U_S 2
#define CHECK_BOX_F_W 12
#define CHECK_BOX_F_H 12
#define CHECK_BOX_D_S 2

/************************************* 弹窗配置 *************************************/
#define WIN_FONT (&u8g2_font_HelvetiPixel_tr)
#define WIN_H 32
#define WIN_W 102
#define WIN_BAR_W 92
#define WIN_BAR_H 7
#define WIN_Y (-WIN_H - 2)
#define WIN_Y_TRG (-WIN_H - 2)
#define WIN_MSG_PAD 4                         //消息弹窗文字边距

/************************************* 关于页面配置 *************************************/
#define ABOUT_FONT (&u8g2_font_HelvetiPixel_tr)
#define ABOUT_INDI_S 4
#define ABOUT_INDI_W 2

/************************************* 定义页面 *************************************/
enum PageIndex {
    M_WINDOW, M_SLEEP, M_MAIN, M_EDITOR, M_KNOB, M_KRF, M_KPF,
    M_VOLT, M_SETTING, M_ABOUT,
};

enum PageState {
    S_FADE, S_WINDOW, S_LAYER_IN, S_LAYER_OUT, S_NONE
};

enum ParamIndex {
    DISP_BRI, TILE_ANI, LIST_ANI, WIN_ANI, SPOT_ANI, TAG_ANI, FADE_ANI,
    BTN_SPT, BTN_LPT, TILE_UFD, LIST_UFD, TILE_LOOP, LIST_LOOP,
    WIN_BOK, KNOB_DIR, DARK_MODE, ROTATE_SCR, LIST_CUR, SLP_T
};

/************************************* 旋钮配置 *************************************/
#define AIO PB12
#define BIO PB13
#define SW PB14
#define KNOB_PARAM 4
#define KNOB_DISABLE 0
#define KNOB_ROT_VOL 1
#define KNOB_ROT_BRI 2
#define BTN_PARAM_TIMES 2

enum ButtonId {
    BTN_ID_CC, BTN_ID_CW, BTN_ID_SP, BTN_ID_LP
};

enum KnobParamIndex {
    KNOB_ROT, KNOB_COD, KNOB_ROT_P, KNOB_COD_P
};

/************************************* EEPROM配置 *************************************/
#define EEPROM_CHECK 11

#endif
