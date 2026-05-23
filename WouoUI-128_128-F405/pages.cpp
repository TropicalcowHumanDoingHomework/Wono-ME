#include "pages.h"
#include "ui_state.h"
#include "animation.h"
#include "display.h"
#include "menu_data.h"
#include "eeprom_manager.h"
#include "knob.h"
#include "window.h"
#include "hid_manager.h"
#include "usb_manager.h"
#include "led.h"
#include <math.h>
#include <string.h>

#define TIM12_BASE  0x40001800u
#define TIM12_CR1   (*(volatile uint32_t *)(TIM12_BASE + 0x00u))
#define TIM12_CCR1  (*(volatile uint32_t *)(TIM12_BASE + 0x34u))

//开发板模拟引脚
uint8_t analog_pin[10] = {PA0, PA1, PA2, PA3, PA4, PA5, PA6, PA7, PB0, PB1};


/*************** 根据列表每行开头符号，判断每行尾部是否绘制以及绘制什么内容 *************/

void check_box_v_init(uint8_t *param) {
    check_box.v = param;
}

void check_box_m_init(uint8_t *param) {
    check_box.m = param;
}

void check_box_s_init(uint8_t *param, uint8_t *param_p) {
    check_box.s = param;
    check_box.s_p = param_p;
}

void check_box_m_select(uint8_t param) {
    check_box.m[param] = !check_box.m[param];
    eeprom.change = true;
}

void check_box_s_select(uint8_t val, uint8_t pos) {
    *check_box.s = val;
    *check_box.s_p = pos;
    eeprom.change = true;
}



//列表显示数值
void list_draw_value(int n) {
    u8g2.print(check_box.v[n - 1]);
}

//绘制外框
void list_draw_check_box_frame() {
    u8g2.drawRFrame(CHECK_BOX_L_S + list.curve, list.temp + CHECK_BOX_U_S, CHECK_BOX_F_W, CHECK_BOX_F_H, 1);
}

//绘制框里面的点
void list_draw_check_box_dot() {
    u8g2.drawBox(CHECK_BOX_L_S + CHECK_BOX_D_S + 1 + list.curve, list.temp + CHECK_BOX_U_S + CHECK_BOX_D_S + 1, CHECK_BOX_F_W - (CHECK_BOX_D_S + 1) * 2, CHECK_BOX_F_H - (CHECK_BOX_D_S + 1) * 2);
}

//列表显示旋钮功能
void list_draw_krf(int n) {
    switch (check_box.v[n - 1]) {
        case 0: u8g2.print("OFF"); break;
        case 1: u8g2.print("VOL"); break;
        case 2: u8g2.print("BRI"); break;
    }
}

//列表显示按键键值
void list_draw_kpf(int n) {
    if (check_box.v[n - 1] == 0) u8g2.print("OFF");
    else if (check_box.v[n - 1] <= 90) u8g2.print((char)check_box.v[n - 1]);
    else u8g2.print("?");
}

//判断列表尾部内容
void list_draw_text_and_check_box(Menu* arr, int i) {
    u8g2.drawStr(LIST_TEXT_S + list.curve, list.temp + LIST_TEXT_H + LIST_TEXT_S, arr[i].title);
    u8g2.setCursor(CHECK_BOX_L_S + list.curve, list.temp + LIST_TEXT_H + LIST_TEXT_S);
    switch (arr[i].title[0]) {
        case '~': if (check_box.map) list_draw_value(check_box.map[i - 1] + 1); else list_draw_value(i); break;
        case '+': list_draw_check_box_frame(); if (check_box.map) { if (check_box.m[check_box.map[i - 1]] == 1) list_draw_check_box_dot(); } else { if (check_box.m[i - 1] == 1) list_draw_check_box_dot(); } break;
        case '=': list_draw_check_box_frame(); if (*check_box.s_p == i) list_draw_check_box_dot(); break;
        case '#': list_draw_krf(i); break;
        case '$': list_draw_kpf(i); break;
    }
}


/********************************* 映射数组 *********************************/

// Setting 页面：菜单项位置 → 参数索引映射
// 菜单项 1~9 对应的 ParamIndex
static const uint8_t setting_param_map[] = {
    0,   // 1: Disp Bri  → DISP_BRI
    19,  // 2: Dark Mode → DARK_MODE
    20,  // 3: Rotate Scr → ROTATE_SCR
    21,  // 4: Buzzer Vol → BUZ_VOL
    11,  // 5: Btn SPT   → BTN_SPT
    12,  // 6: Btn LPT   → BTN_LPT
    18,  // 7: Knob Rot Dir → KNOB_DIR
    22,  // 8: USB Storage → USB_ENABLE
    0    // 9: [ About ] → 无显示（占位）
};

// Animi 页面：菜单项位置 → 参数索引映射
// 菜单项 1~17 对应的 ParamIndex
static const uint8_t animi_param_map[] = {
    1,   // 1: Tile Ani  → TILE_ANI
    2,   // 2: List Cur  → LIST_CUR
    3,   // 3: Box X OS  → BOX_X_OS
    4,   // 4: Box Y OS  → BOX_Y_OS
    5,   // 5: Win Y OS  → WIN_Y_OS
    6,   // 6: List Ani  → LIST_ANI
    7,   // 7: Win Ani   → WIN_ANI
    8,   // 8: Spot Ani  → SPOT_ANI
    9,   // 9: Tag Ani   → TAG_ANI
    10,  // 10: Fade Ani → FADE_ANI
    24,  // 11: Fade Mode → FADE_MODE
    13,  // 12: T Ufd Fm Scr → TILE_UFD
    14,  // 13: L Ufd Fm Scr → LIST_UFD
    15,  // 14: T Loop Mode → TILE_LOOP
    16,  // 15: L Loop Mode → LIST_LOOP
    17,  // 16: Win Bokeh Bg → WIN_BOK
    23   // 17: Win Stretch → WIN_STYLE
};


/********************************* 分页面初始化函数 *********************************/

