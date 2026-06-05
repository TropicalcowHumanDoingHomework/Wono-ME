#include "window.h"
#include "ui_state.h"
#include "animation.h"
#include "display.h"
#include "eeprom_manager.h"
#include "pages.h"
#include "knob.h"

/************************************* 弹窗相关 *************************************/

/*
 * 数值调节弹窗初始化
 * 
 * 创建带进度条和数值显示的调节弹窗
 * 支持WIN_STYLE=1时的拉伸展开动画效果
 * 
 * 参数：
 *   title - 弹窗标题（如"Disp Bri"）
 *   select - 初始选中项
 *   value - 指向要调节的数值变量的指针
 *   max - 最大值
 *   min - 最小值
 *   step - 步进值
 *   bg - 弹窗背景菜单指针
 *   index - 弹窗关闭后返回的页面索引
 */
void window_value_init(const char title[], uint8_t select, uint8_t *value, uint8_t max, uint8_t min, uint8_t step, Menu *bg, uint8_t index) {
    strcpy(win.title, title);
    win.select = select;
    win.value = value;
    win.max = max;
    win.min = min;
    win.step = step;
    win.bg = bg;
    win.index = index;
    ui.index = M_WINDOW;
    window_param_init();

    if (ui.param[WIN_STYLE]) {
        win.box_H = WIN_H;
        win.box_h = 0;
        win.box_h_trg = WIN_H + ui.param[WIN_Y_OS];
        win.w = DISP_W;
        win.w_trg = WIN_W;
    }
}

/*
 * 消息弹窗初始化
 * 
 * 创建纯文本消息提示弹窗
 * 支持多行消息（使用\n换行）
 * 自动计算弹窗宽高以适配文本内容
 * 
 * 参数：
 *   title - 弹窗标题
 *   message - 消息文本（支持\n换行）
 *   bg - 弹窗背景菜单指针
 *   index - 弹窗关闭后返回的页面索引
 */
void window_message_init(const char title[], const char message[], Menu *bg, uint8_t index) {
    strcpy(win.title, title);
    strcpy(win.message, message);
    win.msg_mode = 1;
    win.bg = bg;
    win.index = index;
    ui.index = M_WINDOW;

    u8g2.setFont(WIN_FONT);
    uint8_t max_w = u8g2.getStrWidth(win.title);
    char msg_copy[128];
    strcpy(msg_copy, win.message);
    char* line = strtok(msg_copy, "\n");
    uint8_t line_count = 0;
    while (line) {
        uint8_t line_w = u8g2.getStrWidth(line);
        if (line_w > max_w) max_w = line_w;
        line_count++;
        line = strtok(NULL, "\n");
    }

    win.w = win.w_trg = max_w + WIN_MSG_PAD * 2;
    if (win.w_trg < 60) win.w = win.w_trg = 60;
    win.h = win.h_trg = WIN_MSG_PAD * 2 + LIST_TEXT_H + line_count * LIST_LINE_H;

    if (ui.param[WIN_STYLE]) {
        win.box_H = win.h;
        win.box_h = 0;
        win.box_h_trg = win.h + ui.param[WIN_Y_OS];
        win.w = DISP_W;
    }

    win.l = (DISP_W - win.w) / 2;
    win.y = -win.h - 2;
    win.y_trg = (DISP_H - win.h) / 2;
    win.u = win.y_trg;
    ui.state = S_NONE;
}

/*
 * 列表选择弹窗初始化
 * 
 * 创建可选列表弹窗，支持滚动和选择高亮
 * 最多显示WIN_LIST_MAX项
 * 
 * 参数：
 *   title - 弹窗标题
 *   items - 字符串数组，每项为一个选择
 *   item_count - 选项数量
 *   bg - 弹窗背景菜单指针
 *   index - 弹窗关闭后返回的页面索引
 */
