#ifndef ANIMATION_H
#define ANIMATION_H

#include "config.h"

/************************************* 动画函数 *************************************/

/*
 * 指数缓动动画函数
 * 
 * 将值a从当前值平滑过渡到目标值a_trg
 * 使用指数衰减公式：a += (a_trg - a) * factor
 * 其中factor由n参数决定（n=ui.param[]中的速度索引）
 * factor越大，动画越快；factor=1时为瞬间到位
 * 
 * 参数：
 *   a - 当前值的指针（会被修改）
 *   a_trg - 目标值的指针（会被修改）
 *   n - ui.param数组中速度参数的索引
 */
void animation(float *a, float *a_trg, uint8_t n);

/*
 * 弹簧质点模型动画函数
 * 
 * 模拟弹簧物理系统：
 * 加速度 = -stiffness * (位置 - 目标) - damping * 速度
 * stiffness（刚度）越大，弹簧回到目标位置越快
 * damping（阻尼）越大，振荡衰减越快
 * 
 * 特点：
 * - 会产生"过头再回弹"的弹性效果
 * - 需要维护速度状态变量vel
 * - 适用于需要弹性跟随感的UI元素
 * 
 * 参数：
 *   a - 当前位置的指针
 *   a_trg - 目标位置的指针
 *   vel - 速度状态变量的指针（需在外部保持）
 *   stiffness - 弹簧刚度系数（0.0~1.0）
 *   damping - 弹簧阻尼系数（0.0~1.0）
 */
void animation_spring(float *a, float *a_trg, float *vel, float stiffness, float damping);

/*
 * 阻尼弹跳动画函数
 * 
 * 使用解析公式实现阻尼振荡：
 * a_trg向a_final运动的同时，a向a_trg运动
 * 产生类似球体落地的逐次弹跳效果
 * 
 * vel作为内部速度状态变量（由函数内部修改）
 * n为ui.param[]中的速度索引
 * 
 * 参数：
 *   a - 当前值的指针
 *   a_trg - 过渡目标值的指针
 *   vel - 速度状态变量的指针
 *   n - ui.param数组中速度参数的索引
 */
void animation_bounce(float *a, float *a_trg, float *vel, uint8_t n);

/*
 * 重力弹跳动画函数
 * 
 * 模拟球体受重力作用下落并在地面弹跳的物理过程
 * 目标位置(a_trg)相当于"地面"，当前值只能在地面上方弹跳
 * 
 * 物理模型：
 * 1. 首帧给予一个指向目标的初速度（模拟从高处落下）
 * 2. 每帧施加重力加速度：vel += gravity
 * 3. 位置更新：a += vel
 * 4. 当穿过地面（越过目标）时：速度反向并乘以恢复系数（能量损失）
 * 5. 恢复系数由阻尼参数控制，每次弹跳高度逐渐降低
 * 6. 当弹跳幅度足够小时直接锁定到地面
 * 
 * 与Spring的区别：Spring会产生上下振荡（可穿过目标），
 * Gravity严格在地面一侧弹跳，模拟真实的落体物理
 * 
 * 参数：
 *   a - 当前位置指针
 *   a_trg - 目标位置指针（地面位置）
 *   vel - 速度状态变量指针
 *   n - 未使用（保留参数接口一致性）
 */
void animation_gravity(float *a, float *a_trg, float *vel, uint8_t n);

/*
 * 高亮条动画统一调度函数
 * 
 * 根据 ui.param[HL_ANI_MODE] 自动选择动画算法：
 *   0=Ease → animation()
 *   1=Spring → animation_spring()
 *   2=Bounce → animation_bounce()
 *   3=Gravity → animation_gravity()
 * 
 * 参数签名与各底层动画函数兼容：
 *   n 在 Ease/Bounce/Gravity 模式下作为 ui.param[]速度索引使用
 */
void hl_ani(float *a, float *a_trg, float *vel, uint8_t n);

/*
 * 消失/渐入函数
 * 
 * 页面切换时的过渡效果：
 * - FADE_MODE=0：棋盘格模式，逐格显示/隐藏
 * - FADE_MODE=1：整体遮罩模式，从左至右扫入/扫出
 * 
 * 使用ui.fade（0~1）控制进度
 * 配合UI状态机的S_FADE状态使用
 * 完成后自动切换至S_NONE
 */
void fade();

#endif