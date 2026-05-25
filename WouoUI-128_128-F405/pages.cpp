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

/************************************* TIM12蜂鸣器寄存器（睡眠模式用） *************************************/

/*
 * 睡眠模式下需要直接控制蜂鸣器PWM
 * 此处引用TIM12的寄存器，与knob.cpp中的定义一致
 * 用于sleep_proc()中进入睡眠时关闭蜂鸣器
 */
#define TIM12_BASE  0x40001800u
#define TIM12_CR1   (*(volatile uint32_t *)(TIM12_BASE + 0x00u))
#define TIM12_CCR1  (*(volatile uint32_t *)(TIM12_BASE + 0x34u))

/************************************* 开发板模拟引脚映射 *************************************/

/*
 * STM32F405开发板可用ADC引脚映射
 * 用于电压测量页面选择ADC通道
 * 按数组索引0~9对应M_VOLT页面的10个菜单项
 */
uint8_t analog_pin[10] = {PA0, PA1, PA2, PA3, PA4, PA5, PA6, PA7, PB0, PB1};

/************************************* 高亮条动画包装函数 *************************************/

/*
 * 高亮条动画模式包装函数
 * 
 * 根据HL_ANI_MODE参数选择动画算法：
 *   mode=0：Ease - 指数缓动，平滑自然
 *   mode=1：Spring - 弹簧物理，弹性跟随
 *   mode=2：Bounce - 阻尼振荡，弹跳效果
 * 
 * Spring和Bounce模式使用SPRING_K（刚度）和SPRING_D（阻尼）参数
 * 所有模式均使用统一的n参数（对应ui.param中的速度索引）
 */
/*
 * 级联高亮条动画包装函数
 * 
 * 用于需要两级级联的动画场景：
 * - 第一级：当前值a向过渡目标a_trg运动
 * - 第二级：过渡目标a_trg向最终目标a_final运动
 * 
 * 这种级联设计实现"过头再回弹"的效果
 * 例如选择框宽度变化时，先略微扩大再缩回目标大小
 * Spring模式时两级使用不同的刚度和阻尼系数，产生层次感
 */
static void hl_ani_cascade(float *a, float *a_trg, float *a_final, float *vel, float *vel_trg, uint8_t n) {
    if (ui.param[HL_ANI_MODE] == 0) {
        animation(a, a_trg, n);
        animation(a_trg, a_final, n);
    } else if (ui.param[HL_ANI_MODE] == 1) {
        animation_spring(a_trg, a_final, vel_trg, (ui.param[SPRING_K] / 100.0f) * 1.4f, (ui.param[SPRING_D] / 100.0f) * 0.857f);
        animation_spring(a, a_trg, vel, (ui.param[SPRING_K] / 100.0f) * 1.8f, (ui.param[SPRING_D] / 100.0f) * 1.071f);
    } else if (ui.param[HL_ANI_MODE] == 2) {
        animation_bounce(a_trg, a_final, vel_trg, n);
        animation_bounce(a, a_trg, vel, n);
    } else {
        animation_gravity(a_trg, a_final, vel_trg, n);
        animation_gravity(a, a_trg, vel, n);
    }
}


/************************************* 复选框/数值 行尾控件处理 *************************************/

/*
 * 各函数根据行首符号判断行尾绘制内容：
 * '~' = 数值显示（调用list_draw_value）
 * '+' = 多选框（调用list_draw_check_box_frame/dot）
 * '=' = 单选框（调用list_draw_check_box_frame/dot）
 * '#' = 旋钮功能文字（调用list_draw_krf）
 * '$' = 按键键值文字（调用list_draw_kpf）
 * '*' = 下拉选择文字（直接内联处理）
 * '-' = 仅文字，无行尾控件
 */

/*
 * 数值复选框初始化
 * 将参数数组指针绑定到check_box.v
 * 用于列表显示时读取参数数值
 */
void check_box_v_init(uint8_t *param) {
    check_box.v = param;
}

/*
 * 多选复选框初始化
 * 将多选框状态数组指针绑定到check_box.m
 */
void check_box_m_init(uint8_t *param) {
    check_box.m = param;
}

