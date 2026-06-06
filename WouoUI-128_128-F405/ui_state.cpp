#include "ui_state.h"
#include "menu_data.h"
#include "eeprom_manager.h"

/************************************* UI状态变量定义 *************************************/

/*
 * 全局UI状态实例化
 * 
 * 所有UI状态结构体在此定义全局实例：
 * ui - 核心UI状态（页面索引、层级、参数等）
 * tile - 磁贴页面状态（图标动画、标题、指示器）
 * list - 列表页面状态（滚动、选择框、弯曲）
 * volt - 电压测量页面状态（波形数据、背景）
 * check_box - 复选框/数值控件状态
 * win - 弹窗状态（各模式共用）
 * spot - 聚光灯动画状态
 * about - 关于页面状态
 * knob - 旋钮参数状态
 */

UiState ui;
TileState tile;
ListState list;
VoltageState volt;
CheckBoxState check_box;
WindowState win;
SpotState spot;
AboutState about;
KnobState knob;

/************************************* UI参数默认值初始化 *************************************/

/*
 * UI参数默认值初始化
 * 
 * 在EEPROM初始化时调用（当Flash中无有效数据或校验失败时）
 * 定义所有用户可调参数的出厂默认值
 * 
 * 参数分组：
 * 0~9：动画速度参数（值越小动画越快）
 * 10~11：按键时间参数（扫描周期数）
 * 12~16：开关选项（0=关，1=开）
 * 17~20：硬件配置（方向/模式/旋转/音量/USB）
 * 21~22：动画样式（弹窗/消失）
 * 23~26：弹簧物理参数
 */
void ui_param_init() {
    ui.param[DISP_BRI] = 1;        //屏幕对比度：0=低 1=高
    ui.param[TILE_ANI] = 30;       //磁贴动画速度
    ui.param[LIST_CUR] = 0;        //列表弯曲程度
    ui.param[BOX_X_OS] = 10;       //选择框水平过度延伸
    ui.param[BOX_Y_OS] = 10;       //选择框竖直过度延伸
    ui.param[WIN_Y_OS] = 30;       //弹窗竖直过度延伸
    ui.param[LIST_ANI] = 60;       //列表动画速度
    ui.param[WIN_ANI] = 25;        //弹窗动画速度
    ui.param[SPOT_ANI] = 50;       //聚光动画速度
    ui.param[TAG_ANI] = 60;        //标签动画速度
    ui.param[FADE_ANI] = 0;        //消失动画速度
    ui.param[BTN_SPT] = 25;        //按键短按时长（扫描周期数）
    ui.param[BTN_LPT] = 150;       //按键长按时长（扫描周期数）
    ui.param[TILE_UFD] = 1;        //磁贴图标从头展开开关
    ui.param[LIST_UFD] = 1;        //菜单列表从头展开开关
    ui.param[TILE_LOOP] = 0;       //磁贴图标循环模式开关
    ui.param[LIST_LOOP] = 0;       //菜单列表循环模式开关
    ui.param[WIN_BOK] = 0;         //弹窗背景虚化开关
    ui.param[KNOB_DIR] = 0;        //旋钮方向切换开关
    ui.param[DARK_MODE] = 1;       //黑暗模式开关（默认开启）
    ui.param[ROTATE_SCR] = 0;      //屏幕旋转：0=正常
    ui.param[BUZ_VOL] = 2;         //嗡鸣器音量：0~4
    ui.param[USB_ENABLE] = 0;      //USB存储开关
    ui.param[USB_WP] = 0;          //USB写保护：0可写
    ui.param[HID_ENABLE_SW] = 1;   //HID开关：默认开启
    ui.param[WIN_STYLE] = 0;       //弹窗动画样式：0=简单滑动
    ui.param[FADE_MODE] = 0;       //消失动画模式：0=棋盘格
    ui.param[HL_ANI_MODE] = 0;     //高亮条动画模式：0=Ease
    ui.param[SPRING_K] = 25;       //弹簧刚度：25（实际0.25）
    ui.param[SPRING_D] = 70;       //弹簧阻尼：70（实际0.70）
    ui.param[CDC_ENABLE_SW] = 0;   //CDC虚拟串口：默认关闭
}

/************************************* 系统初始化 *************************************/

/*
 * 系统初始化函数
 * 
 * 在setup()中调用，完成以下工作：
 * 1. 预设各页面的菜单项数量
 * 2. 计算每屏幕可显示的列表行数
 * 3. 设置初始页面为主菜单，触发层级进入动画
 * 
 * 页面数量说明：
 * M_MAIN = 5：休眠、编辑、电压、动画、设置
 * M_ANIMITION = 21：动画/开关参数共20项 + 返回项
 * M_EDITOR = 12：各种测试项 + 返回项
 * M_KNOB = 3：返回、旋转功能、按键功能
 * M_KRF = 7：OFF/VOL/BRI + 分隔行
 * M_KPF = 82：各种键码 + 分隔行
 * M_VOLT = 10：10个ADC通道
 * M_SETTING = 10：系统参数 + 返回项
 * M_ABOUT = 8：关于信息行
 */
void ui_init() {
    ui.index = M_MAIN;      //启动时进入主菜单
    ui.state = S_LAYER_IN;  //触发层级初始化，调用tile_param_init设置动画初始值
    ui.num[M_MAIN] = 6;
    ui.num[M_ANIMITION] = 21;
    ui.num[M_EDITOR] = 11;
    ui.num[M_KNOB] = 3;
    ui.num[M_KRF] = 7;
    ui.num[M_KPF] = 82;
    ui.num[M_VOLT] = 10;
    ui.num[M_USB] = 6;
    ui.num[M_HID_KEY] = 4;
    ui.num[M_SETTING] = 9;
    ui.num[M_ABOUT] = 8;
    list.line_n = DISP_H / LIST_LINE_H;
}