void window_list_select_init(const char title[], const char* items[], uint8_t item_count, Menu *bg, uint8_t index, uint8_t default_select) {
    strcpy(win.title, title);
    win.list_mode = 1;
    win.list_count = item_count < WIN_LIST_MAX ? item_count : WIN_LIST_MAX;
    for (uint8_t i = 0; i < win.list_count; i++) {
        strncpy(win.list_items[i], items[i], WIN_LIST_ITEM_LEN - 1);
        win.list_items[i][WIN_LIST_ITEM_LEN - 1] = '\0';
    }
    win.list_select = default_select < win.list_count ? default_select : 0;
    win.hl_sel_cur = (float)win.list_select;
    win.hl_sel_trg = (float)win.list_select;
    win.list_y = 0;
    win.list_y_trg = 0;
    win.hl_vel = 0;
    win.list_vel = 0;
    win.bg = bg;
    win.index = index;
    win.msg_mode = 0;
    ui.index = M_WINDOW;

    u8g2.setFont(WIN_FONT);
    uint8_t max_w = u8g2.getStrWidth(win.title);
    for (uint8_t i = 0; i < win.list_count; i++) {
        uint8_t item_w = u8g2.getStrWidth(win.list_items[i]);
        if (item_w > max_w) max_w = item_w;
        if (item_w + WIN_MSG_PAD * 2 + 4 > 90) max_w = 90 - WIN_MSG_PAD * 2 - 4;
    }

    win.w = win.w_trg = max_w + WIN_MSG_PAD * 2 + 4;
    if (win.w_trg < 60) win.w = win.w_trg = 60;
    if (win.w_trg > 106) win.w = win.w_trg = 106;

    uint8_t item_area_h = (DISP_H - 20) - WIN_MSG_PAD * 3 - LIST_TEXT_H;
    uint8_t item_rows = item_area_h / LIST_LINE_H;
    if (item_rows > 6) item_rows = 6;
    win.h = win.h_trg = WIN_MSG_PAD * 3 + LIST_TEXT_H + item_rows * LIST_LINE_H;
    if (win.h > DISP_H - 8) win.h = win.h_trg = DISP_H - 8;

    if (ui.param[WIN_STYLE]) {
        win.box_H = win.h;
        win.box_h = 0;
        win.box_h_trg = win.h + ui.param[WIN_Y_OS];
        win.w = DISP_W;
    }

    win.l = (DISP_W - win.w) / 2;
    win.y = -win.h - 2;
    win.y_trg = (DISP_H - win.h) / 2;
    if (win.y_trg < 2) win.y_trg = 2;
    win.u = win.y_trg;

    if (win.list_select >= item_rows)
        win.list_y = win.list_y_trg = -(win.list_select - item_rows + 1) * LIST_LINE_H;

    ui.state = S_NONE;
}

/*
 * 弹窗参数初始化
 * 
 * 重置弹窗的各项参数为默认值
 * 包括：模式标志、位置、尺寸、动画参数
 * 根据WIN_STYLE参数选择弹窗样式（标准/拉伸）
 */
void window_param_init() {
    win.msg_mode = 0;
    win.list_mode = 0;
    win.list_on_close = nullptr;
    win.bokeh_step = 0;
    win.last_bokeh_time = 0;
    win.bar = 0;
    win.y = WIN_Y;
    win.y_trg = 50;
    win.u = win.y_trg;
    win.l = 13;

    if (ui.param[WIN_STYLE]) {
        win.box_H = WIN_H;
        win.box_h = 0;
        win.box_h_trg = WIN_H + ui.param[WIN_Y_OS];
        win.w = DISP_W;
        win.w_trg = WIN_W;
        win.l = (DISP_W - win.w) / 2;
    }

    ui.state = S_NONE;
}

//设置列表选择弹窗关闭回调函数
void window_set_list_callback(void (*cb)(uint8_t)) {
    win.list_on_close = cb;
}

/*
 * 确认弹窗初始化
 * 
 * 创建Yes/No确认对话框
 * 通过回调函数返回用户选择结果
 * 
 * 参数：
 *   title - 弹窗标题
 *   message - 确认消息文本
 *   bg - 弹窗背景菜单指针
 *   index - 弹窗关闭后返回的页面索引
 *   on_close - 关闭回调，参数为true=Yes，false=No
 */
