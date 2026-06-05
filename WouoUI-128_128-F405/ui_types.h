#ifndef UI_TYPES_H
#define UI_TYPES_H

#include "config.h"

/*
 * 菜单项结构体
 * 
 * 每个菜单项包含一个标题字符串（最多31字符+结尾'\0'）
 * title的第一个字符可表示特殊类型：
 * - '£'：分隔行（不可选，仅用于视觉分隔）
 * - 其他：普通可选菜单项
 */
struct Menu {
    char title[32];
};

/************************************* UI状态结构体 *************************************/

/*
 * UI主状态：管理所有页面切换、层级、选择和参数
 * 
 * 状态机流转：
 * S_LAYER_IN → S_FADE → S_NONE → (用户操作) → S_LAYER_IN/S_LAYER_OUT/S_WINDOW
 * sleep=true → S_LAYER_OUT (到M_SLEEP)
 * 
 * layer层级管理：
 * - layer=0：主菜单（最外层）
 * - layer=1：子菜单或接口页面
 * - 最多UI_DEPTH层
 */
struct UiState {
    bool init;                          //当前页面是否已完成初始化动画
    uint8_t num[UI_MNUMB];              //每个页面的菜单项数量
    uint8_t select[UI_DEPTH];           //每层当前选中的菜单项索引
    uint8_t layer;                      //当前层级深度（0=最外层）
    uint8_t index;                      //当前页面索引（PageIndex枚举值）
    uint8_t state;                      //当前状态（PageState枚举：FADE/WINDOW/LAYER_IN/LAYER_OUT/NONE）
    bool sleep;                         //是否处于睡眠模式
    uint8_t fade = 1;                   //消失动画当前步骤（1-4）
    uint8_t fade_dir = 0;               //消失动画方向：0=渐出（内容→黑），1=渐入（黑→内容）
    uint8_t param[UI_PARAM];            //用户可调参数数组，索引见ParamIndex枚举
};

/*
 * 磁贴状态：管理磁贴页面的图标、指示器、标题的动画位置
 * 
 * 磁贴（Tile）是主菜单的UI风格
 * 每个磁贴包含：图标（icon）、指示器（indi）、大标题（title）
 * 支持三种动画模式：Ease、Spring、Bounce
 * 每种动画使用对应的速度/力/阻尼参数
 */
struct TileState {
    float title_y_calc   = TILE_INDI_S + (TILE_INDI_H - TILE_B_TITLE_H) / 2 + TILE_B_TITLE_H * 2;
    float title_y_trg_calc = TILE_INDI_S + (TILE_INDI_H - TILE_B_TITLE_H) / 2 + TILE_B_TITLE_H;
    int16_t temp;
    bool select_flag;
    float icon_x;          //图标X位置（当前值）
    float icon_x_trg;      //图标X位置（目标值）
    float icon_y;          //图标Y位置（当前值）
    float icon_y_trg;      //图标Y位置（目标值）
    float indi_x;          //指示器X位置（当前值）
    float indi_x_trg;      //指示器X位置（目标值）
    float title_y;         //大标题Y位置（当前值）
    float title_y_trg;     //大标题Y位置（目标值）
    float icon_x_vel;      //图标X速度（弹簧/弹跳动画用的速度变量）
    float icon_y_vel;      //图标Y速度
    float indi_x_vel;      //指示器X速度
    float title_y_vel;     //标题Y速度
};

/*
 * 列表状态：管理列表页面的滚动、选择框、进度条的动画位置和尺寸
 * 
 * 列表页面使用弯曲效果（curve），使选中的行在视觉上更突出
 * 选择框（box_x/box_y/box_w/box_h）围绕选中项绘制
 * 滚动条（bar_y）指示当前列表位置
 * 支持循环滚动（loop=true时列表首尾相连）
 */
struct ListState {
    int line_n;              //屏幕可显示的行数
    int16_t temp;
    bool loop;               //是否正在进行循环滚动
    float curve;             //当前行的弯曲偏移量（用于弧形列表效果）
    float y;                 //列表垂直滚动位置（当前值）
    float y_trg;             //列表垂直滚动位置（目标值）
    float box_x;             //选择框X方向延伸（当前值）
    float box_x_trg;         //选择框X方向延伸（目标值）
    float box_y;             //选择框Y位置（当前值）
    float box_y_trg[UI_DEPTH]; //选择框Y位置（目标值，每层独立）
    float bar_y;             //滚动条Y位置（当前值）
    float bar_y_trg;         //滚动条Y位置（目标值）
    float box_W;             //选择框宽度（稳态值）
    float box_w;             //选择框宽度（当前值，含过渡）
    float box_w_trg;         //选择框宽度（过渡目标值）
    float box_H;             //选择框高度（稳态值）
    float box_h;             //选择框高度（当前值，含过渡）
    float box_h_trg;         //选择框高度（过渡目标值）
    float box_y_vel;         //选择框Y速度（弹簧动画用）
    float box_x_vel;         //选择框X速度（弹簧动画用）
    float box_w_vel;         //选择框宽度速度（弹簧动画用）
    float box_w_vel_trg;     //选择框宽度过渡速度（弹簧动画用）
    float box_h_vel;         //选择框高度速度（弹簧动画用）
    float box_h_vel_trg;     //选择框高度过渡速度（弹簧动画用）
};