/*
 * 单选复选框初始化
 * 将单选框值和位置指针绑定到check_box
 */
void check_box_s_init(uint8_t *param, uint8_t *param_p) {
    check_box.s = param;
    check_box.s_p = param_p;
}

/*
 * 多选框切换
 * 切换指定参数的选中状态
 * 设置eeprom.change标志以便在睡眠时保存
 */
void check_box_m_select(uint8_t param) {
    check_box.m[param] = !check_box.m[param];
    eeprom.change = true;
}

/*
 * 单选框选择
 * 设置单选框的值和位置
 * 设置eeprom.change标志以便在睡眠时保存
 */
void check_box_s_select(uint8_t val, uint8_t pos) {
    *check_box.s = val;
    *check_box.s_p = pos;
    eeprom.change = true;
}

/************************************* 行尾控件绘制函数 *************************************/

/*
 * 列表行尾显示数值
 * 在行末尾显示check_box.v数组中的对应数值
 * 用于"~ Disp Bri"等数值调节项
 */
void list_draw_value(int n) {
    u8g2.print(check_box.v[n - 1]);
}

/*
 * 绘制复选框外框
 * 在行末尾绘制矩形外框
 * 位置由CHECK_BOX_L_S和CHECK_BOX_U_S定义
 * 弯曲偏移list.curve使外框随列表弯曲
 */
void list_draw_check_box_frame() {
    u8g2.drawRFrame(CHECK_BOX_L_S + list.curve, list.temp + CHECK_BOX_U_S, CHECK_BOX_F_W, CHECK_BOX_F_H, 1);
}

/*
 * 绘制复选框内部填充点
 * 在复选框外框内部绘制实心方块表示选中状态
 * 内边距CHECK_BOX_D_S使填充点小于外框
 */
void list_draw_check_box_dot() {
    u8g2.drawBox(CHECK_BOX_L_S + CHECK_BOX_D_S + 1 + list.curve, list.temp + CHECK_BOX_U_S + CHECK_BOX_D_S + 1, CHECK_BOX_F_W - (CHECK_BOX_D_S + 1) * 2, CHECK_BOX_F_H - (CHECK_BOX_D_S + 1) * 2);
}

/*
 * 列表行尾显示旋钮旋转功能
 * 将存储的功能值（0/1/2）转为可读文字
 * 0=OFF 1=VOL（音量） 2=BRI（亮度）
 */
void list_draw_krf(int n) {
    switch (check_box.v[n - 1]) {
        case 0: u8g2.print("OFF"); break;
        case 1: u8g2.print("VOL"); break;
        case 2: u8g2.print("BRI"); break;
    }
}

/*
 * 列表行尾显示按键键值
 * 将存储的HID键码转为可读字符
 * 0=OFF 1~90=ASCII字符 其他值=?
 */
void list_draw_kpf(int n) {
    if (check_box.v[n - 1] == 0) u8g2.print("OFF");
    else if (check_box.v[n - 1] <= 90) u8g2.print((char)check_box.v[n - 1]);
    else u8g2.print("?");
}

/************************************* 行尾控件分发绘制 *************************************/

/*
 * 根据行首符号在行尾绘制对应控件
 * 
 * 行首符号决定行尾绘制内容：
 * '~' → 数值（调用list_draw_value）
 * '+' → 多选框（先外框，选中再画点）
 * '=' → 单选框（先外框，选中位置匹配再画点）
 * '#' → 旋钮功能文字（OFF/VOL/BRI）
 * '$' → 按键键值文字（字符或OFF）
 * '*' → 高亮动画模式下拉选择（Ease/Spring/Bounce）
 * 
 * check_box.map用于将菜单项位置映射到参数索引
 * 如果map不为空，则使用map[i-1]作为参数索引
 * 否则直接用i-1作为参数索引
 */