//进入磁贴类时的初始化
void tile_param_init() {
    ui.init = false;
    tile.icon_x = 0;
    tile.icon_x_trg = TILE_ICON_S;
    tile.icon_y = -TILE_ICON_H;
    tile.icon_y_trg = 0;
    tile.indi_x = 0;
    tile.indi_x_trg = TILE_INDI_W;
    tile.title_y = tile.title_y_calc;
    tile.title_y_trg = tile.title_y_trg_calc;
    tile.select_flag = true;  // 防止动画完成后复位指示器和标题
    led_set_red();  // 进入主菜单时显示红色
}


/************************************* 显示函数 *************************************/

//磁贴类页面通用显示函数
void tile_show(Menu* arr_1, Menu* arr_2, const uint8_t icon_pic[][16 * 18]) {
    //计算动画过渡值
    animation(&tile.icon_x, &tile.icon_x_trg, TILE_ANI);
    animation(&tile.icon_y, &tile.icon_y_trg, TILE_ANI);
    animation(&tile.indi_x, &tile.indi_x_trg, TILE_ANI);
    animation(&tile.title_y, &tile.title_y_trg, TILE_ANI);

    //设置大小标题的颜色和文字方向，0透显，1实显，2反色，这里都用实显
    u8g2.setDrawColor(1);
    u8g2.setFontDirection(0);

    //绘制大标题
    u8g2.setFont(TILE_B_FONT);
    u8g2.drawStr(((DISP_W - TILE_INDI_W) - u8g2.getStrWidth(arr_1[ui.select[ui.layer]].title)) / 2 + TILE_INDI_W, tile.title_y, arr_1[ui.select[ui.layer]].title);

    //绘制小标题
    u8g2.setFont(TILE_S_FONT);
    u8g2.drawStr(((DISP_W - u8g2.getStrWidth(arr_2[ui.select[ui.layer]].title)) / 2), 0.5 * (TILE_ICON_S + TILE_INDI_H + DISP_H + LIST_TEXT_H), arr_2[ui.select[ui.layer]].title);

    //绘制大标题指示器
    u8g2.drawBox(0, TILE_ICON_S, tile.indi_x, TILE_INDI_H);

    //绘制图标
    if (!ui.init) {
        for (uint8_t i = 0; i < ui.num[ui.index]; ++i) {
            if (ui.param[TILE_UFD]) tile.temp = (DISP_W - TILE_ICON_W) / 2 + i * tile.icon_x - TILE_ICON_S * ui.select[ui.layer];
            else tile.temp = (DISP_W - TILE_ICON_W) / 2 + (i - ui.select[ui.layer]) * tile.icon_x;
            u8g2.drawXBMP(tile.temp, (int16_t)tile.icon_y, TILE_ICON_W, TILE_ICON_H, icon_pic[i]);
        }
        if (tile.icon_x == tile.icon_x_trg) {
            ui.init = true;
            tile.icon_x = tile.icon_x_trg = -ui.select[ui.layer] * TILE_ICON_S;
        }
    }
    else for (uint8_t i = 0; i < ui.num[ui.index]; ++i) u8g2.drawXBMP((DISP_W - TILE_ICON_W) / 2 + (int16_t)tile.icon_x + i * TILE_ICON_S, 0, TILE_ICON_W, TILE_ICON_H, icon_pic[i]);

    //反转屏幕内元素颜色，白天模式遮罩
    u8g2.setDrawColor(2);
    if (!ui.param[DARK_MODE]) u8g2.drawBox(0, 0, DISP_W, DISP_H);
}

static void list_update_box_size(Menu* menu);

void tile_rotate_switch() {
    switch (btn.id) {
        case BTN_ID_CC:
            if (ui.init) {
                if (ui.select[ui.layer] > 0) {
                    ui.select[ui.layer] -= 1;
                    tile.icon_x_trg += TILE_ICON_S;
                    tile.select_flag = false;
                }
                else {
                    if (ui.param[TILE_LOOP]) {
                        ui.select[ui.layer] = ui.num[ui.index] - 1;
                        tile.icon_x_trg = -TILE_ICON_S * (ui.num[ui.index] - 1);
                        tile.select_flag = false;
                        break;
                    }
                    else tile.select_flag = true;
                }
            }
            break;

        case BTN_ID_CW:
            if (ui.init) {
                if (ui.select[ui.layer] < (ui.num[ui.index] - 1)) {
                    ui.select[ui.layer] += 1;
                    tile.icon_x_trg -= TILE_ICON_S;
                    tile.select_flag = false;
                }
                else {
                    if (ui.param[TILE_LOOP]) {
                        ui.select[ui.layer] = 0;
                        tile.icon_x_trg = 0;
                        tile.select_flag = false;
                        break;
                    }
                    else tile.select_flag = true;
                }
            }
            break;
    }
}