/*
 * 电压测量状态：管理ADC采样和波形显示数据
 * 
 * WAVE_SAMPLE=10个采样点取平均减少噪声
 * WAVE_W=128个数据点对应128像素宽度
 * ch0_wave[128]存储波形数据用于绘制波形图
 * text_bg_l控制文字背景标签的水平滑动动画
 */
struct VoltageState {
    int ch0_adc[WAVE_SAMPLE * WAVE_W];  //ADC采样数据缓冲区（10*128个样本）
    int val;                              //当前电压值（mV）
    int ch0_wave[128];                    //波形绘制数据（128点，对应屏幕宽度）
    float text_bg_l;                      //文字背景左边界（当前值）
    float text_bg_l_trg;                  //文字背景左边界（目标值）
    float text_bg_l_vel;                  //文字背景左边界速度（弹性动画用）
};

/*
 * 复选框状态：管理单选框和多选框的选中状态
 * 
 * 用于设置页面中的开关项和数值选择项
 * 各指针指向ui.param[]中的对应位置
 */
struct CheckBoxState {
    uint8_t* v;      //数值显示数组指针（指向ui.param[]中的数值项）
    uint8_t* m;      //多选框选中状态数组指针
    uint8_t* s;      //单选框选中值指针
    uint8_t* s_p;    //单选框选中位置指针
    uint8_t* map;    //菜单项位置到参数索引的映射表
};

/*
 * 弹窗状态：管理各类弹窗的动画和数据
 * 
 * 弹窗类型（互斥）：
 * - 数值调节弹窗：通过进度条调节一个数值参数
 * - 消息弹窗(msg_mode)：显示一段消息文本
 * - 列表选择弹窗(list_mode)：从列表中选择一项
 * - 确认弹窗(confirm_mode)：是否确认操作
 * 
 * 支持两种动画样式：
 * - WIN_STYLE=0：简单滑动进入
 * - WIN_STYLE=1：拉伸效果
 * 
 * 背景虚化通过bokeh_step和last_bokeh_time控制
 */
struct WindowState {
    char title[32];                       //弹窗标题
    char message[128];                    //弹窗消息内容
    uint8_t select;                       //当前选中项
    uint8_t* value;                       //要修改的数值指针（指向ui.param[]中对应项）
    uint8_t max;                          //数值最大值
    uint8_t min;                          //数值最小值
    uint8_t step;                         //数值步进
    Menu* bg;                             //弹窗背景菜单指针
    uint8_t index;                        //弹窗返回后的页面索引
    uint8_t msg_mode;                     //消息弹窗模式标志
    uint8_t list_mode;                    //列表选择弹窗模式标志
    char list_items[WIN_LIST_MAX][WIN_LIST_ITEM_LEN]; //列表选择弹窗项文字
    uint8_t list_count;                   //列表选择弹窗项数量
    uint8_t list_select;                  //列表选择弹窗当前选中项
    float hl_sel_cur;                     //列表高亮条当前位置
    float hl_sel_trg;                     //列表高亮条目标位置
    float list_y;                         //列表垂直滚动位置
    float list_y_trg;                     //列表垂直滚动目标位置
    float hl_vel;                         //高亮条速度（弹簧动画用）
    float list_vel;                       //列表滚动速度（弹簧动画用）
    uint8_t bokeh_step;                   //背景虚化步骤（0-3）
    uint32_t last_bokeh_time;             //上次背景虚化更新时间
    float bar;                            //进度条位置（当前值）
    float bar_trg;                        //进度条位置（目标值）
    float y;                              //弹窗Y位置（当前值）
    float y_trg;                          //弹窗Y位置（目标值）
    float l;                              //弹窗左边距
    float u;                              //弹窗上边距
    float w;                              //弹窗宽度（当前值）
    float w_trg;                          //弹窗宽度（目标值）
    float h;                              //弹窗高度（当前值）
    float h_trg;                          //弹窗高度（目标值）
    float box_H;                          //弹窗拉伸高度（稳态值，仅WIN_STYLE=1用）
    float box_h;                          //弹窗拉伸高度（当前值）
    float box_h_trg;                      //弹窗拉伸高度（目标值）
    void (*list_on_close)(uint8_t);       //列表选择弹窗关闭回调函数
    uint8_t confirm_mode;                 //确认弹窗模式标志
    void (*confirm_on_close)(bool);       //确认弹窗关闭回调函数
    float conf_hl_cur;                    //确认弹窗高亮条X位置（当前值）
    float conf_hl_trg;                    //确认弹窗高亮条X位置（目标值）
    float conf_hl_vel;                    //确认弹窗高亮条速度（弹簧动画用）
};