void list_draw_text_and_check_box(Menu* arr, int i) {
    u8g2.drawStr(LIST_TEXT_S + list.curve, list.temp + LIST_TEXT_H + LIST_TEXT_S, arr[i].title);
    u8g2.setCursor(CHECK_BOX_L_S + list.curve, list.temp + LIST_TEXT_H + LIST_TEXT_S);
    switch (arr[i].title[0]) {
        case '~': if (check_box.map) list_draw_value(check_box.map[i - 1] + 1); else list_draw_value(i); break;
        case '+': list_draw_check_box_frame(); if (check_box.map) { if (check_box.m[check_box.map[i - 1]] == 1) list_draw_check_box_dot(); } else { if (check_box.m[i - 1] == 1) list_draw_check_box_dot(); } break;
        case '=': list_draw_check_box_frame(); if (*check_box.s_p == i) list_draw_check_box_dot(); break;
        case '#': list_draw_krf(i); break;
        case '$': list_draw_kpf(i); break;
        case '*': { static const char* hl_labels[] = {"Ease", "Spring", "Bounce", "Gravity"}; uint8_t pi = check_box.map ? check_box.map[i - 1] : (uint8_t)(i - 1); u8g2.print(hl_labels[ui.param[pi]]); } break;
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

static const char* hl_ani_mode_items[] = { "Ease", "Spring", "Bounce", "Gravity" };
static void hl_ani_callback(uint8_t select) {
    ui.param[HL_ANI_MODE] = select;
}

// Animi 页面：菜单项位置 → 参数索引映射
// 菜单项 1~20 对应的 ParamIndex
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
    23,  // 17: Win Stretch → WIN_STYLE
    25,  // 18: HL Ani Mode → HL_ANI_MODE
    26,  // 19: Spring K → SPRING_K
    27   // 20: Spring D → SPRING_D
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
    tile.icon_x_vel = 0;
    tile.icon_y_vel = 0;
    tile.indi_x_vel = 0;
    tile.title_y_vel = 0;
    led_set_red();  // 进入主菜单时显示红色
}


/************************************* 磁贴类页面显示函数 *************************************/

/*
 * 磁贴类页面通用显示函数
 * 
 * 绘制"大标题 + 小标题 + 图标行 + 指示器"的磁贴布局
 * 用于主菜单等页面
 * 
 * 显示布局（从上到下）：
 * 1. 图标行（位于屏幕上部，左右滑动切换）
 * 2. 大标题（位于图标下方，显示当前选中项名称）
 * 3. 大标题指示器（大标题左侧的竖条）
 * 4. 小标题（位于屏幕底部，对当前选中项进行补充说明）
 * 
 * 动画效果：
 * - 图标行：水平滑动（icon_x）和垂直滑入（icon_y）
 * - 指示器：水平展开（indi_x）
 * - 标题：垂直飞入（title_y）
 * 
 * 参数：
 *   arr_1 - 大标题菜单数据
 *   arr_2 - 小标题菜单数据
 *   icon_pic - 图标位图数组（每图标16×18像素）
 */
void tile_show(Menu* arr_1, Menu* arr_2, const uint8_t icon_pic[][16 * 18]) {
    //计算动画过渡值
    hl_ani(&tile.icon_x, &tile.icon_x_trg, &tile.icon_x_vel, TILE_ANI);
    hl_ani(&tile.icon_y, &tile.icon_y_trg, &tile.icon_y_vel, TILE_ANI);
    hl_ani(&tile.indi_x, &tile.indi_x_trg, &tile.indi_x_vel, TILE_ANI);
    hl_ani(&tile.title_y, &tile.title_y_trg, &tile.title_y_vel, TILE_ANI);

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

/*
 * 旋钮逆时针/顺时针旋转切换逻辑（磁贴类页面）
 * 
 * 处理磁贴图标的左右滑动选择：
 * - BTN_ID_CC：左移（选中项索引减1）
 * - BTN_ID_CW：右移（选中项索引加1）
 * - 循环模式（TILE_LOOP）：到达边界时跳转到另一端
 * - 非循环模式：到达边界时停止并置位select_flag（阻止指示器/标题复位）
 * 
 * select_flag机制：
 * - false：移动进行中，允许指示器和标题动画
 * - true：移动完成或到达边界，复位指示器和标题位置
 */

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

/*
 * 列表类页面选择切换逻辑
 * 
 * 处理列表上下滚动选择：
 * - BTN_ID_CC：上移（选中项索引减1）
 * - BTN_ID_CW：下移（选中项索引加1）
 * 
 * 边界处理：
 * - 到达列表顶部/底部时，如果LIST_LOOP开启则循环跳转
 * - 否则停留在边界（不响应操作）
 * 
 * 滚动逻辑：
 * - 使用list.y（列表整体垂直偏移）和list.box_y（选择框垂直位置）的组合
 * - 当选中项在可视区域外时，滚动列表使选中项回到可视区域
 * - DISP_H % LIST_LINE_H ≠ 0时处理多出的剩余像素
 * - list.loop标志在动画完成前阻止其他操作
 * 
 * 更新选择框尺寸以匹配新选中项的文字宽度
 */
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


/*
 * 更新选择框尺寸（含过度延伸）
 * 
 * 根据当前选中项的标题文字宽度计算选择框的目标尺寸
 * 过度延伸（BOX_X_OS/BOX_Y_OS）使选择框在动画中产生"过头再回弹"的弹性效果
 * 
 * 选择框最终尺寸：
 * - 宽度 = 标题文字宽度 + LIST_TEXT_S × 2 + BOX_X_OS
 * - 高度 = LIST_LINE_H + BOX_Y_OS
 */
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

/*
 * 列表类页面通用显示函数
 * 
 * 绘制带弯曲效果的滚动列表：
 * - 左侧：文字列表（支持弯曲效果CURVE）
 * - 右侧：滚动条指示器
 * - 选择框：覆盖在当前选中项上的圆角矩形
 * 
 * 行尾控件（根据行首符号）：
 * '~' 数值 '+' 多选框 '=' 单选框
 * '#' 旋钮功能 '$' 按键键值 '*' 下拉选择
 * 
 * 显示流程：
 * 1. 更新动画目标值
 * 2. 计算所有动画过渡值
 * 3. 遍历菜单项绘制每行文字和行尾控件
 * 4. 绘制右侧滚动条
 * 5. 覆盖选择框（异或色2实现反色效果）
 * 6. 白天模式时做全屏遮罩
 * 
 * 参数：
 *   arr - 菜单数据结构体数组
 *   ui_index - 当前页面的索引（M_MAIN/M_EDITOR等）
 */
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
    hl_ani_cascade(&list.box_w, &list.box_w_trg, &list.box_W, &list.box_w_vel, &list.box_w_vel_trg, LIST_ANI);
    hl_ani(&list.box_y, &list.box_y_trg[ui.layer], &list.box_y_vel, LIST_ANI);
    hl_ani_cascade(&list.box_h, &list.box_h_trg, &list.box_H, &list.box_h_vel, &list.box_h_vel_trg, LIST_ANI);
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

/*
 * 电压测量页面初始化
 * 
 * 设置电压文字背景覆盖动画的初始状态
 * text_bg_l从0→DISP_W展开，覆盖电压值显示区域
 */

/*
 * 电压测量页面显示函数
 * 电压测量页面显示函数
 * 
 * 显示布局：
 * - 上部：电压波形图框（WAVE_BOX_W × WAVE_BOX_H）
 * - 中部：选择框（选中的ADC通道名称）
 * - 下部：电压数值显示
 * 
 * 波形绘制：
 * - 每次刷新采样 WAVE_W × 5 个ADC值
 * - 映射到WAVE_MAX~WAVE_MIN像素范围
 * - 绘制折线图显示电压变化趋势
 * - 仅在选择框和列表动画完成后才更新波形（避免闪烁）
 * 
 * ADC读取：
 * - 使用analog_pin[ui.select[ui.layer]]选择通道
 * - 返回值0~4095映射到0~3.3V
 */
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
  hl_ani(&list.box_x, &list.box_x_trg, &list.box_x_vel, LIST_ANI);
  hl_ani(&list.box_y, &list.box_y_trg[ui.layer], &list.box_y_vel, LIST_ANI);
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

/*
 * 关于本机页面初始化
 * 
 * 设置指示器展开动画的初始状态
 * indi_x从0→ABOUT_INDI_S展开显示信息区域
 */
void about_param_init() {
    about.indi_x = 0;
    about.indi_x_trg = ABOUT_INDI_S;
}

/*
 * 关于本机页面显示函数
 * 
 * 显示布局：
 * - 第1行：右侧的选择框 + "About"标题
 * - 第2行：设备名称（如"WouoUI"）
 * - 第3行起：设备详情信息列表
 * - 左侧指示器竖条
 * 
 * 使用TAG_ANI动画速度控制选择框和指示器动画
 */
void about_show() {
    u8g2.setFont(LIST_FONT);
    list.box_x_trg = u8g2.getStrWidth(about_menu[0].title) + LIST_TEXT_S * 2;

    hl_ani(&list.box_x, &list.box_x_trg, &list.box_x_vel, TAG_ANI);
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

/*
 * 动画设置页面初始化
 * 
 * 绑定ui.param数组作为数值/复选框的数据源
 * 使用animi_param_map将菜单项位置映射到参数索引
 * 使第i个菜单项对应map[i-1]号参数
 */
void animition_param_init() {
    check_box_v_init(ui.param);
    check_box_m_init(ui.param);
    check_box.map = (uint8_t*)animi_param_map;
}

/*
 * 动画设置页面主循环
 * 
 * 列表显示所有动画参数调节项：
 * - 1~10：速度参数（数值弹窗调节）
 * - 11~17：开关选项（复选框切换）
 * - 18：动画模式选择（下拉选择弹窗）
 * - 19~20：弹簧物理参数（数值弹窗调节）
 * 
 * LP（长按）：返回上级
 * SP（短按）：数值调节打开弹窗，开关项直接切换
 */
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
                    case 18: window_list_select_init("HL Ani Mode", hl_ani_mode_items, 4, animition_menu, M_ANIMITION); window_set_list_callback(hl_ani_callback); break;
                    case 19: window_value_init("Spring K", SPRING_K, &ui.param[SPRING_K], 100, 10, 1, animition_menu, M_ANIMITION); break;
                    case 20: window_value_init("Spring D", SPRING_D, &ui.param[SPRING_D], 100, 10, 1, animition_menu, M_ANIMITION); break;
                }
                break;
        }
    }
}