void list_rotate_switch() {
    if (!list.loop) {
        // 获取当前页面的菜单
        Menu* current_menu = NULL;
        switch (ui.index) {
            case M_MAIN: current_menu = main_menu; break;
            case M_ANIMITION: current_menu = animition_menu; break;
            case M_EDITOR: current_menu = editor_menu; break;
            case M_KNOB: current_menu = knob_menu; break;
            case M_KRF: current_menu = krf_menu; break;
            case M_KPF: current_menu = kpf_menu; break;
            case M_VOLT: current_menu = volt_menu; break;
            case M_SETTING: current_menu = setting_menu; break;
            case M_ABOUT: current_menu = about_menu; break;
            case M_WIN_LIST_DEMO: current_menu = win_list_demo_menu; break;
        }
        if (!current_menu) return;
        
        switch (btn.id) {
            case BTN_ID_CC:
                if (ui.select[ui.layer] == 0) {
                    if (ui.param[LIST_LOOP] && ui.init) {
                        list.loop = true;
                        ui.select[ui.layer] = ui.num[ui.index] - 1;
                        if (ui.num[ui.index] > list.line_n) {
                            list.box_y_trg[ui.layer] = DISP_H - LIST_LINE_H;
                            list.y_trg = DISP_H - ui.num[ui.index] * LIST_LINE_H;
                        }
                        else list.box_y_trg[ui.layer] = (ui.num[ui.index] - 1) * LIST_LINE_H;
                        list_update_box_size(current_menu);
                        break;
                    }
                    else break;
                }
                if (ui.init) {
                    ui.select[ui.layer] -= 1;
                    if (ui.select[ui.layer] < -(list.y_trg / LIST_LINE_H)) {
                        if (!(DISP_H % LIST_LINE_H)) list.y_trg += LIST_LINE_H;
                        else {
                            if (list.box_y_trg[ui.layer] == DISP_H - LIST_LINE_H * list.line_n) {
                                list.y_trg += (list.line_n + 1) * LIST_LINE_H - DISP_H;
                                list.box_y_trg[ui.layer] = 0;
                            }
                            else if (list.box_y_trg[ui.layer] == LIST_LINE_H) {
                                list.box_y_trg[ui.layer] = 0;
                            }
                            else list.y_trg += LIST_LINE_H;
                        }
                    }
                    else list.box_y_trg[ui.layer] -= LIST_LINE_H;
                    list_update_box_size(current_menu);
                    break;
                }

            case BTN_ID_CW:
                if (ui.select[ui.layer] == (ui.num[ui.index] - 1)) {
                    if (ui.param[LIST_LOOP] && ui.init) {
                        list.loop = true;
                        ui.select[ui.layer] = 0;
                        list.y_trg = 0;
                        list.box_y_trg[ui.layer] = 0;
                        list_update_box_size(current_menu);
                        break;
                    }
                    else break;
                }
                if (ui.init) {
                    ui.select[ui.layer] += 1;
                    if ((ui.select[ui.layer] + 1) > (list.line_n - list.y_trg / LIST_LINE_H)) {
                        if (!(DISP_H % LIST_LINE_H)) list.y_trg -= LIST_LINE_H;
                        else {
                            if (list.box_y_trg[ui.layer] == LIST_LINE_H * (list.line_n - 1)) {
                                list.y_trg -= (list.line_n + 1) * LIST_LINE_H - DISP_H;
                                list.box_y_trg[ui.layer] = DISP_H - LIST_LINE_H;
                            }
                            else if (list.box_y_trg[ui.layer] == DISP_H - LIST_LINE_H * 2) {
                                list.box_y_trg[ui.layer] = DISP_H - LIST_LINE_H;
                            }
                            else list.y_trg -= LIST_LINE_H;
                        }
                    }
                    else list.box_y_trg[ui.layer] += LIST_LINE_H;
                    list_update_box_size(current_menu);
                    break;
                }
                break;
        }
    }
}


// 更新选择框尺寸（含过度延伸）
void list_update_box_size(Menu* menu) {
    u8g2.setFont(LIST_FONT);
    list.box_W = u8g2.getStrWidth(menu[ui.select[ui.layer]].title) + LIST_TEXT_S * 2;
    list.box_H = LIST_LINE_H;
    list.box_w_trg = list.box_W;
    list.box_h_trg = list.box_H;
    list.box_w_trg += ui.param[BOX_X_OS];
    list.box_h_trg += ui.param[BOX_Y_OS];
}


/******************************** 列表显示函数 **************************************/

//列表类页面通用显示函数
void list_show(Menu* arr, uint8_t ui_index) {
    //更新动画目标值
    u8g2.setFont(LIST_FONT);
    list.box_W = u8g2.getStrWidth(arr[ui.select[ui.layer]].title) + LIST_TEXT_S * 2;
    list.box_x_trg = list.box_W;
    list.bar_y_trg = ceil((ui.select[ui.layer]) * ((float)DISP_H / (ui.num[ui_index] - 1)));
    list.box_H = LIST_LINE_H;

    //计算动画过渡值
    animation(&list.y, &list.y_trg, LIST_ANI);
    animation(&list.box_x, &list.box_x_trg, LIST_ANI);
    animation(&list.box_w, &list.box_w_trg, LIST_ANI);
    animation(&list.box_w_trg, &list.box_W, LIST_ANI);
    animation(&list.box_y, &list.box_y_trg[ui.layer], LIST_ANI);
    animation(&list.box_h, &list.box_h_trg, LIST_ANI);
    animation(&list.box_h_trg, &list.box_H, LIST_ANI);
    animation(&list.bar_y, &list.bar_y_trg, LIST_ANI);

    if (list.loop && list.box_y == list.box_y_trg[ui.layer]) list.loop = false;

    u8g2.setDrawColor(1);

    if (!ui.init) {
        for (int i = 0; i < ui.num[ui_index]; ++i) {
            if (ui.param[LIST_UFD]) list.temp = i * list.y - LIST_LINE_H * ui.select[ui.layer] + list.box_y_trg[ui.layer];
            else list.temp = (i - ui.select[ui.layer]) * list.y + list.box_y_trg[ui.layer];
            if (list.temp + LIST_LINE_H <= 0 || list.temp >= DISP_H) continue;
            list.curve = (ui.param[LIST_CUR] / 1000.0f) * pow(list.temp - list.box_y, 2);
            list_draw_text_and_check_box(arr, i);
        }
        if (list.y == list.y_trg) {
            ui.init = true;
            list.y = list.y_trg = -LIST_LINE_H * ui.select[ui.layer] + list.box_y_trg[ui.layer];
        }
    }
    else for (int i = 0; i < ui.num[ui_index]; ++i) {
        list.temp = LIST_LINE_H * i + list.y;
        if (list.temp + LIST_LINE_H <= 0 || list.temp >= DISP_H) continue;
        list.curve = (ui.param[LIST_CUR] / 1000.0f) * pow(list.temp - list.box_y, 2);
        list_draw_text_and_check_box(arr, i);
    }

    u8g2.drawHLine(DISP_W - LIST_BAR_W, 0, LIST_BAR_W);
    u8g2.drawHLine(DISP_W - LIST_BAR_W, DISP_H - 1, LIST_BAR_W);
    u8g2.drawVLine(DISP_W - ceil((float)LIST_BAR_W / 2), 0, DISP_H);
    u8g2.drawBox(DISP_W - LIST_BAR_W, 0, LIST_BAR_W, list.bar_y);

    u8g2.setDrawColor(2);
    if (list.box_y + LIST_LINE_H > 0 && list.box_y < DISP_H)
        u8g2.drawRBox(0, list.box_y - (list.box_h - LIST_LINE_H) / 2, list.box_w, list.box_h, LIST_BOX_R);

    if (!ui.param[DARK_MODE]) {
        u8g2.drawBox(0, 0, DISP_W, DISP_H);
    }
}