void window_confirm_init(const char title[], const char message[], Menu *bg, uint8_t index, void (*on_close)(bool)) {
    strcpy(win.title, title);
    strcpy(win.message, message);
    win.confirm_mode = 1;
    win.msg_mode = 0;
    win.list_mode = 0;
    win.confirm_on_close = on_close;
    win.select = 0;
    win.conf_hl_cur = 0;
    win.conf_hl_trg = 0;
    win.conf_hl_vel = 0;
    win.bg = bg;
    win.index = index;
    ui.index = M_WINDOW;

    u8g2.setFont(WIN_FONT);
    uint8_t max_w = u8g2.getStrWidth(win.title);
    char msg_copy[128];
    strcpy(msg_copy, win.message);
    char* line = strtok(msg_copy, "\n");
    uint8_t line_count = 0;
    while (line) {
        uint8_t line_w = u8g2.getStrWidth(line);
        if (line_w > max_w) max_w = line_w;
        line_count++;
        line = strtok(NULL, "\n");
    }

    win.w = win.w_trg = max_w + WIN_MSG_PAD * 2;
    if (win.w_trg < 80) win.w = win.w_trg = 80;
    win.h = win.h_trg = WIN_MSG_PAD * 2 + LIST_TEXT_H + line_count * LIST_LINE_H + 4 + LIST_LINE_H + WIN_MSG_PAD;

    if (ui.param[WIN_STYLE]) {
        win.box_H = win.h;
        win.box_h = 0;
        win.box_h_trg = win.h + ui.param[WIN_Y_OS];
        win.w = DISP_W;
    }

    win.l = (DISP_W - win.w) / 2;
    win.y = -win.h - 2;
    win.y_trg = (DISP_H - win.h) / 2;
    win.u = win.y_trg;
    ui.state = S_NONE;
}

/*
 * 弹窗显示函数
 * 
 * 绘制弹窗的所有UI元素，根据弹窗模式（消息/确认/列表/数值）进行不同绘制
 * 支持虚化背景效果（BOKEH）和拉伸动画（WIN_STYLE）
 * 
 * 虚化背景原理：
 * - 在弹窗下方的背景区域绘制棋盘格遮罩
 * - 根据弹窗进入进度分步增加虚化强度
 * - 深色模式和亮色模式使用相反的虚化逻辑
 */