/*
 * 进入睡眠模式时的初始化
 * 
 * 执行步骤：
 * 1. 清除屏幕并进入省电模式（关闭显示驱动）
 * 2. 关闭蜂鸣器PWM（TIM12_CCR1=0，TIM12_CR1清除使能位）
 * 3. 如果USB已连接，先断开USB并保存磁盘数据
 * 4. 如果有参数变更（eeprom.change），写入Flash EEPROM
 * 5. 启动白色呼吸灯指示睡眠状态
 *
 * 睡眠模式下的操作：
 * - 旋钮旋转：发送HID音量/亮度增减（如果HID_ENABLE）
 * - 短按：发送HID按键（所选键码）
 * - 长按（BTN_ID_LP）：唤醒设备
 */
/*
 * 睡眠模式主循环
 * 
 * 在ui.sleep=true时持续运行，循环执行：
 * 1. btn_scan() - 轮询按键
 * 2. buzzer_proc() - 后台蜂鸣器处理
 * 3. led_proc() - LED呼吸灯更新
 * 
 * 按键响应：
 * - CW：增加音量或亮度（走USB HID）
 * - CC：减小音量或亮度（走USB HID）
 * - SP：短按发送预设键码（走USB HID键盘）
 * - LP：唤醒设备，关闭呼吸灯，若USB_ENABLE开启则启动USB
 * 
 * 退出条件：长按唤醒后ui.sleep=false
 */