//电压测量页面初始化
void volt_param_init() {
    volt.text_bg_l = 0;
    volt.text_bg_l_trg = DISP_W;
}

void volt_show()
{
  //更新动画目标值
  u8g2.setFont(LIST_FONT);
  list.box_x_trg = u8g2.getStrWidth(volt_menu[ui.select[ui.layer]].title) + LIST_TEXT_S * 2;

  //计算动画过渡值  
  animation(&list.y, &list.y_trg, LIST_ANI);
  animation(&list.box_x, &list.box_x_trg, LIST_ANI);
  animation(&list.box_y, &list.box_y_trg[ui.layer], LIST_ANI);
  animation(&volt.text_bg_l, &volt.text_bg_l_trg, TAG_ANI);

  //检查循环动画是否结束
  if (list.loop && list.box_y == list.box_y_trg[ui.layer]) list.loop = false;

  //设置文字和曲线颜色，0透显，1实显，2反色，这里都用实显
  u8g2.setDrawColor(1);  

  //绘制列表文字
  u8g2.setFontDirection(1);
  if (!ui.init)
  {
    for (uint8_t i = 0; i < ui.num[ui.index]; ++ i) u8g2.drawStr(LIST_TEXT_S + (i - ui.select[ui.layer]) * list.y + list.box_y_trg[ui.layer] - 1, VOLT_LIST_U_S , volt_menu[i].title);
    if (list.y == list.y_trg) 
    {
      ui.init = true;
      list.y = list.y_trg = - LIST_LINE_H * ui.select[ui.layer] + list.box_y_trg[ui.layer];
    }
  }
  else for (uint8_t i = 0; i < ui.num[ui.index]; ++ i) u8g2.drawStr(LIST_TEXT_S + LIST_LINE_H * i + (int16_t)list.y - 1, VOLT_LIST_U_S , volt_menu[i].title); 
  
  //绘制电压曲线和外框
  volt.val = 0;
  u8g2.drawFrame(0, 0, WAVE_BOX_W, WAVE_BOX_H);
  u8g2.drawFrame(1, 1, WAVE_BOX_W - 2, WAVE_BOX_H - 2);
  if (list.box_y == list.box_y_trg[ui.layer] && list.y == list.y_trg)
  {
    for (int i = 0; i < WAVE_SAMPLE * WAVE_W; i++) volt.ch0_adc[i] = volt.val = analogRead(analog_pin[ui.select[ui.layer]]);
    for (int i = 1; i < WAVE_W - 1; i++)
    { 
      volt.ch0_wave[i] = map(volt.ch0_adc[int(5 * i)], 0, 4095, WAVE_MAX, WAVE_MIN);   
      u8g2.drawLine(WAVE_L + i - 1, WAVE_U + volt.ch0_wave[i - 1], WAVE_L + i, WAVE_U + volt.ch0_wave[i]);
    }
  }

  //绘制电压值
  u8g2.setFontDirection(0);
  u8g2.setFont(VOLT_FONT); 
  u8g2.setCursor(23, VOLT_LIST_U_S - 12);
  u8g2.print(volt.val / 4096.0f * 3.3f);
  u8g2.print("V");

  //绘制列表选择框和电压文字背景
  u8g2.setDrawColor(2);
  u8g2.drawRBox(list.box_y, VOLT_LIST_U_S - LIST_TEXT_S, LIST_LINE_H, list.box_x, LIST_BOX_R);
  u8g2.drawBox(DISP_W - volt.text_bg_l, VOLT_TEXT_BG_U_S, DISP_W, VOLT_TEXT_BG_H);

  //反转屏幕内元素颜色，白天模式遮罩
  if (!ui.param[DARK_MODE]) u8g2.drawBox(0, 0, DISP_W, DISP_H);
}

//关于本机页面初始化
void about_param_init() {
    about.indi_x = 0;
    about.indi_x_trg = ABOUT_INDI_S;
}

void about_show() {
    u8g2.setFont(LIST_FONT);
    list.box_x_trg = u8g2.getStrWidth(about_menu[0].title) + LIST_TEXT_S * 2;

    animation(&list.box_x, &list.box_x_trg, TAG_ANI);
    animation(&about.indi_x, &about.indi_x_trg, TAG_ANI);

    u8g2.setDrawColor(1);

    u8g2.drawStr(ABOUT_INDI_S + LIST_TEXT_S, ABOUT_INDI_S + LIST_TEXT_S + LIST_TEXT_H, about_menu[0].title);
    u8g2.drawStr(ABOUT_INDI_S + list.box_x_trg + ABOUT_INDI_S, ABOUT_INDI_S + LIST_TEXT_S + LIST_TEXT_H, about_menu[1].title);
    for (int i = 2; i < ui.num[M_ABOUT]; i++) u8g2.drawStr(about.indi_x_trg + ABOUT_INDI_W + ABOUT_INDI_S * 2, ABOUT_INDI_S + LIST_LINE_H + LIST_TEXT_S / 2 + (i - 1) * LIST_LINE_H, about_menu[i].title);
    u8g2.drawBox(about.indi_x, ABOUT_INDI_S + LIST_LINE_H + ABOUT_INDI_S * 2, ABOUT_INDI_W, (ui.num[M_ABOUT] - 2) * LIST_LINE_H - LIST_TEXT_S);

    u8g2.setDrawColor(2);
    u8g2.drawRBox(ABOUT_INDI_S, ABOUT_INDI_S, list.box_x, LIST_LINE_H, LIST_BOX_R);

    if (!ui.param[DARK_MODE]) u8g2.drawBox(0, 0, DISP_W, DISP_H);
}

