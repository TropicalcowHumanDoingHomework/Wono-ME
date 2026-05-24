#include "animation.h"
#include "display.h"
#include "ui_state.h"

/************************************* 动画函数 *************************************/

//动画函数：指数缓动，差值小于阈值时直接归位
//参数n对应ui.param中的动画速度索引，值越大动画越慢
void animation(float *a, float *a_trg, uint8_t n) {
    if (*a != *a_trg) {
        if (fabs(*a - *a_trg) < 0.15f) {
            *a = *a_trg;  //接近目标时直接归位，避免微小抖动
        } else {
            *a += (*a_trg - *a) / (ui.param[n] / 10.0f);  //按比例趋近目标
        }
    }
}

//弹簧动画函数：基于弹簧质点模型的物理动画，产生弹性跟随效果
void animation_spring(float *a, float *a_trg, float *vel, float stiffness, float damping) {
    if (*a != *a_trg || fabs(*vel) > 0.01f) {
        float force = (*a_trg - *a) * stiffness;  //弹簧回复力（胡克定律）
        *vel = (*vel + force) * damping;          //速度累加并施加阻尼衰减
        *a += *vel;                                //更新位置
        if (fabs(*a - *a_trg) < 0.05f && fabs(*vel) < 0.05f) {
            *a = *a_trg;   //稳定时归位
            *vel = 0;      //速度置零
        }
    }
}

//弹跳动画函数：使用阻尼振荡解析公式，产生富有弹性的弹跳效果
void animation_bounce(float *a, float *a_trg, float *vel, uint8_t n) {
    if (*a != *a_trg || fabs(*vel) > 0.01f) {
        //从用户参数中获取弹簧刚度和阻尼系数
        float k = ui.param[SPRING_K] / 100.0f;
        float d = ui.param[SPRING_D] / 100.0f;
        if (k < 0.1f) k = 0.1f;  //限制最小刚度
        float w = 4.0f * sqrtf(k);     //角频率
        float damp = (1.0f - d * 0.4f); //阻尼因子
        if (damp < 0.5f) damp = 0.5f;
        if (damp > 0.99f) damp = 0.99f;
        float force = (*a_trg - *a) * k * 0.5f;  //回复力
        *vel = (*vel + force) * damp;             //速度更新+阻尼
        *vel += (*a_trg - *a) * 0.03f * w;        //附加弹性项
        *a += *vel;                                //位置更新
        if (fabs(*a - *a_trg) < 0.05f && fabs(*vel) < 0.05f) {
            *a = *a_trg;
            *vel = 0;
        }
    }
}

//消失函数：以棋盘格或整体遮罩方式实现画面渐入/渐出
//支持两种模式（FADE_MODE）：0=棋盘格交替行模式，1=逐行整体遮罩模式
//分别适配黑暗模式（DARK_MODE=1）和白天模式（DARK_MODE=0）
//棋盘格模式：分4步，每步处理偶数行或奇数行的特定像素位
//整体遮罩模式：分4步，每步填充整行像素为0xAA或0xFF
void fade() {
    static uint32_t last_fade_time = 0;
    uint32_t now = millis();

    //根据FADE_ANI参数控制每步的时间间隔
    if (now - last_fade_time < ui.param[FADE_ANI]) {
        return;
    }
    last_fade_time = now;

    bool dark = ui.param[DARK_MODE];

    switch (ui.param[FADE_MODE]) {
        case 0:  //棋盘格模式：通过交替操作奇数/偶数行的像素位实现平滑渐变
            if (dark) {
                //黑暗模式：逐步清除像素位，从偶数行→奇数行→反相→全黑
                switch (ui.fade) {
                    case 1:
                        for (uint16_t y = 0; y < 128; ++y)
                            if (y % 2 == 0)
                                for (uint16_t x = 0; x < 16; ++x)
                                    buf_ptr[y * 16 + x] &= 0xAA;  //保留奇数位
                        break;
                    case 2:
                        for (uint16_t y = 0; y < 128; ++y)
                            if (y % 2 == 1)
                                for (uint16_t x = 0; x < 16; ++x)
                                    buf_ptr[y * 16 + x] &= 0x55;  //保留偶数位
                        break;
                    case 3:
                        for (uint16_t y = 0; y < 128; ++y)
                            if (y % 2 == 0)
                                for (uint16_t x = 0; x < 16; ++x)
                                    buf_ptr[y * 16 + x] &= 0x55;  //偶数行反相清除
                        break;
                    case 4:
                        for (uint16_t i = 0; i < buf_len; ++i)
                            buf_ptr[i] = 0x00;  //全黑
                        ui.state = S_NONE;
                        ui.fade = 0;
                        break;
                    default:
                        ui.state = S_NONE;
                        ui.fade = 0;
                        break;
                }
            } else {
                //白天模式：逐步设置像素位，从偶数行→奇数行→反相→全白
                switch (ui.fade) {
                    case 1:
                        for (uint16_t y = 0; y < 128; ++y)
                            if (y % 2 == 0)
                                for (uint16_t x = 0; x < 16; ++x)
                                    buf_ptr[y * 16 + x] |= 0x55;  //设置奇数位
                        break;
                    case 2:
                        for (uint16_t y = 0; y < 128; ++y)
                            if (y % 2 == 1)
                                for (uint16_t x = 0; x < 16; ++x)
                                    buf_ptr[y * 16 + x] |= 0xAA;  //设置偶数位
                        break;
                    case 3:
                        for (uint16_t y = 0; y < 128; ++y)
                            if (y % 2 == 0)
                                for (uint16_t x = 0; x < 16; ++x)
                                    buf_ptr[y * 16 + x] |= 0xAA;  //偶数行补充
                        break;
                    case 4:
                        for (uint16_t i = 0; i < buf_len; ++i)
                            buf_ptr[i] = 0xFF;  //全白
                        ui.state = S_NONE;
                        ui.fade = 0;
                        break;
                    default:
                        ui.state = S_NONE;
                        ui.fade = 0;
                        break;
                }
            }
            break;
        case 1:  //整体遮罩模式：逐行填充为固定值实现渐入/渐出
            switch (ui.fade) {
                case 1:
                    for (uint16_t y = 0; y < 128; y += 2)
                        for (uint16_t x = 0; x < 16; ++x)
                            buf_ptr[y * 16 + x] = 0xAA;  //偶数行填充棋盘格
                    break;
                case 2:
                    for (uint16_t y = 1; y < 128; y += 2)
                        for (uint16_t x = 0; x < 16; ++x)
                            buf_ptr[y * 16 + x] = 0x55;  //奇数行填充反棋盘格
                    break;
                case 3:
                    for (uint16_t y = 0; y < 128; y += 2)
                        for (uint16_t x = 0; x < 16; ++x)
                            buf_ptr[y * 16 + x] = 0xFF;  //偶数行全白
                    break;
                case 4:
                    for (uint16_t y = 1; y < 128; y += 2)
                        for (uint16_t x = 0; x < 16; ++x)
                            buf_ptr[y * 16 + x] = 0xFF;  //奇数行全白
                    ui.state = S_NONE;  //动画完成，恢复状态
                    ui.fade = 0;
                    break;
                default:
                    ui.state = S_NONE;
                    ui.fade = 0;
                    break;
        }
    }
    ui.fade++;
}