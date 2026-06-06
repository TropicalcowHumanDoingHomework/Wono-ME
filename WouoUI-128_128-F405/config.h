#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <U8g2lib.h>

/************************************* MCU检测 *************************************/

/*
 * MCU型号自动检测
 * 
 * 通过编译时预定义宏级联判断芯片型号
 * 用于在"关于本机"页面正确显示硬件信息
 * 优先级：STM32F405xx > STM32F407xx > STM32F4xx > ARM Cortex-M4 > 默认F103
 * 
 * SPI总线时钟固定为2MHz，满足LS013B7DH03数据手册要求的最低周期500ns
 */
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

/*
 * 夏普 LS013B7DH03 Memory LCD 128x128 驱动配置
 * 
 * 使用硬件SPI3外设（PC10=SCK, PC12=MOSI）进行通信
 * 初始化时通过U8g2的SW_SPI构造函数注册引脚，再注入SPI3硬件回调替换软件SPI
 * 
 * 屏幕特性：
 * - 反射式Memory LCD，无需背光
 * - SPI模式0（CPOL=0, CPHA=0）
 * - 数据手册最低时钟周期500ns，对应最高2MHz
 * - DC引脚在此方案中用作DISP（显示使能），非数据/命令切换
 * - EXTMODE引脚需接GND（使用内部VCOM）或接VDD（需外部EXTCOMIN方波）
 */
#define DISP_H 128
#define DISP_W 128
#define SCL PC10         //SPI3_SCK  SPI时钟
#define SDA PC12         //SPI3_MOSI SPI数据
#define RES U8X8_PIN_NONE //此屏幕未使用复位引脚
#define DC PC5           //LCD_DISP 显示使能
#define CS PC4           //LCD_CS   SPI片选

/************************************* UI配置 *************************************/

/*
 * UI系统核心配置
 * 
 * UI_DEPTH：页面层级最大深度
 * - 当前页面层级：M_MAIN(0) → M_EDITOR(1) → M_KNOB(2) → M_KRF(3)
 * - 设置20层足够未来扩展
 * 
 * UI_MNUMB：菜单项数组最大长度
 * - 最长的菜单是kpf_menu（82项）
 * - 设置100给未来扩展留有余量
 * 
 * UI_PARAM：用户可调参数总数
 * - 对应ParamIndex枚举中的28个参数
 * - 初始值在ui_param_init()中设置
 * - 通过EEPROM持久化保存
 */
#define UI_DEPTH 20
#define UI_MNUMB 20
#define UI_PARAM 31

/************************************* 磁贴配置 *************************************/

/*
 * 磁贴（Tile）页面布局配置
 * 
 * 所有磁贴页面共享同一套布局参数
 * 磁贴页面由三部分组成：图标区、大标题区、小标题区
 * 
 * 布局结构：
 * - 图标行（y=0~47）：128x48区域显示可横向滑动的图标
 * - 指示器行（y=57~96）：当前选中图标的大标题和指示器
 * - 底部小标题行（y=97~127）：对当前选中的功能进行说明
 * 
 * 图标间距TILE_ICON_S=57，比图标宽度48多9像素间距
 * 动画中图标切换时图标会水平滑动，间距决定滑动距离
 */
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

/*
 * 列表（List）页面布局配置
 * 
 * 列表页面用于显示可滚动的菜单项集合
 * 每行包含：弯曲偏移(可选) + 图标/文字 + 数值/复选框(可选)
 * 
 * 布局计算：
 * - 屏幕高度128，每行16，可完整显示8行
 * - 行首符号决定行尾控件类型：~数值 +复选框 =单选 #功能 $键值 *下拉
 * - LIST_BOX_R=0.5f圆角半径，值越大选择框越圆润
 * - LIST_CUR参数控制弯曲程度，可产生弧形列表的视觉效果
 */