void animition_param_init() {
    check_box_v_init(ui.param);
    check_box_m_init(ui.param);
    check_box.map = (uint8_t*)animi_param_map;
}

void animition_proc() {
    list_show(animition_menu, M_ANIMITION);
    if (btn.pressed) {
        btn.pressed = false;
        switch (btn.id) {
            case BTN_ID_CW:
            case BTN_ID_CC:
                list_rotate_switch();
                break;
            case BTN_ID_LP:
                ui.select[ui.layer] = 0;
            case BTN_ID_SP:
                switch (ui.select[ui.layer]) {
                    case 0: ui.index = M_MAIN; ui.state = S_LAYER_OUT; break;
                    case 1: window_value_init("Tile Ani", TILE_ANI, &ui.param[TILE_ANI], 100, 10, 1, animition_menu, M_ANIMITION); break;
                    case 2: window_value_init("List Cur", LIST_CUR, &ui.param[LIST_CUR], 200, 0, 1, animition_menu, M_ANIMITION); break;
                    case 3: window_value_init("Box X OS", BOX_X_OS, &ui.param[BOX_X_OS], 40, 0, 1, animition_menu, M_ANIMITION); break;
                    case 4: window_value_init("Box Y OS", BOX_Y_OS, &ui.param[BOX_Y_OS], 40, 0, 1, animition_menu, M_ANIMITION); break;
                    case 5: window_value_init("Win Y OS", WIN_Y_OS, &ui.param[WIN_Y_OS], 40, 0, 1, animition_menu, M_ANIMITION); break;
                    case 6: window_value_init("List Ani", LIST_ANI, &ui.param[LIST_ANI], 100, 10, 1, animition_menu, M_ANIMITION); break;
                    case 7: window_value_init("Win Ani", WIN_ANI, &ui.param[WIN_ANI], 100, 10, 1, animition_menu, M_ANIMITION); break;
                    case 8: window_value_init("Spot Ani", SPOT_ANI, &ui.param[SPOT_ANI], 100, 10, 1, animition_menu, M_ANIMITION); break;
                    case 9: window_value_init("Tag Ani", TAG_ANI, &ui.param[TAG_ANI], 100, 10, 1, animition_menu, M_ANIMITION); break;
                    case 10: window_value_init("Fade Ani", FADE_ANI, &ui.param[FADE_ANI], 255, 0, 1, animition_menu, M_ANIMITION); break;
                    case 11: check_box_m_select(FADE_MODE); break;
                    case 12: check_box_m_select(TILE_UFD); break;
                    case 13: check_box_m_select(LIST_UFD); break;
                    case 14: check_box_m_select(TILE_LOOP); break;
                    case 15: check_box_m_select(LIST_LOOP); break;
                    case 16: check_box_m_select(WIN_BOK); break;
                    case 17: check_box_m_select(WIN_STYLE); break;
                }
                break;
        }
    }
}

//进入睡眠时的初始化
void sleep_param_init() {
    u8g2.setDrawColor(0);
    u8g2.drawBox(0, 0, DISP_W, DISP_H);
    u8g2.sendBuffer();
    u8g2.setPowerSave(1);
    ui.state = S_NONE;
    ui.sleep = true;

    TIM12_CCR1 = 0;
    TIM12_CR1 &= ~(1u << 0);

    if (USBManager::isEnabled()) {
        USBManager::end();
    }

    if (eeprom.change) {
        led_set_white();  // 保存 Flash 时显示白色
        eeprom_write_all_data();
        eeprom.change = false;
    }
    
    led_start_breathing_white();  // 保存完毕后启动白色呼吸灯
}

void sleep_proc() {
    while (ui.sleep) {
        btn_scan();
        buzzer_proc();
        led_proc();

        if (btn.pressed) {
            btn.pressed = false;
            switch (btn.id) {
                case BTN_ID_CW:
#if HID_ENABLE
                    switch (knob.param[KNOB_ROT]) {
                        case KNOB_ROT_VOL: Consumer.press(HIDConsumer::VOLUME_UP); Consumer.release(); break;
                        case KNOB_ROT_BRI: Consumer.press(HIDConsumer::BRIGHTNESS_UP); Consumer.release(); break;
                    }
#endif
                    break;

                case BTN_ID_CC:
#if HID_ENABLE
                    switch (knob.param[KNOB_ROT]) {
                        case KNOB_ROT_VOL: Consumer.press(HIDConsumer::VOLUME_DOWN); Consumer.release(); break;
                        case KNOB_ROT_BRI: Consumer.press(HIDConsumer::BRIGHTNESS_DOWN); Consumer.release(); break;
                    }
#endif
                    break;

                case BTN_ID_SP:
#if HID_ENABLE
                    Keyboard.press(knob.param[KNOB_COD]); Keyboard.release(knob.param[KNOB_COD]);
#endif
                    break;

                case BTN_ID_LP: buzzer_exit_sound(); led_set_red(); if (ui.param[USB_ENABLE]) { USBManager::begin(); } ui.index = M_MAIN; ui.state = S_LAYER_IN; u8g2.setPowerSave(0); ui.sleep = false; break;
            }
        }
    }
}

void main_proc() {
    tile_show(main_menu, main_menu_exp, main_icon_pic);
    if (btn.pressed) {
        btn.pressed = false;
        switch (btn.id) {
            case BTN_ID_CW:
            case BTN_ID_CC:
                tile_rotate_switch();
                break;
            case BTN_ID_SP:
                switch (ui.select[ui.layer]) {
                    case 0: ui.index = M_SLEEP; ui.state = S_LAYER_OUT; break;
                    case 1: ui.index = M_EDITOR; ui.state = S_LAYER_IN; break;
                    case 2: ui.index = M_VOLT; ui.state = S_LAYER_IN; break;
                    case 3: ui.index = M_ANIMITION; ui.state = S_LAYER_IN; break;
                    case 4: ui.index = M_SETTING; ui.state = S_LAYER_IN; break;
                }
                break;
        }
    }
    if (!tile.select_flag && ui.init) {
        tile.indi_x = 0;
        tile.title_y = tile.title_y_calc;
        tile.select_flag = true;
    }
}

