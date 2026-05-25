#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <U8g2lib.h>

/************************************* 屏幕驱动 *************************************/

//分辨率128*128，支持 I2C 与 SPI 可选

// ==================== 屏幕配置 ====================
#define DISP_H 128
#define DISP_W 128

// 接口选择：取消注释 USE_I2C 使用 I2C（硬件 I2C），注释掉则使用硬件 SPI
#define USE_I2C

// I2C 引脚 (硬件 I2C1：PB6=SCL, PB7=SDA)
#define I2C_SCL PB6
#define I2C_SDA PB7

// SPI 引脚
#define SCL PA5
#define SDA PA7
#define RES U8X8_PIN_NONE
#define DC PB6
#define CS PB7

/************************************* UI配置 *************************************/

// ==================== UI配置 ====================
#define UI_DEPTH 20      //最深层级数
#define UI_MNUMB 100     //菜单数量
#define UI_PARAM 19      //参数数量

/************************************* 磁贴配置 *************************************/

//所有磁贴页面都使用同一套参数
// ==================== 磁贴配置 ====================
#define TILE_B_FONT u8g2_font_helvB24_tr        //磁贴大标题字体
#define TILE_S_FONT u8g2_font_HelvetiPixel_tr   //磁贴小标题字体
#define TILE_B_TITLE_H 25                       //磁贴大标题字体高度
#define TILE_S_TITLE_H 8                        //磁贴小标题字体高度
#define TILE_ICON_H 48                          //磁贴图标高度
#define TILE_ICON_W 48                          //磁贴图标宽度
#define TILE_ICON_S 57                          //磁贴图标间距
#define TILE_INDI_H 40                          //磁贴大标题指示器高度
#define TILE_INDI_W 10                          //磁贴大标题指示器宽度
#define TILE_INDI_S 57                          //磁贴大标题指示器上边距

/************************************* 列表配置 *************************************/

//默认参数
// ==================== 列表配置 ====================
#define LIST_FONT u8g2_font_HelvetiPixel_tr   //列表字体
#define LIST_TEXT_H 8                         //列表每行文字字体的高度
#define LIST_LINE_H 16                        //列表单行高度
#define LIST_TEXT_S 4                         //列表每行文字的上边距，左边距和右边距，下边距由它和字体高度和行高度决定
#define LIST_BAR_W 5                          //列表进度条宽度，需要是奇数，因为正中间有1像素宽度的线
#define LIST_BOX_R 0.5f                       //列表选择框圆角

/************************************* 电压测量配置 *************************************/

// ==================== 电压测量配置 ====================
#define WAVE_SAMPLE 20                          //采集倍数
#define WAVE_W DISP_W                           //波形宽度
#define WAVE_L 0                                //波形左边距
#define WAVE_U 0                                //波形上边距
#define WAVE_MAX 43                             //最大值
#define WAVE_MIN 5                              //最小值
#define WAVE_BOX_H 49                           //波形边框高度
#define WAVE_BOX_W DISP_W                       //波形边框宽度
#define VOLT_FONT u8g2_font_helvB24_tr          //电压数字字体
#define VOLT_LIST_U_S 94                        //列表上边距
#define VOLT_TEXT_BG_U_S 53                     //文字背景框上边距
#define VOLT_TEXT_BG_H 33                       //文字背景框高度

/************************************* 复选框配置 *************************************/

//默认参数
// ==================== 复选框配置 ====================
#define CHECK_BOX_L_S 95                        //选择框在每行的左边距
#define CHECK_BOX_U_S 2                         //选择框在每行的上边距
#define CHECK_BOX_F_W 12                        //选择框外框宽度
#define CHECK_BOX_F_H 12                        //选择框外框高度
#define CHECK_BOX_D_S 2                         //选择框里面的点距离外框的边距

/************************************* 弹窗配置 *************************************/

