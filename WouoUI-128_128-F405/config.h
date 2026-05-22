#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <U8g2lib.h>

/************************************* MCU检测 *************************************/

//自动编译时检测芯片型号，关于本机页面正确显示
//STM32F405xx/STM32F407xx在Arduino环境不一定定义，用级联回退
// ==================== MCU检测 ====================
#if defined(STM32F405xx)
  #define MCU_BOARD  "STM32F405"
  #define MCU_RAM    "192k"
  #define MCU_FLASH  "1024k"
  #define MCU_FREQ   "168Mhz"
  #define SPI_BUS_CLOCK 2000000            //LS013B7DH03数据手册最低周期500ns=2MHz
#elif defined(STM32F407xx)
  #define MCU_BOARD  "STM32F407"
  #define MCU_RAM    "192k"
  #define MCU_FLASH  "1024k"
  #define MCU_FREQ   "168Mhz"
  #define SPI_BUS_CLOCK 2000000
#elif defined(STM32F4xx)
  #define MCU_BOARD  "STM32F4xx"
  #define MCU_RAM    "192k"
  #define MCU_FLASH  "1024k"
  #define MCU_FREQ   "168Mhz"
  #define SPI_BUS_CLOCK 2000000
#elif defined(__ARM_ARCH_7EM__)
  #define MCU_BOARD  "STM32F4xx"
  #define MCU_RAM    "192k"
  #define MCU_FLASH  "1024k"
  #define MCU_FREQ   "168Mhz"
  #define SPI_BUS_CLOCK 2000000
#elif __CORTEX_M == 4
  #define MCU_BOARD  "STM32F4xx"
  #define MCU_RAM    "192k"
  #define MCU_FLASH  "1024k"
  #define MCU_FREQ   "168Mhz"
  #define SPI_BUS_CLOCK 2000000
#else
  #define MCU_BOARD  "STM32F103"
  #define MCU_RAM    "20k"
  #define MCU_FLASH  "64k"
  #define MCU_FREQ   "72Mhz"
  #define SPI_BUS_CLOCK 2000000            //LS013B7DH03数据手册最低周期500ns=2MHz
#endif

/************************************* 屏幕驱动 *************************************/

//夏普 LS013B7DH03 Memory LCD 128*128，硬件SPI3
//使用SW_SPI构造函数注册引脚 + lcd_init中注入SPI3硬件回调
//SPI模式0（CPOL=0,CPHA=0），数据手册最低时钟周期500ns=2MHz
//DC引脚在此屏幕为DISP（显示使能），非数据/命令切换
//EXTMODE引脚接GND使用内部VCOM，或接VDD需外部EXTCOMIN方波

// ==================== 屏幕配置 ====================
#define DISP_H 128
#define DISP_W 128
#define SCL PC10         //SPI3_SCK  SPI时钟
#define SDA PC12         //SPI3_MOSI SPI数据
#define RES U8X8_PIN_NONE //此屏幕未使用复位引脚
#define DC PC5           //LCD_DISP 显示使能
#define CS PC4           //LCD_CS   SPI片选

/************************************* UI配置 *************************************/

// ==================== UI配置 ====================
#define UI_DEPTH 20      //最深层级数
#define UI_MNUMB 100     //菜单数量
#define UI_PARAM 24      //参数数量

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
#define WIN_MSG_PAD 6                         //消息弹窗内边距

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
      M_ANIMITION,
      M_EDITOR,
        M_WIN_LIST_DEMO,
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
    LIST_CUR,     //列表弯曲程度
    BOX_X_OS,     //选择框水平过度延伸
    BOX_Y_OS,     //选择框竖直过度延伸
    WIN_Y_OS,     //弹窗竖直过度延伸
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
    BUZ_VOL,      //嗡鸣器音量：0~4
    USB_ENABLE,   //USB存储开关：0禁用，1启用
    WIN_STYLE     //弹窗动画样式：0=简单滑动 1=拉伸效果
};

/************************************* 旋钮配置 *************************************/

//可按下旋钮引脚（SIQ-02FVS3）
// ==================== 旋钮配置 ====================
#define AIO PC15         //EC_A  编码器A相
#define BIO PC13         //EC_B  编码器B相
#define SW PC14          //EC_KEY 编码器按键
#define BUZ PB14         //嗡鸣器
#define RGB_R PA8        //LED 红色通道
#define RGB_G PA9        //LED 绿色通道
#define RGB_B PA10       //LED 蓝色通道
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

/************************************* HID配置 *************************************/

// ==================== HID配置 ====================
//0=禁用USB HID（如未安装USBComposite库），1=启用
#define HID_ENABLE 0

/************************************* USB MSC配置 *************************************/

// ==================== USB MSC配置 ====================
//0=禁用USB大容量存储（如未安装USBComposite库），1=启用
//启用后可在设置菜单中打开"USB Storage"开关
#define USB_MSC_ENABLE 0

/************************************* USB HID 键码 *************************************/

// ==================== HID键盘键码 ====================
//对应USB HID标准键盘键码，用于旋钮按键功能选择页面
#ifndef KEY_ESC
#define KEY_ESC          41     //Esc键
#define KEY_F1           58     //F1键
#define KEY_F2           59     //F2键
#define KEY_F3           60     //F3键
#define KEY_F4           61     //F4键
#define KEY_F5           62     //F5键
#define KEY_F6           63     //F6键
#define KEY_F7           64     //F7键
#define KEY_F8           65     //F8键
#define KEY_F9           66     //F9键
#define KEY_F10          67     //F10键
#define KEY_F11          68     //F11键
#define KEY_F12          69     //F12键
#define KEY_LEFT_CTRL    0xE0  //左Ctrl
#define KEY_LEFT_SHIFT   0xE1  //左Shift
#define KEY_LEFT_ALT     0xE2  //左Alt
#define KEY_LEFT_GUI     0xE3  //左Win
#define KEY_RIGHT_CTRL   0xE4  //右Ctrl
#define KEY_RIGHT_SHIFT  0xE5  //右Shift
#define KEY_RIGHT_ALT    0xE6  //右Alt
#define KEY_RIGHT_GUI    0xE7  //右Win
#define KEY_CAPS_LOCK    57    //大小写锁定
#define KEY_BACKSPACE    42    //退格键
#define KEY_RETURN       40    //回车键
#define KEY_INSERT       73    //Insert键
#define KEY_DELETE       76    //Delete键
#define KEY_TAB          43    //Tab键
#define KEY_HOME         74    //Home键
#define KEY_END          77    //End键
#define KEY_PAGE_UP      75    //Page Up键
#define KEY_PAGE_DOWN    78    //Page Down键
#define KEY_UP_ARROW     82    //上箭头
#define KEY_DOWN_ARROW   81    //下箭头
#define KEY_LEFT_ARROW   80    //左箭头
#define KEY_RIGHT_ARROW  79    //右箭头
#endif

/************************************* EEPROM配置 *************************************/

// ==================== EEPROM配置 ====================
#define EEPROM_CHECK 11

#endif