void editor_proc() {
    list_show(editor_menu, M_EDITOR);
    if (btn.pressed) {
        btn.pressed = false;
        switch (btn.id) {
            case BTN_ID_CW:
            case BTN_ID_CC:
                list_rotate_switch();
                break;
            case BTN_ID_LP:
                ui.select[ui.layer] = 0;
            case BTN_ID_SP:
                switch (ui.select[ui.layer]) {
                    case 0: ui.index = M_MAIN; ui.state = S_LAYER_OUT; break;
                    case 10: ui.index = M_WIN_LIST_DEMO; ui.state = S_LAYER_IN; break;
                    case 11: window_message_init("Message", "Hello World!\nLine 2\nLine 3", editor_menu, M_EDITOR); break;
                    case 12: ui.index = M_KNOB; ui.state = S_LAYER_IN; break;
                }
                break;
        }
    }
}

void win_list_demo_proc() {
    list_show(win_list_demo_menu, M_WIN_LIST_DEMO);
    if (btn.pressed) {
        btn.pressed = false;
        switch (btn.id) {
            case BTN_ID_CW:
            case BTN_ID_CC:
                list_rotate_switch();
                break;
            case BTN_ID_LP:
                ui.select[ui.layer] = 0;
            case BTN_ID_SP:
                switch (ui.select[ui.layer]) {
                    case 0: ui.index = M_EDITOR; ui.state = S_LAYER_OUT; break;
                    case 11: window_message_init("Win List Demo", "This is a demo\nof Win List\nanimation!", win_list_demo_menu, M_WIN_LIST_DEMO); break;
                }
                break;
        }
    }
}

//旋钮设置页面初始化
void knob_param_init() {
    check_box_v_init(knob.param);
}

//旋钮旋转页面初始化
void krf_param_init() {
    check_box_s_init(&knob.param[KNOB_ROT], &knob.param[KNOB_ROT_P]);
}

//旋钮按键页面初始化
void kpf_param_init() {
    check_box_s_init(&knob.param[KNOB_COD], &knob.param[KNOB_COD_P]);
}

//设置页面初始化
void setting_param_init() {
    check_box_v_init(ui.param);
    check_box_m_init(ui.param);
    check_box.map = (uint8_t*)setting_param_map;
}

void knob_proc() {
    list_show(knob_menu, M_KNOB);
    if (btn.pressed) {
        btn.pressed = false;
        switch (btn.id) {
            case BTN_ID_CW:
            case BTN_ID_CC:
                list_rotate_switch();
                break;
            case BTN_ID_LP:
                ui.select[ui.layer] = 0;
            case BTN_ID_SP:
                switch (ui.select[ui.layer]) {
                    case 0: ui.index = M_EDITOR; ui.state = S_LAYER_OUT; break;
                    case 1: ui.index = M_KRF; ui.state = S_LAYER_IN; check_box_s_init(&knob.param[KNOB_ROT], &knob.param[KNOB_ROT_P]); break;
                    case 2: ui.index = M_KPF; ui.state = S_LAYER_IN; check_box_s_init(&knob.param[KNOB_COD], &knob.param[KNOB_COD_P]); break;
                }
                break;
        }
    }
}

void krf_proc() {
    list_show(krf_menu, M_KRF);
    if (btn.pressed) {
        btn.pressed = false;
        switch (btn.id) {
            case BTN_ID_CW:
            case BTN_ID_CC:
                list_rotate_switch();
                break;
            case BTN_ID_LP:
                ui.select[ui.layer] = 0;
            case BTN_ID_SP:
                switch (ui.select[ui.layer]) {
                    case 0: ui.index = M_KNOB; ui.state = S_LAYER_OUT; break;
                    case 1: break;
                    case 2: check_box_s_select(KNOB_DISABLE, ui.select[ui.layer]); break;
                    case 3: break;
                    case 4: check_box_s_select(KNOB_ROT_VOL, ui.select[ui.layer]); break;
                    case 5: check_box_s_select(KNOB_ROT_BRI, ui.select[ui.layer]); break;
                    case 6: break;
                }
                break;
        }
    }
}