// ==================== 弹窗配置 ====================
#define WIN_FONT u8g2_font_HelvetiPixel_tr   //弹窗字体
#define WIN_H 32                              //弹窗高度
#define WIN_W 102                             //弹窗宽度
#define WIN_BAR_W 92                          //弹窗进度条宽度
#define WIN_BAR_H 7                           //弹窗进度条高度
#define WIN_Y (-WIN_H - 2)                    //弹窗竖直方向出场起始位置
#define WIN_Y_TRG (-WIN_H - 2)                //弹窗竖直方向退场终止位置
#define WIN_MSG_PAD 4                         //消息弹窗文字边距

/************************************* 关于页面配置 *************************************/

// ==================== 关于页面配置 ====================
#define ABOUT_FONT u8g2_font_HelvetiPixel_tr      //关于本机字体
#define ABOUT_INDI_S 4                            //关于本机页面列表指示左边距，也用于规范页面内元素之间的位置关系
#define ABOUT_INDI_W 2                            //关于本机页面列表指示器宽度

/************************************* 定义页面 *************************************/

// ==================== 页面枚举 ====================
//总目录，缩进表示页面层级
enum PageIndex {
  M_WINDOW,
  M_SLEEP,
    M_MAIN, 
      M_EDITOR,
        M_KNOB,
          M_KRF,
          M_KPF,
      M_VOLT,
      M_SETTING,
        M_ABOUT,
};

// ==================== 状态枚举 ====================
//状态，初始化标签
enum PageState {
    S_FADE,       //转场动画
    S_WINDOW,     //弹窗初始化
    S_LAYER_IN,   //层级初始化
    S_LAYER_OUT,  //层级初始化
    S_NONE        //直接选择页面
};

// ==================== 参数枚举 ====================
enum ParamIndex {
    DISP_BRI,     //屏幕亮度
    TILE_ANI,     //磁贴动画速度
    LIST_ANI,     //列表动画速度
    WIN_ANI,      //弹窗动画速度
    SPOT_ANI,     //聚光动画速度
    TAG_ANI,      //标签动画速度
    FADE_ANI,     //消失动画速度
    BTN_SPT,      //按键短按时长
    BTN_LPT,      //按键长按时长
    TILE_UFD,     //磁贴图标从头展开开关
    LIST_UFD,     //菜单列表从头展开开关
    TILE_LOOP,    //磁贴图标循环模式开关
    LIST_LOOP,    //菜单列表循环模式开关
    WIN_BOK,      //弹窗背景虚化开关
    KNOB_DIR,     //旋钮方向切换开关
    DARK_MODE,    //黑暗模式开关
    ROTATE_SCR,   //屏幕旋转：0=正常 1=右旋90° 2=180° 3=右旋270°
    LIST_CUR,     //列表弯曲程度 0~200
    SLP_T,        //休眠延时 0=关 1=10s 2=30s 3=60s 4=180s 5=300s
};

/************************************* 旋钮配置 *************************************/

//可按下旋钮引脚
// ==================== 旋钮配置 ====================
#define AIO PB12
#define BIO PB13
#define SW PB14
#define KNOB_PARAM 4
#define KNOB_DISABLE 0
#define KNOB_ROT_VOL 1
#define KNOB_ROT_BRI 2
#define BTN_PARAM_TIMES 2      //由于uint8_t最大值可能不够，但它存储起来方便，这里放大两倍使用

// ==================== 按钮ID枚举 ====================
enum ButtonId {
    BTN_ID_CC,    //逆时针旋转
    BTN_ID_CW,    //顺时针旋转
    BTN_ID_SP,    //短按
    BTN_ID_LP     //长按
};

// ==================== 旋钮参数枚举 ====================
enum KnobParamIndex {
    KNOB_ROT,     //睡眠下旋转旋钮的功能，0禁用，1音量，2亮度
    KNOB_COD,     //睡眠下短按旋钮输入的字符码，0禁用
    KNOB_ROT_P,   //旋转旋钮功能在单选框中选择的位置
    KNOB_COD_P    //字符码在单选框中选择的位置
};

/************************************* EEPROM配置 *************************************/

// ==================== EEPROM配置 ====================
#define EEPROM_CHECK 11

#endif