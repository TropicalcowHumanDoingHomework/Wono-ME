#include "animation.h"
#include "display.h"
#include "ui_state.h"

/************************************* 指数缓动动画 *************************************/

/*
 * 指数缓动动画
 * 
 * 原理：
 * 使用指数衰减公式将值从当前位置平滑过渡到目标位置
 * 每次调用按固定比例缩小差距，形成"先快后慢"的缓出效果
 * 
 * 公式：a += (a_trg - a) / (param[n] / 10.0)
 * 分母越大动画越慢：param=10时一步到位，param=100时每次趋近10%
 * 
 * 阈值处理：
 * 当差值 < 0.15时直接归位，避免浮点精度导致的微幅抖动
 * 
 * 参数：
 *   a - 当前值指针
 *   a_trg - 目标值指针
 *   n - ui.param[]中的速度参数索引
 */
void animation(float *a, float *a_trg, uint8_t n) {
    if (*a != *a_trg) {
        if (fabs(*a - *a_trg) < 0.15f) {
            *a = *a_trg;
        } else {
            *a += (*a_trg - *a) / (ui.param[n] / 10.0f);
        }
    }
}

/************************************* 弹簧质点动画 *************************************/

/*
 * 弹簧质点模型动画
 * 
 * 原理：
 * 模拟弹簧-质点-阻尼物理系统，产生弹性跟随效果
 * 物体受回复力（胡克定律）和阻尼力共同作用
 * 
 * 物理模型：
 * 回复力 F = (目标 - 当前位置) * 刚度(stiffness)
 * 速度更新：vel = (vel + F) * 阻尼(damping)
 * 位置更新：a += vel
 * 
 * 特点：
 * - stiffness > 0.5时会产生"过头"的弹性效果
 * - damping < 0.8时会产生振荡衰减效果
 * - damping ≈ 1.0时接近无阻尼简谐运动（持续振荡）
 * 
 * 稳定判定：
 * 当位置差值 < 0.05且速度 < 0.05时归位锁定
 * 
 * 参数：
 *   a - 当前位置指针
 *   a_trg - 目标位置指针
 *   vel - 速度状态变量指针（调用者需持久化）
 *   stiffness - 弹簧刚度（0~1）
 *   damping - 阻尼系数（0~1）
 */
void animation_spring(float *a, float *a_trg, float *vel, float stiffness, float damping) {
    if (*a != *a_trg || fabs(*vel) > 0.01f) {
        float force = (*a_trg - *a) * stiffness;
        *vel = (*vel + force) * damping;
        *a += *vel;
        if (fabs(*a - *a_trg) < 0.05f && fabs(*vel) < 0.05f) {
            *a = *a_trg;
            *vel = 0;
        }
    }
}

/************************************* 弹跳动画 *************************************/

/*
 * 阻尼弹跳动画
 * 
 * 原理：
 * 在弹簧模型基础上增加角频率项，模拟类似球体落地的弹跳效果
 * 使用用户可调的弹簧刚度(SPRING_K)和阻尼(SPRING_D)参数
 * 
 * 核心变量：
 * - k = param[SPRING_K] / 100：弹簧刚度（0.10~1.00）
 * - d = param[SPRING_D] / 100：阻尼系数（0.10~1.00）
 * - w = 4.0 * sqrt(k)：角频率，控制弹跳频率
 * - damp = 1.0 - d * 0.4：阻尼因子（限幅0.5~0.99）
 * 
 * 附加弹性项：vel += (a_trg - a) * 0.03 * w
 * 该额外项使目标位置附近产生过冲，形成弹跳感
 * 
 * 稳定判定与弹簧动画相同（< 0.05时归位）
 * 
 * 参数：
 *   a - 当前位置指针
 *   a_trg - 目标位置指针
 *   vel - 速度状态变量指针
 *   n - 未使用（保留参数接口一致性）
 */
void animation_bounce(float *a, float *a_trg, float *vel, uint8_t n) {
    if (*a != *a_trg || fabs(*vel) > 0.01f) {
        float k = ui.param[SPRING_K] / 100.0f;
        float d = ui.param[SPRING_D] / 100.0f;
        if (k < 0.1f) k = 0.1f;
        float w = 4.0f * sqrtf(k);
        float damp = (1.0f - d * 0.4f);
        if (damp < 0.5f) damp = 0.5f;
        if (damp > 0.99f) damp = 0.99f;
        float force = (*a_trg - *a) * k * 0.5f;
        *vel = (*vel + force) * damp;
        *vel += (*a_trg - *a) * 0.03f * w;
        *a += *vel;
        if (fabs(*a - *a_trg) < 0.05f && fabs(*vel) < 0.05f) {
            *a = *a_trg;
            *vel = 0;
        }
    }
}