void kpf_proc() {
    list_show(kpf_menu, M_KPF);
    if (btn.pressed) {
        btn.pressed = false;
        switch (btn.id) {
            case BTN_ID_CW:
            case BTN_ID_CC:
                list_rotate_switch();
                break;
            case BTN_ID_LP:
                ui.select[ui.layer] = 0;
            case BTN_ID_SP:
                switch (ui.select[ui.layer]) {
                    case 0: ui.index = M_KNOB; ui.state = S_LAYER_OUT; break;
                    case 1: break;
                    case 2: check_box_s_select(KNOB_DISABLE, ui.select[ui.layer]); break;
                    case 3: break;
                    case 4: check_box_s_select('A', ui.select[ui.layer]); break;
                    case 5: check_box_s_select('B', ui.select[ui.layer]); break;
                    case 6: check_box_s_select('C', ui.select[ui.layer]); break;
                    case 7: check_box_s_select('D', ui.select[ui.layer]); break;
                    case 8: check_box_s_select('E', ui.select[ui.layer]); break;
                    case 9: check_box_s_select('F', ui.select[ui.layer]); break;
                    case 10: check_box_s_select('G', ui.select[ui.layer]); break;
                    case 11: check_box_s_select('H', ui.select[ui.layer]); break;
                    case 12: check_box_s_select('I', ui.select[ui.layer]); break;
                    case 13: check_box_s_select('J', ui.select[ui.layer]); break;
                    case 14: check_box_s_select('K', ui.select[ui.layer]); break;
                    case 15: check_box_s_select('L', ui.select[ui.layer]); break;
                    case 16: check_box_s_select('M', ui.select[ui.layer]); break;
                    case 17: check_box_s_select('N', ui.select[ui.layer]); break;
                    case 18: check_box_s_select('O', ui.select[ui.layer]); break;
                    case 19: check_box_s_select('P', ui.select[ui.layer]); break;
                    case 20: check_box_s_select('Q', ui.select[ui.layer]); break;
                    case 21: check_box_s_select('R', ui.select[ui.layer]); break;
                    case 22: check_box_s_select('S', ui.select[ui.layer]); break;
                    case 23: check_box_s_select('T', ui.select[ui.layer]); break;
                    case 24: check_box_s_select('U', ui.select[ui.layer]); break;
                    case 25: check_box_s_select('V', ui.select[ui.layer]); break;
                    case 26: check_box_s_select('W', ui.select[ui.layer]); break;
                    case 27: check_box_s_select('X', ui.select[ui.layer]); break;
                    case 28: check_box_s_select('Y', ui.select[ui.layer]); break;
                    case 29: check_box_s_select('Z', ui.select[ui.layer]); break;
                    case 30: break;
                    case 31: check_box_s_select('0', ui.select[ui.layer]); break;
                    case 32: check_box_s_select('1', ui.select[ui.layer]); break;
                    case 33: check_box_s_select('2', ui.select[ui.layer]); break;
                    case 34: check_box_s_select('3', ui.select[ui.layer]); break;
                    case 35: check_box_s_select('4', ui.select[ui.layer]); break;
                    case 36: check_box_s_select('5', ui.select[ui.layer]); break;
                    case 37: check_box_s_select('6', ui.select[ui.layer]); break;
                    case 38: check_box_s_select('7', ui.select[ui.layer]); break;
                    case 39: check_box_s_select('8', ui.select[ui.layer]); break;
                    case 40: check_box_s_select('9', ui.select[ui.layer]); break;
                    case 41: break;
                    case 42: check_box_s_select(KEY_ESC, ui.select[ui.layer]); break;
                    case 43: check_box_s_select(KEY_F1, ui.select[ui.layer]); break;
                    case 44: check_box_s_select(KEY_F2, ui.select[ui.layer]); break;
                    case 45: check_box_s_select(KEY_F3, ui.select[ui.layer]); break;
                    case 46: check_box_s_select(KEY_F4, ui.select[ui.layer]); break;
                    case 47: check_box_s_select(KEY_F5, ui.select[ui.layer]); break;
                    case 48: check_box_s_select(KEY_F6, ui.select[ui.layer]); break;
                    case 49: check_box_s_select(KEY_F7, ui.select[ui.layer]); break;
                    case 50: check_box_s_select(KEY_F8, ui.select[ui.layer]); break;
                    case 51: check_box_s_select(KEY_F9, ui.select[ui.layer]); break;
                    case 52: check_box_s_select(KEY_F10, ui.select[ui.layer]); break;
                    case 53: check_box_s_select(KEY_F11, ui.select[ui.layer]); break;
                    case 54: check_box_s_select(KEY_F12, ui.select[ui.layer]); break;
                    case 55: break;
                    case 56: check_box_s_select(KEY_LEFT_CTRL, ui.select[ui.layer]); break;
                    case 57: check_box_s_select(KEY_LEFT_SHIFT, ui.select[ui.layer]); break;
                    case 58: check_box_s_select(KEY_LEFT_ALT, ui.select[ui.layer]); break;
                    case 59: check_box_s_select(KEY_LEFT_GUI, ui.select[ui.layer]); break;
                    case 60: check_box_s_select(KEY_RIGHT_CTRL, ui.select[ui.layer]); break;
                    case 61: check_box_s_select(KEY_RIGHT_SHIFT, ui.select[ui.layer]); break;
                    case 62: check_box_s_select(KEY_RIGHT_ALT, ui.select[ui.layer]); break;
                    case 63: check_box_s_select(KEY_RIGHT_GUI, ui.select[ui.layer]); break;
                    case 64: break;
                    case 65: check_box_s_select(KEY_CAPS_LOCK, ui.select[ui.layer]); break;
                    case 66: check_box_s_select(KEY_BACKSPACE, ui.select[ui.layer]); break;
                    case 67: check_box_s_select(KEY_RETURN, ui.select[ui.layer]); break;
                    case 68: check_box_s_select(KEY_INSERT, ui.select[ui.layer]); break;
                    case 69: check_box_s_select(KEY_DELETE, ui.select[ui.layer]); break;
                    case 70: check_box_s_select(KEY_TAB, ui.select[ui.layer]); break;
                    case 71: break;
                    case 72: check_box_s_select(KEY_HOME, ui.select[ui.layer]); break;
                    case 73: check_box_s_select(KEY_END, ui.select[ui.layer]); break;
                    case 74: check_box_s_select(KEY_PAGE_UP, ui.select[ui.layer]); break;
                    case 75: check_box_s_select(KEY_PAGE_DOWN, ui.select[ui.layer]); break;
                    case 76: break;
                    case 77: check_box_s_select(KEY_UP_ARROW, ui.select[ui.layer]); break;
                    case 78: check_box_s_select(KEY_DOWN_ARROW, ui.select[ui.layer]); break;
                    case 79: check_box_s_select(KEY_LEFT_ARROW, ui.select[ui.layer]); break;
                    case 80: check_box_s_select(KEY_RIGHT_ARROW, ui.select[ui.layer]); break;
                    case 81: break;
                }
                break;
        }
    }
}

void volt_proc() {
    volt_show();
    if (btn.pressed) {
        btn.pressed = false;
        switch (btn.id) {
            case BTN_ID_CW:
            case BTN_ID_CC:
                list_rotate_switch();
                break;
            case BTN_ID_SP:
            case BTN_ID_LP:
                ui.index = M_MAIN;
                ui.state = S_LAYER_OUT;
                break;
        }
    }
}