void sleep_proc() {
    if (!ui.sleep) {
        u8g2.setDrawColor(0);
        u8g2.drawBox(0, 0, DISP_W, DISP_H);
        u8g2.sendBuffer();
        u8g2.setPowerSave(1);
        ui.sleep = true;

        TIM12_CCR1 = 0;
        TIM12_CR1 &= ~(1u << 0);

        if (USBManager::isEnabled()) {
            USBManager::end();
        }

        if (eeprom.change) {
            led_set_white();
            eeprom_write_all_data();
            eeprom.change = false;
        }

        led_start_breathing_white();
    }
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

/*
 * 主菜单磁贴页面主循环
 * 
 * 显示5个磁贴图标：
 * 0=Sleep 1=Editor 2=Voltage 3=Animation 4=Setting
 * 
 * SP（短按）：进入选中磁贴对应的子页面
 * LP（长按）：功能同SP（未区分）
 * 
 * 选择动画复位：
 * 当tile.select_flag=false且动画已完成（ui.init=true）时
 * 将指示器（indi_x）和标题（title_y）复位到初始位置
 * 为下一次选择切换做准备
 */
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

static void conf_test_callback(bool confirmed) {
}

/*
 * 编辑器页面主循环
 * 
 * 功能测试页面，用于演示各种弹窗类型：
 * - 8：确认弹窗（Conf Test）
 * - 9：列表选择弹窗（Select Item）
 * - 10：消息弹窗（Message）
 * - 11：进入旋钮设置子页面
 * 
 * LP（长按）：返回主菜单，选中项重置为0
 */
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
                    case 8: window_confirm_init("Conf Test", "Are you sure?\nChoose Yes or No.", editor_menu, M_EDITOR, conf_test_callback); break;
                    case 9: window_list_select_init("Select Item", win_list_test_items, WIN_LIST_TEST_ITEMS_NUM, editor_menu, M_EDITOR); break;
                    case 10: window_message_init("Message", "Hello World!\nLine 2\nLine 3", editor_menu, M_EDITOR); break;
                    case 11: ui.index = M_KNOB; ui.state = S_LAYER_IN; break;
                }
                break;
        }
    }
}