#define LIST_FONT u8g2_font_HelvetiPixel_tr   //列表字体
#define LIST_TEXT_H 8                         //列表每行文字字体的高度
#define LIST_LINE_H 16                        //列表单行高度
#define LIST_TEXT_S 4                         //列表每行文字的上边距，左边距和右边距，下边距由它和字体高度和行高度决定
#define LIST_BAR_W 5                          //列表进度条宽度，需要是奇数，因为正中间有1像素宽度的线
#define LIST_BOX_R 0.5f                       //列表选择框圆角

/************************************* 电压测量配置 *************************************/

/*
 * 电压测量（Volt）页面布局配置
 * 
 * 显示ADC采样的实时波形和电压数值
 * 支持选择不同ADC通道（PA0~PB1共10个）
 * 
 * WAVE_SAMPLE=20：每波形点采集20次取平均，降低噪声
 * WAVE_MAX/MIN=43/5：波形在49像素高的边框内绘制
 * 范围映射：ADC值0~4095映射到WAVE_MIN~WAVE_MAX
 * 
 * 电压值计算：ADC读数/4096 * 3.3V（STM32F405参考电压）
 */
#define WAVE_SAMPLE 20
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

/*
 * 复选框/单选框布局配置
 * 
 * 行首符号'+'表示多选框（可同时选中多项）
 * 行首符号'='表示单选框（只能选中一项）
 * 
 * 每行右侧区域显示外框+内部填充点的形式
 * 选中时内部点填充，未选中时仅外框
 * 单选框点击自动切换选中项，多选框独立切换
 */
#define CHECK_BOX_L_S 95
#define CHECK_BOX_U_S 2                         //选择框在每行的上边距
#define CHECK_BOX_F_W 12                        //选择框外框宽度
#define CHECK_BOX_F_H 12                        //选择框外框高度
#define CHECK_BOX_D_S 2                         //选择框里面的点距离外框的边距

/************************************* 弹窗配置 *************************************/

/*
 * 弹窗（Window）基础布局配置
 * 
 * 基础弹窗（数值调节）的默认尺寸
 * 弹窗分为顶部消息区和中部进度条区
 * 
 * 弹窗Y位置：
 * - WIN_Y = -WIN_H - 2：弹窗初始位置（屏幕上方隐藏）
 * - WIN_Y_TRG = -WIN_H - 2：弹窗退场目标位置（滑出屏幕）
 * - 弹窗进场时从WIN_Y滑到win.y_trg（通常为屏幕中部）
 * 
 * WIN_W=102弹窗宽度（屏幕128，左右各留13像素边距）
 * WIN_STYLE=1时启用拉伸展开动画（弹窗从底部向上展开）
 */
#define WIN_FONT u8g2_font_HelvetiPixel_tr
#define WIN_H 32                              //弹窗高度
#define WIN_W 102                             //弹窗宽度
#define WIN_BAR_W 92                          //弹窗进度条宽度
#define WIN_BAR_H 7                           //弹窗进度条高度
#define WIN_Y (-WIN_H - 2)                    //弹窗竖直方向出场起始位置
#define WIN_Y_TRG (-WIN_H - 2)                //弹窗竖直方向退场终止位置
#define WIN_MSG_PAD 6                         //消息弹窗内边距

/************************************* 列表选择弹窗配置 *************************************/

// ==================== 列表选择弹窗配置 ====================
#define WIN_LIST_MAX 20                           //列表选择弹窗最大选项数
#define WIN_LIST_ITEM_LEN 24                      //列表选择弹窗选项文字最大长度

/************************************* 关于页面配置 *************************************/

/*
 * "关于本机"页面布局配置
 * 
 * 显示设备型号、RAM大小、Flash大小、主频等硬件信息
 * 
 * 布局结构：
 * - 顶部两行：标题"WouoUI"和版本号"v2.3"
 * - 左侧竖条指示器标记当前信息行位置
 * - 右侧列表显示各项硬件参数
 * 
 * ABOUT_INDI_S=4：指示器左边距和元素间距
 * ABOUT_INDI_W=2：指示器宽度（2像素竖条）
 */