void window_show() {
    list_show(win.bg, win.index);

    if (ui.param[WIN_BOK]) {
        float start_y = WIN_Y;
        float target_y = win.y_trg > start_y ? win.y_trg : win.u;
        float range = target_y - start_y;
        float entry_progress = range > 0 ? (win.y - start_y) / range : 1.0f;
        if (entry_progress < 0) entry_progress = 0;
        if (entry_progress > 1) entry_progress = 1;

        uint32_t now = millis();
        if (now - win.last_bokeh_time >= ui.param[FADE_ANI]) {
            uint8_t target = 0;
            if (entry_progress >= 0.75f) target = 3;
            else if (entry_progress >= 0.50f) target = 2;
            else if (entry_progress >= 0.25f) target = 1;
            if (target > win.bokeh_step) win.bokeh_step++;
            else if (target < win.bokeh_step) win.bokeh_step--;
            win.last_bokeh_time = now;
        }

        if (win.bokeh_step) {
            if (ui.param[FADE_MODE] == 0) {
                if (ui.param[DARK_MODE]) {
                    if (win.bokeh_step >= 3) {
                        for (uint16_t y = 0; y < 128; ++y)
                            for (uint16_t x = 0; x < 16; ++x)
                                if (y % 2 == 0) buf_ptr[y * 16 + x] &= 0x55;
                                else buf_ptr[y * 16 + x] &= 0xAA;
                    } else if (win.bokeh_step >= 2) {
                        for (uint16_t y = 0; y < 128; ++y)
                            for (uint16_t x = 0; x < 16; ++x)
                                if (y % 2 == 0) buf_ptr[y * 16 + x] &= 0xAA;
                                else buf_ptr[y * 16 + x] &= 0x55;
                    } else {
                        for (uint16_t y = 0; y < 128; y += 2)
                            for (uint16_t x = 0; x < 16; ++x)
                                buf_ptr[y * 16 + x] &= 0x55;
                    }
                } else {
                    if (win.bokeh_step >= 3) {
                        for (uint16_t y = 0; y < 128; ++y)
                            for (uint16_t x = 0; x < 16; ++x)
                                if (y % 2 == 0) buf_ptr[y * 16 + x] |= 0xAA;
                                else buf_ptr[y * 16 + x] |= 0x55;
                    } else if (win.bokeh_step >= 2) {
                        for (uint16_t y = 0; y < 128; ++y)
                            for (uint16_t x = 0; x < 16; ++x)
                                if (y % 2 == 0) buf_ptr[y * 16 + x] |= 0x55;
                                else buf_ptr[y * 16 + x] |= 0xAA;
                    } else {
                        for (uint16_t y = 0; y < 128; y += 2)
                            for (uint16_t x = 0; x < 16; ++x)
                                buf_ptr[y * 16 + x] |= 0x55;
                    }
                }
            } else {
                for (uint16_t y = 0; y < 128; y += 2)
                    for (uint16_t x = 0; x < 16; ++x)
                        buf_ptr[y * 16 + x] &= 0xAA;
                if (win.bokeh_step >= 2)
                    for (uint16_t y = 1; y < 128; y += 2)
                        for (uint16_t x = 0; x < 16; ++x)
                            buf_ptr[y * 16 + x] &= 0x55;
            }
        }
    }

    uint8_t bg_color = 0;
    uint8_t fg_color = 1;

    u8g2.setFont(WIN_FONT);

    if (win.msg_mode) {
        animation(&win.y, &win.y_trg, WIN_ANI);

        if (ui.param[WIN_STYLE]) {
            animation(&win.box_h, &win.box_h_trg, WIN_ANI);
            animation(&win.box_h_trg, &win.box_H, WIN_ANI);
            animation(&win.w, &win.w_trg, WIN_ANI);
            win.l = (DISP_W - win.w) / 2;

            if (win.box_h > 2) {
                u8g2.setDrawColor(bg_color);
                u8g2.drawBox((int16_t)win.l, (int16_t)win.y, (int16_t)win.w, (int16_t)win.box_h);
                u8g2.setDrawColor(fg_color);
                u8g2.drawRFrame((int16_t)win.l, (int16_t)win.y, (int16_t)win.w, (int16_t)win.box_h, 1);
            }
        } else {
            u8g2.setDrawColor(bg_color);
            u8g2.drawBox((int16_t)win.l, (int16_t)win.y, (int16_t)win.w, (int16_t)win.h);
            u8g2.setDrawColor(fg_color);
            u8g2.drawRFrame((int16_t)win.l, (int16_t)win.y, (int16_t)win.w, (int16_t)win.h, 1);
        }

        if (!ui.param[WIN_STYLE] || win.box_h > WIN_MSG_PAD + LIST_TEXT_H) {
            uint8_t title_w = u8g2.getStrWidth(win.title);
            u8g2.setCursor((int16_t)win.l + ((int16_t)win.w - title_w) / 2, (int16_t)win.y + WIN_MSG_PAD + LIST_TEXT_H);
            u8g2.print(win.title);

            char msg_copy[128];
            strcpy(msg_copy, win.message);
            char* line = strtok(msg_copy, "\n");
            int line_y = (int16_t)win.y + WIN_MSG_PAD + LIST_TEXT_H + LIST_LINE_H;
            while (line) {
                uint8_t text_w = u8g2.getStrWidth(line);
                u8g2.setCursor((int16_t)win.l + ((int16_t)win.w - text_w) / 2, line_y);
                u8g2.print(line);
                line_y += LIST_LINE_H;
                line = strtok(NULL, "\n");
            }
        }
    } else if (win.confirm_mode) {
        animation(&win.y, &win.y_trg, WIN_ANI);
        hl_ani(&win.conf_hl_cur, &win.conf_hl_trg, &win.conf_hl_vel, LIST_ANI);

        if (ui.param[WIN_STYLE]) {
            animation(&win.box_h, &win.box_h_trg, WIN_ANI);
            animation(&win.box_h_trg, &win.box_H, WIN_ANI);
            animation(&win.w, &win.w_trg, WIN_ANI);
            win.l = (DISP_W - win.w) / 2;
            if (win.box_h > 2) {
                u8g2.setDrawColor(bg_color);
                u8g2.drawBox((int16_t)win.l, (int16_t)win.y, (int16_t)win.w, (int16_t)win.box_h);
                u8g2.setDrawColor(fg_color);
                u8g2.drawRFrame((int16_t)win.l, (int16_t)win.y, (int16_t)win.w, (int16_t)win.box_h, 1);
            }
        } else {
            u8g2.setDrawColor(bg_color);
            u8g2.drawBox((int16_t)win.l, (int16_t)win.y, (int16_t)win.w, (int16_t)win.h);
            u8g2.setDrawColor(fg_color);
            u8g2.drawRFrame((int16_t)win.l, (int16_t)win.y, (int16_t)win.w, (int16_t)win.h, 1);
        }

        if (!ui.param[WIN_STYLE] || win.box_h > WIN_MSG_PAD + LIST_TEXT_H) {
            uint8_t title_w = u8g2.getStrWidth(win.title);
            u8g2.setCursor((int16_t)win.l + ((int16_t)win.w - title_w) / 2, (int16_t)win.y + WIN_MSG_PAD + LIST_TEXT_H);
            u8g2.print(win.title);

            char msg_copy[128];
            strcpy(msg_copy, win.message);
            char* line = strtok(msg_copy, "\n");
            int line_y = (int16_t)win.y + WIN_MSG_PAD + LIST_TEXT_H + LIST_LINE_H;
            uint8_t line_count = 0;
            while (line) {
                uint8_t text_w = u8g2.getStrWidth(line);
                u8g2.setCursor((int16_t)win.l + ((int16_t)win.w - text_w) / 2, line_y);
                u8g2.print(line);
                line_y += LIST_LINE_H;
                line_count++;
                line = strtok(NULL, "\n");
            }

            int16_t btn_y = (int16_t)win.y + WIN_MSG_PAD * 2 + LIST_TEXT_H + line_count * LIST_LINE_H + 4;
            const char* yes_str = "Yes";
            const char* no_str = "No";
            uint8_t yes_w = u8g2.getStrWidth(yes_str);
            uint8_t no_w = u8g2.getStrWidth(no_str);
            int16_t yes_x = (int16_t)win.l + (int16_t)win.w / 4 - yes_w / 2;
            int16_t no_x = (int16_t)win.l + (int16_t)win.w * 3 / 4 - no_w / 2;
            uint8_t btn_w = ((yes_w > no_w) ? yes_w : no_w) + 8;

            win.conf_hl_trg = win.select == 0 ? (float)(yes_x - 4) : (float)(no_x - 4);

            u8g2.setDrawColor(fg_color);
            u8g2.setCursor(yes_x, btn_y + LIST_TEXT_H + LIST_TEXT_S);
            u8g2.print(yes_str);
            u8g2.setCursor(no_x, btn_y + LIST_TEXT_H + LIST_TEXT_S);
            u8g2.print(no_str);

            u8g2.setDrawColor(2);
            u8g2.drawRBox((int16_t)win.conf_hl_cur, btn_y, btn_w, LIST_LINE_H, LIST_BOX_R);
        }
    } else if (win.list_mode) {
        animation(&win.y, &win.y_trg, WIN_ANI);
        hl_ani(&win.list_y, &win.list_y_trg, &win.list_vel, LIST_ANI);
        hl_ani(&win.hl_sel_cur, &win.hl_sel_trg, &win.hl_vel, LIST_ANI);

        if (ui.param[WIN_STYLE]) {
            animation(&win.box_h, &win.box_h_trg, WIN_ANI);
            animation(&win.box_h_trg, &win.box_H, WIN_ANI);
            animation(&win.w, &win.w_trg, WIN_ANI);
            win.l = (DISP_W - win.w) / 2;
            if (win.box_h > 2) {
                u8g2.setDrawColor(bg_color);
                u8g2.drawBox((int16_t)win.l, (int16_t)win.y, (int16_t)win.w, (int16_t)win.box_h);
                u8g2.setDrawColor(fg_color);
                u8g2.drawRFrame((int16_t)win.l, (int16_t)win.y, (int16_t)win.w, (int16_t)win.box_h, 1);
            }
        } else {
            u8g2.setDrawColor(bg_color);
            u8g2.drawBox((int16_t)win.l, (int16_t)win.y, (int16_t)win.w, (int16_t)win.h);
            u8g2.setDrawColor(fg_color);
            u8g2.drawRFrame((int16_t)win.l, (int16_t)win.y, (int16_t)win.w, (int16_t)win.h, 1);
        }

        int16_t content_top = (int16_t)win.y + WIN_MSG_PAD * 2 + LIST_TEXT_H;
        int16_t content_bot = (int16_t)win.y + (int16_t)win.h - WIN_MSG_PAD;
        uint8_t item_rows = (content_bot - content_top) / LIST_LINE_H;

        u8g2.setDrawColor(fg_color);
        u8g2.setCursor((int16_t)win.l + ((int16_t)win.w - u8g2.getStrWidth(win.title)) / 2, (int16_t)win.y + WIN_MSG_PAD + LIST_TEXT_H);
        u8g2.print(win.title);

        u8g2.setClipWindow((int16_t)win.l + 2, content_top, (int16_t)win.l + (int16_t)win.w - 2, content_bot);

        for (uint8_t i = 0; i < win.list_count; i++) {
            int16_t item_y = content_top + (int16_t)(i * LIST_LINE_H) + (int16_t)win.list_y;
            u8g2.setDrawColor(fg_color);
            u8g2.setCursor((int16_t)win.l + ((int16_t)win.w - u8g2.getStrWidth(win.list_items[i])) / 2, item_y + LIST_TEXT_H + LIST_TEXT_S);
            u8g2.print(win.list_items[i]);
        }

        int16_t hl_y = content_top + (int16_t)(win.hl_sel_cur * LIST_LINE_H) + (int16_t)win.list_y;
        uint8_t need_scroll = (win.list_count > item_rows);
        int16_t hl_w = need_scroll ? (int16_t)win.w - 8 : (int16_t)win.w - 4;
        u8g2.setDrawColor(2);
        u8g2.drawRBox((int16_t)win.l + 2, hl_y, hl_w, LIST_LINE_H, LIST_BOX_R);

        u8g2.setMaxClipWindow();

        if (win.list_count > item_rows) {
            int16_t eff_h = ui.param[WIN_STYLE] ? (int16_t)win.box_h : (int16_t)win.h;
            int16_t scr_bot = (int16_t)win.y + eff_h - WIN_MSG_PAD;
            int16_t track_h = scr_bot - content_top - 4;
            if (track_h > 4) {
                u8g2.setClipWindow((int16_t)win.l, (int16_t)win.y + 2,
                                   (int16_t)win.l + (int16_t)win.w,
                                   (int16_t)win.y + eff_h - 1);
                u8g2.setDrawColor(fg_color);
                float scroll_ratio = (float)(-win.list_y) / ((win.list_count - item_rows) * LIST_LINE_H);
                if (scroll_ratio < 0) scroll_ratio = 0;
                if (scroll_ratio > 1) scroll_ratio = 1;
                uint8_t thumb_h = track_h * item_rows / win.list_count;
                if (thumb_h < 4) thumb_h = 4;
                uint8_t thumb_y = content_top + 2 + (track_h - thumb_h) * scroll_ratio;
                u8g2.drawVLine((int16_t)win.l + (int16_t)win.w - 3, content_top + 2, track_h);
                u8g2.drawBox((int16_t)win.l + (int16_t)win.w - 4, thumb_y, 3, thumb_h);
                u8g2.setMaxClipWindow();
            }
        }
    } else {
        win.bar_trg = (float)(*win.value - win.min) / (float)(win.max - win.min) * (WIN_BAR_W - 4);

        animation(&win.bar, &win.bar_trg, WIN_ANI);
        animation(&win.y, &win.y_trg, WIN_ANI);

        if (ui.param[WIN_STYLE]) {
            animation(&win.box_h, &win.box_h_trg, WIN_ANI);
            animation(&win.box_h_trg, &win.box_H, WIN_ANI);
            animation(&win.w, &win.w_trg, WIN_ANI);
            win.l = (DISP_W - win.w) / 2;

            if (win.box_h > 2) {
                u8g2.setDrawColor(bg_color);
                u8g2.drawRBox(win.l, (int16_t)win.y, win.w, win.box_h, 1);
                u8g2.setDrawColor(fg_color);
                u8g2.drawRFrame(win.l, (int16_t)win.y, win.w, win.box_h, 1);
            }
        } else {
            u8g2.setDrawColor(bg_color);
            u8g2.drawRBox(win.l, (int16_t)win.y, WIN_W, WIN_H, 1);
            u8g2.setDrawColor(fg_color);
            u8g2.drawRFrame(win.l, (int16_t)win.y, WIN_W, WIN_H, 1);
        }

        if (!ui.param[WIN_STYLE] || win.box_h > 16) {
            u8g2.drawRFrame(win.l + 5, (int16_t)win.y + 20, WIN_BAR_W, WIN_BAR_H, 1);
            u8g2.drawBox(win.l + 7, (int16_t)win.y + 22, win.bar, WIN_BAR_H - 4);
            u8g2.setCursor(win.l + 5, (int16_t)win.y + 14);
            u8g2.print(win.title);
            u8g2.setCursor(win.l + 78, (int16_t)win.y + 14);
            u8g2.print(*win.value);
        }

        if (!strcmp(win.title, "Disp Bri")) {
            u8g2.setContrast(ui.param[DISP_BRI]);
        }
    }
}