/********************************* 参数初始化函数 *********************************/

/*
 * 旋钮设置页面参数初始化
 * 
 * 绑定knob.param数组作为数值显示的数据源
 * 用于列表显示旋钮当前的功能配置
 */
void knob_param_init() {
    check_box_v_init(knob.param);
}

/*
 * 旋钮旋转功能页面参数初始化
 * 
 * 使用单选框模式绑定旋钮旋转功能参数
 * 用户从列表中选择一个功能（OFF/VOL/BRI）
 * s_init将单选值和位置指针绑定到knob.param中的对应位
 */
void krf_param_init() {
    check_box_s_init(&knob.param[KNOB_ROT], &knob.param[KNOB_ROT_P]);
}

/*
 * 旋钮按键功能页面参数初始化
 * 
 * 使用单选框模式绑定旋钮按键功能参数
 * 用户从列表中选择一个按键键码
 * s_init将单选值和位置指针绑定到knob.param中的对应位
 */
void kpf_param_init() {
    check_box_s_init(&knob.param[KNOB_COD], &knob.param[KNOB_COD_P]);
}

/*
 * 设置页面参数初始化
 * 
 * 绑定ui.param数组作为数值/复选框的数据源
 * 使用setting_param_map将菜单项位置映射到参数索引
 * 使第i个菜单项对应map[i-1]号参数
 */
void setting_param_init() {
    check_box_v_init(ui.param);
    check_box_m_init(ui.param);
    check_box.map = (uint8_t*)setting_param_map;
}

/********************************* 页面主循环函数 *********************************/

/*
 * 旋钮设置页面主循环
 * 
 * 显示旋钮功能配置列表：
 * - 0：返回编辑器
 * - 1：进入旋钮旋转功能设置
 * - 2：进入旋钮按键功能设置
 * 
 * LP（长按）：返回上级，选中项重置
 */
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

/*
 * 旋钮旋转功能设置页面主循环
 * 
 * 以单选框列表选择旋钮旋转对应的功能：
 * - OFF：旋钮旋转无功能
 * - VOL：控制系统音量
 * - BRI：控制屏幕亮度
 * 
 * 选择项分隔行（奇数项）不可选，仅做分类显示
 */
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

/*
 * 旋钮按键功能设置页面主循环
 * 
 * 以单选框列表选择旋钮短按时发送的HID键码：
 * - 可选项：字母A-Z、数字0-9、功能键、修饰键、方向键
 * - 分隔行（特定位置）不可选，用于分类显示
 * 
 * 选中项的键码通过HID键盘报告发送
 * 用于在睡眠模式下通过旋钮按键控制电脑
 */
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

/*
 * 电压测量页面主循环
 * 
 * 显示ADC电压波形和数值：
 * - 通过列表选择ADC通道（PA0~PB1共10通道）
 * - 实时绘制电压波形图
 * - 显示当前电压数值
 * 
 * SP/LP（短按/长按）：返回主菜单
 */
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