#define ABOUT_FONT u8g2_font_HelvetiPixel_tr      //关于本机字体
#define ABOUT_INDI_S 4                            //关于本机页面列表指示左边距，也用于规范页面内元素之间的位置关系
#define ABOUT_INDI_W 2                            //关于本机页面列表指示器宽度

/************************************* 定义页面 *************************************/

/*
 * 页面索引枚举（PageIndex）
 * 
 * 定义系统所有页面的唯一标识符
 * 缩进表示页面之间的层级关系：
 * 
 * M_WINDOW(0) → 弹窗页（覆盖在其他页面之上）
 * M_SLEEP(1) → 睡眠页
 * M_MAIN(2) → 主磁贴页（最顶层）
 *   M_ANIMITION(3) → 动画设置列表页
 *   M_EDITOR(4) → 编辑器功能测试列表页
 *     M_KNOB(5) → 旋钮设置页
 *       M_KRF(6) → 旋钮旋转功能选择
 *       M_KPF(7) → 旋钮按键功能选择
 *   M_VOLT(8) → 电压测量页
 *   M_SETTING(9) → 系统设置页
 *     M_ABOUT(10) → 关于本机页
 * 
 * 页面切换规则：
 * - 按下级页面：层级+1，触发S_LAYER_IN
 * - 返回上级：层级-1，触发S_LAYER_OUT
 * - 弹窗：直接切换索引到M_WINDOW
 */

// ==================== 页面枚举 ====================
enum PageIndex {
  M_WINDOW,
  M_SLEEP,
    M_MAIN, 
      M_ANIMITION,
      M_EDITOR,
        M_KNOB,
          M_KRF,
          M_KPF,
      M_VOLT,
      M_USB,
        M_HID_KEY,
      M_SETTING,
        M_ABOUT,
};

/************************************* 状态枚举 *************************************/

/*
 * 页面状态枚举（PageState）
 * 
 * 描述页面切换时的过渡状态：
 * S_FADE：执行棋盘格/遮罩消失动画
 * S_WINDOW：弹窗初始化中（仅由pages.cpp内部使用）
 * S_LAYER_IN：新页面进场动画进行中
 * S_LAYER_OUT：旧页面退场动画进行中
 * S_NONE：空闲状态，直接渲染当前页面内容
 * 
 * 状态转换示例（主菜单→编辑器）：
 * S_LAYER_OUT（主菜单退场）→ S_FADE（画面渐出）→ S_LAYER_IN（编辑器进场）→ S_NONE
 */
enum PageState {
    S_FADE,       //转场动画
    S_WINDOW,     //弹窗初始化
    S_LAYER_IN,   //层级初始化
    S_LAYER_OUT,  //层级初始化
    S_NONE        //直接选择页面
};

/************************************* 参数枚举 *************************************/

/*
 * 用户可调参数索引枚举（ParamIndex）
 * 
 * 对应ui.param[UI_PARAM]数组的索引
 * 所有参数初始值在ui_param_init()中定义
 * 参数通过弹窗或复选框在设置页面中调节
 * 最终通过EEPROM持久化保存
 * 
 * 参数分类：
 * 0-10：显示和动画速度参数
 * 11-12：按钮时间参数
 * 13-17：开关选项（展开/循环/虚化/拉伸）
 * 18-20：硬件配置（旋钮方向/暗色模式/屏幕旋转/音量/USB）
 * 21-23：动画样式参数
 * 24-27：弹簧物理参数（用于Spring/Bounce动画模式）
 */
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
    USB_WP,       //USB写保护：0可写，1只读
    HID_ENABLE_SW,//HID开关：0禁用，1启用
    WIN_STYLE,    //弹窗动画样式：0=简单滑动 1=拉伸效果
    FADE_MODE,    //消失动画模式：0=棋盘格 1=整体遮罩
    HL_ANI_MODE,  //高亮条动画模式：0=Ease 1=Spring 2=Bounce 3=Gravity
    SPRING_K,     //弹簧刚度：10~100（实际值=参数/100）
    SPRING_D,     //弹簧阻尼：10~100（实际值=参数/100）
    CDC_ENABLE_SW //CDC虚拟串口开关：0禁用，1启用
};