/************************************* 重力弹跳动画 *************************************/

/*
 * 重力弹跳动画
 * 
 * 原理：
 * 模拟球体从高处自由落体，碰到地面后弹跳的物理过程
 * 目标位置 = "地面"，球只在地面上方弹跳，不会穿过地面
 * 
 * 物理参数（由用户可调的 SPRING_K 和 SPRING_D 控制）：
 * - gravity = k * 1.5：重力加速度（刚度越大，下落越快）
 * - restitution = 1.0 - d * 0.6：弹跳恢复系数（阻尼越大，每次弹跳损失越多能量）
 * 
 * 运动过程：
 * 1. 首帧：给予指向地面的初速度（模拟从高处抛出的球）
 * 2. 每帧：施加重力 vel += gravity，更新位置 a += vel
 * 3. 地面碰撞检测：当球穿过地面时，速度反向并乘以恢复系数
 * 4. 弹跳次数增加 → 弹跳高度降低 → 最终静止在地面
 * 
 * 与 Spring 的区别：
 * Spring 是弹性回复力模型，会在目标值两侧振荡
 * Gravity 是重力+碰撞模型，严格在地面一侧弹跳，更像真实物理
 * 
 * 参数：
 *   a - 当前位置指针
 *   a_trg - 目标位置指针（地面）
 *   vel - 速度状态变量指针
 *   n - 未使用（保留接口一致性）
 */
void animation_gravity(float *a, float *a_trg, float *vel, uint8_t n) {
    if (*a != *a_trg || fabs(*vel) > 0.01f) {
        float k = ui.param[SPRING_K] / 100.0f;
        float d = ui.param[SPRING_D] / 100.0f;
        if (k < 0.1f) k = 0.1f;

        // 重力加速度（刚度控制强度）
        float gravity = k * 1.5f;
        // 弹跳恢复系数：每次碰撞后保留的速度比例
        float restitution = 1.0f - d * 0.6f;
        if (restitution < 0.15f) restitution = 0.15f;
        if (restitution > 0.9f) restitution = 0.9f;

        // 保存旧位置用于碰撞检测
        float old_a = *a;

        // 首帧：给予指向目标（地面）的初速度
        if (fabs(*vel) < 0.001f && fabs(*a - *a_trg) > 1.0f) {
            *vel = (*a_trg - *a) * 0.35f;
        }

        // 重力始终指向目标位置（地面方向）
        float dir = *a_trg - *a;
        if (dir > 0.001f) {
            *vel += gravity;
        } else if (dir < -0.001f) {
            *vel -= gravity;
        }

        // 更新位置
        *a += *vel;

        // 碰撞检测：判断是否越过地面
        bool crossed = (old_a < *a_trg && *a >= *a_trg) ||
                       (old_a > *a_trg && *a <= *a_trg);

        if (crossed) {
            // 速度反向，能量损失
            *vel = -*vel * restitution;
            // 贴回地面
            *a = *a_trg;

            // 弹跳幅度足够小时锁定到地面
            if (fabs(*vel) < 0.3f) {
                *a = *a_trg;
                *vel = 0;
            }
        }
    }
}

/************************************* 高亮条动画统一调度 *************************************/

void hl_ani(float *a, float *a_trg, float *vel, uint8_t n) {
    if (ui.param[HL_ANI_MODE] == 0) {
        animation(a, a_trg, n);
    } else if (ui.param[HL_ANI_MODE] == 1) {
        animation_spring(a, a_trg, vel, ui.param[SPRING_K] / 100.0f, ui.param[SPRING_D] / 100.0f);
    } else if (ui.param[HL_ANI_MODE] == 2) {
        animation_bounce(a, a_trg, vel, n);
    } else {
        animation_gravity(a, a_trg, vel, n);
    }
}

/************************************* 页面切换过渡 *************************************/