/*
 * 聚光灯状态：管理聚光灯聚焦动画的XY位置
 * 
 * 用于某些页面的选中项高亮放大效果
 * x/y为当前聚光位置，x_trg/y_trg为目标位置
 */
struct SpotState {
    float x;           //聚光灯X位置（当前值）
    float y;           //聚光灯Y位置（当前值）
    float x_trg;       //聚光灯X位置（目标值）
    float y_trg;       //聚光灯Y位置（目标值）
};

/*
 * 关于页面状态：管理"关于本机"页面的信息滚动
 * 
 * index：当前显示的关于信息行索引
 * indi_x/indi_x_trg：指示器滑动动画位置
 */
struct AboutState {
    uint8_t index;           //当前显示的关于信息行
    float indi_x;            //指示器X位置（当前值）
    float indi_x_trg;        //指示器X位置（目标值）
};

/*
 * 旋钮状态：存储旋钮功能配置参数
 * 
 * param[0]：旋转功能（0=OFF, 1=VOL, 2=BRI）
 * param[1]：按键键码（键盘按键HID Usage ID）
 * 这些参数通过EEPROM持久化
 */
struct KnobState {
    uint8_t param[KNOB_PARAM];  //旋钮参数数组
};

/*
 * 按钮状态：管理EC11编码器旋钮的输入和蜂鸣器控制
 * 
 * 编码器信号处理：
 * 1. A相和B相正交信号，通过中断检测A相边沿
 * 2. 在A相边沿采样B相电平判断旋转方向
 * 3. 使用flag状态机避免机械抖动造成的重复触发
 * 
 * 按键处理：
 * 1. SW引脚低电平=按下（内部上拉）
 * 2. 5ms消抖确认电平稳定
 * 3. 自动区分短按和长按
 * 
 * 蜂鸣器控制：
 * buzzer_trig/buzzer_confirm/buzzer_exit/buzzer_boot
 * 分别对应四种音效的触发标志
 * buzzer_start记录播放开始时间
 */
struct ButtonState {
    bool pressed;           //是否有按钮事件发生（主循环中处理完毕后置false）
    uint8_t id;             //按钮事件ID（对应ButtonId枚举：CW/CC/SP/LP）
    bool alv;               //编码器A相当前电平
    bool blv;               //编码器B相当前电平
    bool flag;              //编码器状态机标志（防重复触发）
    bool CW_1;              //顺时针旋转判定标志1（A下降沿时B的电平）
    bool CW_2;              //顺时针旋转判定标志2（A上升沿时B的电平取反）
    bool CC_1;              //逆时针旋转判定标志1
    bool CC_2;              //逆时针旋转判定标志2
    bool pressed_1;         //按键按下检测标志1
    bool pressed_2;         //按键按下检测标志2
    bool long_pressed;      //长按事件标志
    uint32_t spt;           //短按时间阈值
    uint32_t lpt;           //长按时间阈值
    uint32_t spt_cnt;       //短按计数
    uint32_t lpt_cnt;       //长按计数
    bool buzzer_trig;       //蜂鸣器触发标志（旋转操作音）
    bool buzzer_confirm;    //蜂鸣器确认标志（确认操作音）
    bool buzzer_exit;       //蜂鸣器退出标志（退出操作音）
    bool buzzer_boot;       //蜂鸣器开机标志（开机提示音）
    uint32_t buzzer_start;  //蜂鸣器播放开始时间（毫秒）
};

/*
 * EEPROM状态：管理Flash仿真EEPROM的数据校验和写入状态
 * 
 * 通过Flash模拟EEPROM实现参数持久化：
 * - 在Flash中分配两个备份页，交替写入防止数据丢失
 * - 使用校验数组check_param[EEPROM_CHECK]检测数据完整性
 * - 允许一位误码（容错设计）
 * - change标志标记是否有未保存的修改
 */
struct EepromState {
    uint8_t check;          //校验结果（允许一位误码）
    uint16_t address;       //当前读写地址
    bool change;            //是否有数据变更需要保存
    uint8_t check_param[EEPROM_CHECK];  //校验参数数组
};

#endif