/************************************* 旋钮配置 *************************************/

/*
 * EC11编码器旋钮引脚定义（SIQ-02FVS3型号）
 * 
 * EC11编码器特性：
 * - A相/B相输出正交方波信号（2-bit格雷码）
 * - 每步旋转产生一个完整的A/B相位差周期
 * - 内部无上拉电阻，需外部上拉（使用MCU内部上拉）
 * 
 * 引脚分配：
 * AIO(PC15)：A相信号，配置为外部中断输入（上升沿触发）
 * BIO(PC13)：B相信号，用于方向判断
 * SW(PC14)：按键信号，低电平有效
 * 
 * 编码器旋转判断逻辑：
 * 1. A相上升沿时采样B相
 * 2. B相=高→顺时针，B相=低→逆时针
 * 3. 采用状态机防抖
 * 
 * BUZ(PB14)：蜂鸣器PWM输出（TIM12_CH1）
 * RGB_R(PA8)/G(PA9)/B(PA10)：RGB LED共阳极控制
 * 
 * 睡眠模式下的旋钮功能配置：
 * KNOB_ROT_VOL=1：旋转控制音量（HID Consumer）
 * KNOB_ROT_BRI=2：旋转控制亮度（HID Consumer）
 * KNOB_DISABLE=0：禁用
 */
// ==================== 旋钮配置 ====================
#define AIO PC15         //EC_A  编码器A相
#define BIO PC13         //EC_B  编码器B相
#define SW PC14          //EC_KEY 编码器按键
#define BUZ PB14         //嗡鸣器
#define RGB_R PA8        //LED 红色通道
#define RGB_G PA9        //LED 绿色通道
#define RGB_B PA10       //LED 蓝色通道
#define KNOB_PARAM 5
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
    KNOB_COD_P,   //字符码在单选框中选择的位置
    KNOB_CASE     //字母大小写：0=小写，1=大写
};

/************************************* HID配置 *************************************/

// ==================== HID配置 ====================
//0=禁用USB HID，1=启用（复合MSC+HID模式）
#define HID_ENABLE 1

/************************************* USB MSC配置 *************************************/

/*
 * USB大容量存储设备（MSC）配置
 *
 * 硬件平台：STM32F405RGT6 + USB3300-EZK-TR (ULPI HS PHY) + USB Type-C
 * 使用 USB_OTG_HS 内核，通过 ULPI 8-bit 接口连接外部 PHY
 * 存储后端：W25Q512JVEIQ (512Mbit SPI NOR Flash)
 *
 * USB_MSC_ENABLE：
 * 0=禁用USB大容量存储
 * 1=启用（需在设置页面打开"USB Storage"开关）
 *
 * 磁盘特性：
 * - 64MB大小（W25Q512全容量），512字节/块
 * - 高速USB (480Mbps)，MSC端点包大小512B
 * - 4KB扇区缓存写入策略（读-改-擦-写）
 */
#define USB_MSC_ENABLE 1

/* ==================== W25Q512 SPI Flash 引脚定义 ==================== */
#define W25Q_SPI              SPI1
#define W25Q_SCK_PIN          PB3
#define W25Q_MISO_PIN         PA6
#define W25Q_MOSI_PIN         PA7
#define W25Q_CS_PIN           PA4

