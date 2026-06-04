#ifndef WINDOW_H
#define WINDOW_H

#include "ui_types.h"

/************************************* 各种弹窗管理 *************************************/

/*
 * 弹窗状态全局变量
 * 
 * 管理所有弹窗类型（互斥使用）：
 * - 数值调节弹窗（默认）：带进度条，可调节数值参数
 * - 消息弹窗：显示只读消息文本
 * - 列表选择弹窗：从列表中选取一项
 * - 确认弹窗：Yes/No确认对话框
 */
extern WindowState win;

/*
 * 数值调节弹窗初始化
 * 
 * 创建带进度条的数值调节弹窗
 * 通过旋转旋钮或按键调节数值范围[min, max]
 * 
 * 参数：
 *   title - 弹窗标题
 *   select - 初始选中项
 *   value - 要修改的数值指针（指向ui.param[]中对应项）
 *   max/min - 数值范围
 *   step - 调节步进
 *   bg - 背景菜单指针（用于虚化背景绘制）
 *   index - 关闭弹窗后返回的页面索引
 */
void window_value_init(const char title[], uint8_t select, uint8_t *value, uint8_t max, uint8_t min, uint8_t step, Menu *bg, uint8_t index);

/*
 * 消息弹窗初始化
 * 
 * 创建纯文本消息提示弹窗
 * 仅显示标题和消息内容，无交互控件
 * 任意按键或旋钮操作即可关闭
 */
void window_message_init(const char title[], const char message[], Menu *bg, uint8_t index);

/*
 * 列表选择弹窗初始化
 * 
 * 创建可选择项的列表弹窗
 * 支持高亮条跟随动画
 * 关闭时通过回调函数返回选中项索引
 */
void window_list_select_init(const char title[], const char* items[], uint8_t item_count, Menu *bg, uint8_t index, uint8_t default_select = 0);

/*
 * 确认弹窗初始化
 * 
 * 创建Yes/No确认对话框
 * 左右选择确认/取消
 * 关闭时通过回调函数返回用户选择（true=确认，false=取消）
 */
void window_confirm_init(const char title[], const char message[], Menu *bg, uint8_t index, void (*on_close)(bool));

/*
 * 弹窗参数初始化
 * 
 * 重置弹窗位置、尺寸和模式标志到初始状态
 * 包括：
 * - 清空所有模式标志（msg/list/confirm_mode）
 * - 重置动画位置（y, w, h, bar等）
 * - 重置高亮条位置
 * - 重置虚化步骤
 */
void window_param_init();

/*
 * 设置列表选择弹窗关闭回调函数
 * 
 * 注册一个回调函数，当列表选择弹窗关闭时调用
 * 回调参数为选中项的索引（uint8_t）
 */
void window_set_list_callback(void (*cb)(uint8_t));

/*
 * 弹窗显示函数
 * 
 * 绘制弹窗所有UI元素：
 * - 虚化背景（如果WIN_BOK开启）
 * - 背景半透明遮罩
 * - 标题
 * - 内容区域（根据模式不同）
 *   - 数值模式：进度条+数值显示
 *   - 消息模式：消息文本
 *   - 列表模式：选项列表+高亮条
 *   - 确认模式：Yes/No选项
 * - 动画过渡
 */
void window_show();

/*
 * 弹窗处理函数
 * 
 * 处理弹窗的输入事件和状态更新：
 * - 旋钮旋转：切换选项或调节数值
 * - 短按：确认选择或关闭弹窗
 * - 长按：取消或关闭弹窗
 * - 调用各模式对应的回调函数
 * 
 * 在主循环中调用
 */
void window_proc();

#endif