/*
 * 弹窗处理函数
 * 
 * 处理弹窗的输入事件：
 * - 数值弹窗：旋转调节数值，按键确认/取消
 * - 消息弹窗：按键关闭
 * - 列表弹窗：旋转选择项，按键确认
 * - 确认弹窗：旋转切换Yes/No，按键确认
 * 
 * 支持弹窗未完全进入时禁止操作
 * 弹窗关闭时使用退出音效
 */
void window_proc() {
    window_show();
    if (win.y == win.y_trg && win.y_trg < 0) {
        if (!ui.param[WIN_STYLE] || win.box_h == 0) {
            win.list_mode = 0;
            win.confirm_mode = 0;
            ui.index = win.index;
            ui.state = S_NONE;
        }
    }
    if (btn.pressed && win.y == win.y_trg && win.y_trg > 0) {
        btn.pressed = false;
        if (win.confirm_mode) {
            switch (btn.id) {
                case BTN_ID_CW:
                    win.select = 1;
                    break;
                case BTN_ID_CC:
                    win.select = 0;
                    break;
                case BTN_ID_SP:
                case BTN_ID_LP:
                    if (win.confirm_on_close) {
                        win.confirm_on_close(win.select == 0);
                        win.confirm_on_close = nullptr;
                    }
                    win.y_trg = -win.h - 2;
                    if (ui.param[WIN_STYLE]) {
                        win.box_H = 0;
                        win.box_h_trg = 0;
                        win.w_trg = DISP_W;
                    }
                    buzzer_exit_sound();
                    break;
            }
        } else if (win.msg_mode) {
            win.y_trg = -win.h - 2;
            if (ui.param[WIN_STYLE]) {
                win.box_H = 0;
                win.box_h_trg = 0;
                win.w_trg = DISP_W;
            }
            buzzer_exit_sound();
        } else if (win.list_mode) {
            int16_t content_top = (int16_t)win.y + WIN_MSG_PAD * 2 + LIST_TEXT_H;
            int16_t content_bot = (int16_t)win.y + (int16_t)win.h - WIN_MSG_PAD;
            uint8_t item_rows = (content_bot - content_top) / LIST_LINE_H;
            switch (btn.id) {
                case BTN_ID_CW:
                    if (win.list_select < win.list_count - 1) {
                        win.list_select++;
                        win.hl_sel_trg = (float)win.list_select;
                        int16_t scroll_rows = (int16_t)(-win.list_y_trg / LIST_LINE_H);
                        if ((int16_t)win.list_select >= scroll_rows + (int16_t)item_rows)
                            win.list_y_trg -= LIST_LINE_H;
                    }
                    break;
                case BTN_ID_CC:
                    if (win.list_select > 0) {
                        win.list_select--;
                        win.hl_sel_trg = (float)win.list_select;
                        int16_t scroll_rows = (int16_t)(-win.list_y_trg / LIST_LINE_H);
                        if ((int16_t)win.list_select < scroll_rows)
                            win.list_y_trg += LIST_LINE_H;
                    }
                    break;
                case BTN_ID_SP:
                case BTN_ID_LP:
                    if (win.list_on_close) {
                        win.list_on_close(win.list_select);
                        win.list_on_close = nullptr;
                        eeprom.change = true;
                    }
                    win.y_trg = -win.h - 2;
                    if (ui.param[WIN_STYLE]) {
                        win.box_H = 0;
                        win.box_h_trg = 0;
                        win.w_trg = DISP_W;
                    }
                    buzzer_exit_sound();
                    break;
            }
        } else {
            switch (btn.id) {
                case BTN_ID_CW:
                    if (*win.value < win.max) {
                        *win.value += win.step;
                        eeprom.change = true;
                    }
                    break;
                case BTN_ID_CC:
                    if (*win.value > win.min) {
                        *win.value -= win.step;
                        eeprom.change = true;
                    }
                    break;
                case BTN_ID_SP:
                case BTN_ID_LP:
                    win.y_trg = WIN_Y_TRG;
                    if (ui.param[WIN_STYLE]) {
                        win.box_H = 0;
                        win.box_h_trg = 0;
                        win.w_trg = DISP_W;
                    }
                    buzzer_exit_sound();
                    break;
            }
        }
    }
}