void setting_proc() {
    list_show(setting_menu, M_SETTING);
    if (btn.pressed) {
        btn.pressed = false;
        switch (btn.id) {
            case BTN_ID_CW:
            case BTN_ID_CC:
                list_rotate_switch();
                break;
            case BTN_ID_LP:
                ui.select[ui.layer] = 0;
            case BTN_ID_SP:
                switch (ui.select[ui.layer]) {
                    case 0: ui.index = M_MAIN; ui.state = S_LAYER_OUT; break;
                    case 1: window_value_init("Disp Bri", DISP_BRI, &ui.param[DISP_BRI], 1, 0, 1, setting_menu, M_SETTING); break;
                    case 2: check_box_m_select(DARK_MODE); break;
                    case 3: window_value_init("Rotate Scr", ROTATE_SCR, &ui.param[ROTATE_SCR], 3, 0, 1, setting_menu, M_SETTING); break;
                    case 4: window_value_init("Buzzer Vol", BUZ_VOL, &ui.param[BUZ_VOL], 4, 0, 1, setting_menu, M_SETTING); break;
                    case 5: window_value_init("Btn SPT", BTN_SPT, &ui.param[BTN_SPT], 255, 0, 1, setting_menu, M_SETTING); break;
                    case 6: window_value_init("Btn LPT", BTN_LPT, &ui.param[BTN_LPT], 255, 0, 1, setting_menu, M_SETTING); break;
                    case 7: check_box_m_select(KNOB_DIR); break;
                    case 8: check_box_m_select(USB_ENABLE); break;
                    case 9: ui.index = M_ABOUT; ui.state = S_LAYER_IN; break;
                }
                break;
        }
    }
}

void about_proc() {
    about_show();
    if (btn.pressed) {
        btn.pressed = false;
        switch (btn.id) {
            case BTN_ID_SP:
            case BTN_ID_LP:
                ui.index = M_SETTING;
                ui.state = S_LAYER_OUT;
                break;
        }
    }
}

// 初始化列表选择框参数（进入/退出层级时）
static void init_list_box_params() {
    Menu* current_menu = NULL;
    switch (ui.index) {
        case M_MAIN: current_menu = main_menu; break;
        case M_ANIMITION: current_menu = animition_menu; break;
        case M_EDITOR: current_menu = editor_menu; break;
        case M_KNOB: current_menu = knob_menu; break;
        case M_KRF: current_menu = krf_menu; break;
        case M_KPF: current_menu = kpf_menu; break;
        case M_VOLT: current_menu = volt_menu; break;
        case M_SETTING: current_menu = setting_menu; break;
        case M_ABOUT: current_menu = about_menu; break;
        case M_WIN_LIST_DEMO: current_menu = win_list_demo_menu; break;
    }
    if (current_menu) {
        u8g2.setFont(LIST_FONT);
        list.box_W = u8g2.getStrWidth(current_menu[0].title) + LIST_TEXT_S * 2;
    } else {
        list.box_W = 0;
    }
    list.box_w = 0;
    list.box_w_trg = list.box_W;
    list.box_H = LIST_LINE_H;
    list.box_h = 0;
    list.box_h_trg = list.box_H;
    list.box_w_trg += ui.param[BOX_X_OS];
    list.box_h_trg += ui.param[BOX_Y_OS];
}


/********************************** 通用初始化函数 **********************************/

/*
    页面层级管理逻辑是，把所有页面都先当作列表类初始化，不是列表类按需求再初始化对应函数
    这样做会浪费一些资源，但跳转页面时只需要考虑页面层级，逻辑上更清晰，减少出错
*/

//进入更深层级时的初始化
void layer_init_in() {
    ui.layer++;
    ui.select[ui.layer] = 0;
    list.box_y_trg[ui.layer] = 0;
    ui.init = false;
    list.y = 0;
    list.y_trg = LIST_LINE_H;
    list.box_x = 0;
    list.box_y = 0;
    list.bar_y = 0;
    init_list_box_params();
    ui.fade = 1;
    ui.state = S_FADE;
    switch (ui.index) {
        case M_MAIN: tile_param_init(); break;
        case M_ANIMITION: animition_param_init(); break;
        case M_KNOB: knob_param_init(); break;
        case M_KRF: krf_param_init(); break;
        case M_KPF: kpf_param_init(); break;
        case M_VOLT: volt_param_init(); break;
        case M_SETTING: setting_param_init(); break;
        case M_ABOUT: about_param_init(); break;
    }
}

//进入更浅层级时的初始化
void layer_init_out() {
    ui.select[ui.layer] = 0;
    list.box_y_trg[ui.layer] = 0;
    ui.layer--;
    ui.init = false;
    list.y = 0;
    list.y_trg = LIST_LINE_H;
    list.bar_y = 0;
    init_list_box_params();
    ui.fade = 1;
    ui.state = S_FADE;
    switch (ui.index) {
        case M_SLEEP:
            sleep_param_init();
            break;
        case M_MAIN:
            buzzer_exit_sound();
            led_set_red();
            tile_param_init();
            break;
        default:
            buzzer_exit_sound();
            break;
    }
}



void ui_proc() {
    static const u8g2_cb_t *rot_table[4] = { U8G2_R0, U8G2_R1, U8G2_R2, U8G2_R3 };
    u8g2.setDisplayRotation(rot_table[ui.param[ROTATE_SCR] & 3u]);

    switch (ui.state) {
        case S_FADE:          fade();                   break;
        case S_WINDOW:        window_param_init();      break;
        case S_LAYER_IN:      layer_init_in();          break;
        case S_LAYER_OUT:     layer_init_out();         break;

        case S_NONE:
            u8g2.clearBuffer();
            switch (ui.index) {
                case M_WINDOW: window_proc(); break;
                case M_SLEEP: sleep_proc(); break;
                case M_MAIN: main_proc(); break;
                case M_ANIMITION: animition_proc(); break;
                case M_EDITOR: editor_proc(); break;
                case M_WIN_LIST_DEMO: win_list_demo_proc(); break;
                case M_KNOB: knob_proc(); break;
                case M_KRF: krf_proc(); break;
                case M_KPF: kpf_proc(); break;
                case M_VOLT: volt_proc(); break;
                case M_SETTING: setting_proc(); break;
                case M_ABOUT: about_proc(); break;
            }
            break;
    }

    u8g2.sendBuffer();
}