/*
 * 画面渐入/渐出过渡函数
 * 
 * 用于页面切换时的视觉过渡效果，分4步完成
 * 每步通过last_fade_time控制时间间隔
 * 
 * 模式0（棋盘格模式）：
 * 按行交替操作实现平滑渐变：
 * - 黑暗模式(1)：逐步清除像素位 → 全黑（偶数行→奇数行→反相→全黑）
 * - 白天模式(0)：逐步设置像素位 → 全白（偶数行→奇数行→反相→全白）
 * 
 * 模式1（整体遮罩模式）：
 * 逐行填充固定图案：
 * - 第1步：偶数行=0xAA
 * - 第2步：奇数行=0x55
 * - 第3步：偶数行=0xFF
 * - 第4步：奇数行=0xFF → 全白，完成
 * 
 * 完成时：ui.state = S_NONE, ui.fade = 0
 * 
 * 像素位说明：
 * LS013B7DH03是Memory LCD，1像素=1bit
 * 每字节(16bit)中：0x55 = 01010101, 0xAA = 10101010
 * 棋盘格交替位可产生中间灰度过渡效果
 */
void fade() {
    static uint32_t last_fade_time = 0;
    uint32_t now = millis();

    if (now - last_fade_time < ui.param[FADE_ANI]) {
        return;
    }
    last_fade_time = now;

    bool dark = ui.param[DARK_MODE] || ui.index == M_SLEEP;

    switch (ui.param[FADE_MODE]) {
        case 0:
            if (dark) {
                switch (ui.fade) {
                    case 1:
                        for (uint16_t y = 0; y < 128; ++y)
                            if (y % 2 == 0)
                                for (uint16_t x = 0; x < 16; ++x)
                                    buf_ptr[y * 16 + x] &= 0xAA;
                        break;
                    case 2:
                        for (uint16_t y = 0; y < 128; ++y)
                            if (y % 2 == 1)
                                for (uint16_t x = 0; x < 16; ++x)
                                    buf_ptr[y * 16 + x] &= 0x55;
                        break;
                    case 3:
                        for (uint16_t y = 0; y < 128; ++y)
                            if (y % 2 == 0)
                                for (uint16_t x = 0; x < 16; ++x)
                                    buf_ptr[y * 16 + x] &= 0x55;
                        break;
                    case 4:
                        for (uint16_t i = 0; i < buf_len; ++i)
                            buf_ptr[i] = 0x00;
                        ui.state = S_NONE;
                        ui.fade = 0;
                        break;
                    default:
                        ui.state = S_NONE;
                        ui.fade = 0;
                        break;
                }
            } else {
                switch (ui.fade) {
                    case 1:
                        for (uint16_t y = 0; y < 128; ++y)
                            if (y % 2 == 0)
                                for (uint16_t x = 0; x < 16; ++x)
                                    buf_ptr[y * 16 + x] &= 0xAA;
                        break;
                    case 2:
                        for (uint16_t y = 0; y < 128; ++y)
                            if (y % 2 == 1)
                                for (uint16_t x = 0; x < 16; ++x)
                                    buf_ptr[y * 16 + x] &= 0x55;
                        break;
                    case 3:
                        for (uint16_t y = 0; y < 128; ++y)
                            if (y % 2 == 0)
                                for (uint16_t x = 0; x < 16; ++x)
                                    buf_ptr[y * 16 + x] &= 0x55;
                        break;
                    case 4:
                        for (uint16_t i = 0; i < buf_len; ++i)
                            buf_ptr[i] = 0x00;
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
        case 1:
            switch (ui.fade) {
                case 1:
                    for (uint16_t y = 0; y < 128; y += 2)
                        for (uint16_t x = 0; x < 16; ++x)
                            buf_ptr[y * 16 + x] &= 0xAA;
                    break;
                case 2:
                    for (uint16_t y = 1; y < 128; y += 2)
                        for (uint16_t x = 0; x < 16; ++x)
                            buf_ptr[y * 16 + x] &= 0x55;
                    break;
                case 3:
                    for (uint16_t y = 0; y < 128; y += 2)
                        for (uint16_t x = 0; x < 16; ++x)
                            buf_ptr[y * 16 + x] &= 0x55;
                    break;
                case 4:
                    for (uint16_t y = 1; y < 128; y += 2)
                        for (uint16_t x = 0; x < 16; ++x)
                            buf_ptr[y * 16 + x] &= 0xAA;
                    ui.state = S_NONE;
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