/*
 * 系统设置页面主循环
 * 
 * 显示所有可调节的系统参数：
 * - 1：屏幕亮度（Disp Bri）
 * - 2：暗色模式开关
 * - 3：屏幕旋转（0°/90°/180°/270°）
 * - 4：蜂鸣器音量
 * - 5~6：按键短按/长按时长
 * - 7：旋钮方向切换
 * - 8：USB存储开关
 * - 9：关于本机页面
 * 
 * LP（长按）：返回主菜单，选中项重置
 * SP（短按）：数值调节打开弹窗，开关项直接切换
 */
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

/*
 * 关于本机页面主循环
 * 
 * 显示设备名称、版本信息和详情：
 * - 第1行："About"标签
 * - 第2行：设备名称（如"WouoUI"）
 * - 第3行起：版本号、固件日期、作者等信息
 * 
 * SP/LP：返回系统设置页面
 */
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

/*
 * 初始化列表选择框参数（进入/退出层级时）
 * 
 * 根据当前页面的第一个菜单项的文字宽度，设置选择框的初始目标尺寸
 * 同时重置所有动画变量（位置、速度）为初始值
 * 确保每次层级切换时选择框动画从正确状态开始
 */
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
    list.box_y_vel = 0;
    list.box_x_vel = 0;
    list.box_w_vel = 0;
    list.box_w_vel_trg = 0;
    list.box_h_vel = 0;
    list.box_h_vel_trg = 0;
}


/********************************** 通用初始化函数 **********************************/

/*
 * 页面层级管理策略：
 * 所有页面先按列表类初始化（box_x/box_y等），
 * 不是列表类的页面（如磁贴、关于等）在各自的分支中覆盖初始化
 * 
 * 这样做会浪费一些资源（初始化了用不到的变量），
 * 但使页面跳转逻辑统一，只需考虑层级变化，代码更清晰
 */

/*
 * 进入更深层级时的初始化
 * 
 * 步骤：
 * 1. ui.layer层级递增
 * 2. 当前层级选中项置0
 * 3. 重置所有列表/动画状态变量
 * 4. 调用init_list_box_params()设置选择框初始尺寸
 * 5. 设置淡入动画状态S_FADE
 * 6. 调用当前页面的专用param_init()
 * 
 * M_SLEEP页面不走此处（睡眠由layer_init_out进入）
 */
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

/*
 * 进入更浅层级时的初始化（返回上级页面）
 * 
 * 步骤：
 * 1. 记录当前层级的选中项和选择框位置
 * 2. ui.layer层级递减
 * 3. 重置动画状态变量
 * 4. 调用init_list_box_params()设置选择框初始尺寸
 * 5. 设置淡入动画状态S_FADE
 * 6. 根据目标页面执行专用初始化：
 *    - M_SLEEP：调用sleep_param_init()进入睡眠
 *    - M_MAIN：播放退出音效，LED重置红色，初始化磁贴
 *    - 其他：仅播放退出音效
 */
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



/*
 * UI主循环入口
 * 
 * 每个主循环周期调用一次，执行流程：
 * 1. 根据ROTATE_SCR参数设置屏幕旋转方向
 * 2. 根据当前状态执行对应处理：
 *    - S_FADE：淡入/淡出动画
 *    - S_WINDOW：弹窗初始化
 *    - S_LAYER_IN：进入子页面初始化
 *    - S_LAYER_OUT：返回父页面初始化
 *    - S_NONE：页面主循环，根据ui.index分发到各页面处理函数
 * 3. 发送缓冲区数据到显示驱动
 * 
 * 页面分发（S_NONE状态）：
 * - M_WINDOW：弹窗处理
 * - M_SLEEP：睡眠模式
 * - M_MAIN：主菜单磁贴
 * - M_ANIMITION：动画参数设置
 * - M_EDITOR：编辑器/测试页
 * - M_KNOB/KRF/KPF：旋钮设置
 * - M_VOLT：电压测量
 * - M_SETTING：系统设置
 * - M_ABOUT：关于本机
 */
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