/* ==================== USB HS ULPI 引脚映射 ==================== */
/*
 * USB3300 ULPI 8-bit 接口引脚：
 * D[0:7] → PA3, PB0, PB1, PB10, PB11, PB12, PB13, PB5
 * STP → PC0, DIR → PC2, NXT → PC3, CLK(60MHz) → PA5
 * 全部使用 AF10 (GPIO_AFMODE_OTG_FS = OTG_HS)
 */
#define ULPI_D0_PIN           PA3
#define ULPI_D1_PIN           PB0
#define ULPI_D2_PIN           PB1
#define ULPI_D3_PIN           PB10
#define ULPI_D4_PIN           PB11
#define ULPI_D5_PIN           PB12
#define ULPI_D6_PIN           PB13
#define ULPI_D7_PIN           PB5
#define ULPI_CLK_PIN          PA5
#define ULPI_STP_PIN          PC0
#define ULPI_DIR_PIN          PC2
#define ULPI_NXT_PIN          PC3

/* OTG_HS IRQ号（STM32F405） */
#define OTG_HS_IRQn           77

/************************************* USB HID 键码 *************************************/

/*
 * USB HID标准键盘键码定义
 * 
 * 用于旋钮按键功能选择页面（M_KPF）
 * 用户在kpf菜单中选择按键时，对应的HID键码通过键盘发送
 * 
 * 键码范围：
 * 0-90：标准ASCII可打印字符（A-Z, 0-9）
 * 0xE0-0xE7：修饰键（Ctrl/Shift/Alt/Win）
 * 41-78：功能键和控制键（Esc, F1-F12, Enter等）
 * 79-82：方向键
 * 
 * 注意：键码定义用#ifndef包裹，防止与其他库重复定义
 */
#ifndef KEY_ESC
#define KEY_ESC          41     //Esc键
#define KEY_F1           58     //F1键
#define KEY_F2           59     //F2键
#define KEY_F3           60     //F3键
#define KEY_F4           61     //F4键
#define KEY_F5           62     //F5键
#define KEY_F6           63     //F6键
#define KEY_F7           64     //F7键
#define KEY_F8           0xE9  //F8键（0xE9避免与ASCII 'A'=65冲突）
#define KEY_F9           0xEA  //F9键（0xEA避免与ASCII 'B'=66冲突）
#define KEY_F10          0xEB  //F10键（0xEB避免与ASCII 'C'=67冲突）
#define KEY_F11          0xEC  //F11键（0xEC避免与ASCII 'D'=68冲突）
#define KEY_F12          0xED  //F12键（0xED避免与ASCII 'E'=69冲突）
#define KEY_LEFT_CTRL    0xE0  //左Ctrl
#define KEY_LEFT_SHIFT   0xE1  //左Shift
#define KEY_LEFT_ALT     0xE2  //左Alt
#define KEY_LEFT_GUI     0xE3  //左Win
#define KEY_RIGHT_CTRL   0xE4  //右Ctrl
#define KEY_RIGHT_SHIFT  0xE5  //右Shift
#define KEY_RIGHT_ALT    0xE6  //右Alt
#define KEY_RIGHT_GUI    0xE7  //右Win
#define KEY_CAPS_LOCK    0xE8  //大小写锁定（0xE8避免与ASCII '9'=57冲突）
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

/*
 * EEPROM校验和参数保存配置
 * 
 * 使用STM32F405的Flash模拟EEPROM进行配置持久化
 * 
 * EEPROM_CHECK=11：校验数组长度
 * - 存储11字节的校验码用于数据完整性检查
 * - 允许1位误码的容错机制
 * - 校验通过则从EEPROM读取配置，否则使用默认值
 * 
 * 数据在Flash中的布局：
 * [0~10]   校验码（11字节）
 * [11~41]  UI参数（31字节）
 * [42~45]  旋钮参数（4字节）
 * 
 * 写入时机：用户进入睡眠模式时自动写入
 */
#define EEPROM_CHECK 11

#endif
