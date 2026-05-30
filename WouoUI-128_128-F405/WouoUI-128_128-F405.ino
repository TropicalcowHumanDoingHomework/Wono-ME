
/*
  WouoUI v2.3 - F405 移植版
  基于 STM32F103C8 原版移植至 STM32F405 (168MHz / 192KB SRAM / 1024KB Flash)

  原版说明：

  此项目模仿自稚晖君暂未开源的 MonoUI，用于实现类似 UltraLink 的丝滑界面

  有四个版本：

    * 分辨率 128 * 128 ：主菜单，列表和关于本机页基本还原了 UltraLink 的界面和动态效果，电压测量页为原创设计
    * 分辨率 128 * 64 ：主菜单模仿 UltraLink 重新设计，去掉了小标题，电压测量页重新设计，关于本机页面改为列表，列表适配了该分辨率
    * 分辨率 128 * 32 ：在 128 * 64 分辨率的基础上，主界面只保留图标，电压测量页重新设计
    * 通用版本：仅保留列表，主菜单也改为列表，删除电压测量页和与列表无关的动画（保留弹窗效果），经过简单修改可以适配任何分辨率，任何行高度的情况

  WouoUI v2 功能：

    * 全部使用非线性的平滑缓动动画，包括列表，弹窗，甚至进度条
    * 优化平滑动画算法到只有两行，分类别定义平滑权重，并且每个权重值都分别可调
    * 可以打断的非线性动画，当前动画未结束但下一次动画已经被触发时，动画可以自然过渡
    * 非通用版本分别适配了类似 UltraLink 主菜单的磁贴界面（因为让我想起 WP7 的 Metron 风格，所以称之为磁贴）
    * 通用版本仅保留列表类界面，经过简单修改可以适配所有分辨率的屏幕，包括屏幕内行数不是整数的情况
    * 列表菜单，列表可以无限延长
    * 列表文字选择框，选择框可根据选择的字符串长度自动伸缩，进入菜单时从列表开头从长度 0 展开，转到上一级列表时，长度和纵坐标平滑移动到上一级选择的位置
    * 列表单选框，储存数据时也储存该值在列表中所在的位置，展开列表时根据每行开头的字符判断是否绘制外框，再根据位置数据判断是否绘制里面的点
    * 列表多选框，储存数据的数组跟多选框列表的行数对应，不要求连续排列，展开列表时根据每行开头的字符判断是否绘制外框，再根据行数对应的储存数据位置的数值是否为1判断是否绘制里面的点
    * 列表显示数值，与多选框原理相似，但不涉及修改操作
    * 列表展开动画，初始化列表时，可以选择列表从头开始展开，或者从上次选中的位置展开
    * 图标展开动画，初始化磁贴类界面时，可以选择图标从头开始展开，或者从上次选中的位置展开
    * 弹出窗口，实现了窗口弹出的动画效果，可以自定义最大值，最小值，步进值，需要修改的参数等，窗口独立运行，调用非常简单
    * 弹出窗口背景虚化可选项，背景虚化会产生卡顿感，但删掉代码有些可惜，因此做成可选项，默认关闭
    * 亮度调节，在弹出窗口中调节亮度值可以实时改变当前亮度值
    * 旋钮功能，使用EC11旋钮控制，旋钮方向可以软件调整，内置一个USB控制电脑的示例，在睡眠模式下旋转调整音量或者亮度，短按输入一个键值，长按进入主菜单，旋钮消抖时长等参数可以在弹出窗口中调整
    * 循环模式，选择项超过头尾时，选择项跳转到另一侧继续，列表和磁贴类可以分别选择
    * 黑暗模式，其实本来就是黑暗模式，是增加了白天模式，默认开启黑暗模式
    * 消失动画适配两种模式，一种是渐变成全黑，另一种渐变成全白
    * 断电存储，用简单直观的方式将每种功能参数写入EEPROM，只在修改过参数，进入睡眠模式时写入，避免重复擦写，初始化时检查11个标志位，允许一位误码

  项目参考：

    * B站：路徍要什么自行车：在线仿真：https://wokwi.com/projects/350306511434547796，https://www.bilibili.com/video/BV1HA411S7pv/
    * Github：createskyblue：OpenT12：https://github.com/createskyblue/OpenT12
    * Github：peng-zhihui：OpenHeat：https://github.com/peng-zhihui/OpenHeat

  注意事项：

    * 为防止使用者在上传程序后无法直接使用，认为是代码有问题，HID功能默认禁用，如需使用旋钮音量控制和点按输入功能，请在初始化函数中启用相关功能，上传成功后设置两个跳线帽都为0，断电再插上 USB 线
    * F405 版本使用 config.h 中自动检测 MCU 型号，关于本机页面自动显示对应硬件信息
    * 若需切换到其他 F4 系列芯片，无需修改代码，config.h 会自动适配

  本项目不设置开源协议，如需商用或借鉴，在醒目处标注本项目开源地址即可
  欢迎关注我的B站账号，用户名：音游玩的人，B站主页：https://space.bilibili.com/9182439?spm_id_from=..0.0
*/

#include "config.h"
#include "ui_types.h"
#include "display.h"
#include "ui_state.h"
#include "animation.h"
#include "menu_data.h"
#include "eeprom_manager.h"
#include "knob.h"
#include "window.h"
#include "pages.h"
#include "hid_manager.h"
#include "usb_manager.h"
#include "usb_debug.h"
#include "led.h"

/* ==================== USB 非阻塞轮询状态 ==================== */
#if USB_MSC_ENABLE
static uint32_t usb_poll_start  = 0;
static bool     usb_poll_active = false;
static bool     usb_mounted     = false;
#endif

/* ==================== 硬件早期初始化 ==================== */

/*
 * 在 setup() 最开头调用，抢占变体初始化可能的 GPIO 冲突
 *
 * DISCO_F407VG 变体初始化会将部分 ULPI 引脚配为其他功能：
 *   PB5  → 音频 DAC (I2S3_SD)
 *   PB10 → I2C2_SCL
 *   PB11 → I2C2_SDA
 * 此函数强制将它们重新配为 AF10 (OTG_HS ULPI)
 *
 * 使用与 display.cpp 同风格的直接寄存器访问（REG32 宏）
 * 避免 STM32duino 核心的 GPIO 抽象层差异
 */

#define REG32(addr) (*(volatile uint32_t *)(addr))
#define RCC_BASE    0x40023800u
#undef GPIOA_BASE
#define GPIOA_BASE  0x40020000u
#undef GPIOB_BASE
#define GPIOB_BASE  0x40020400u
#undef GPIOC_BASE
#define GPIOC_BASE  0x40020800u

#define GPIO_MODER(b)   REG32((b) + 0x00u)
#define GPIO_OSPEEDR(b) REG32((b) + 0x08u)
#define GPIO_PUPDR(b)   REG32((b) + 0x0Cu)
#define GPIO_AFRL(b)    REG32((b) + 0x20u)
#define GPIO_AFRH(b)    REG32((b) + 0x24u)

/* AF10 = OTG_HS ULPI, MODE=2=AF, SPEED=3=100MHz */
#define ULPI_AF_VAL  (10u)
#define AF_MODE_VAL  (2u)
#define HS_SPEED_VAL (3u)

static void ulpi_pin_cfg(uint32_t gpio, uint8_t pin)
{
    uint32_t s2 = (uint32_t)pin * 2u;
    /* MODER: 10 = Alternate Function */
    GPIO_MODER(gpio)   = (GPIO_MODER(gpio)   & ~(0x3u << s2)) | (AF_MODE_VAL << s2);
    /* OSPEEDR: 11 = Very High */
    GPIO_OSPEEDR(gpio) = (GPIO_OSPEEDR(gpio) & ~(0x3u << s2)) | (HS_SPEED_VAL << s2);
    /* AFR: 1010 = AF10 */
    if (pin < 8u) {
        uint32_t s4 = (uint32_t)pin * 4u;
        GPIO_AFRL(gpio) = (GPIO_AFRL(gpio) & ~(0xFu << s4)) | ((uint32_t)ULPI_AF_VAL << s4);
    } else {
        uint32_t s4 = ((uint32_t)pin - 8u) * 4u;
        GPIO_AFRH(gpio) = (GPIO_AFRH(gpio) & ~(0xFu << s4)) | ((uint32_t)ULPI_AF_VAL << s4);
    }
}

static void hardware_early_init(void)
{
    /* ──── 使能 GPIO 时钟 ──── */
    REG32(RCC_BASE + 0x30u) |= (1u << 0) | (1u << 1) | (1u << 2);
    __asm volatile ("dsb");
    __asm volatile ("isb");

    /* Port A: PA3(D0), PA5(CLK) */
    ulpi_pin_cfg(GPIOA_BASE, 3);
    ulpi_pin_cfg(GPIOA_BASE, 5);

    /* Port B: PB0(D1), PB1(D2), PB5(D7), PB10(D3), PB11(D4), PB12(D5), PB13(D6) */
    ulpi_pin_cfg(GPIOB_BASE, 0);
    ulpi_pin_cfg(GPIOB_BASE, 1);
    ulpi_pin_cfg(GPIOB_BASE, 5);
    ulpi_pin_cfg(GPIOB_BASE, 10);
    ulpi_pin_cfg(GPIOB_BASE, 11);
    ulpi_pin_cfg(GPIOB_BASE, 12);
    ulpi_pin_cfg(GPIOB_BASE, 13);

    /* Port C: PC0(STP), PC2(DIR), PC3(NXT) */
    ulpi_pin_cfg(GPIOC_BASE, 0);
    ulpi_pin_cfg(GPIOC_BASE, 2);
    ulpi_pin_cfg(GPIOC_BASE, 3);

    __asm volatile ("dsb");
    __asm volatile ("isb");
}

void setup() {
  hardware_early_init();  /* 第一行：抢占变体初始化 */

  led_init();
  eeprom_init();
  ui_init();
  lcd_init();

  /* ========== 系统启动第一阶段：USB 调试屏幕 ==========
   * 屏幕点亮后立即渲染 USB 初始化进度
   * 等枚举完成（或超时）后才继续进入 UI 初始化
   */
  usb_debug_reset();
  usb_debug_refresh();

#if USB_MSC_ENABLE
  if (ui.param[USB_ENABLE]) {
    usb_debug_set_msg("Init W25Q512...");
    usb_debug_refresh();
    USBManager::registerComponent();

    /* === 诊断：progress 变量跟踪 usbd_msc_reinit 执行进度 === */
    #define USB_DBG_SET_STEP(n,s) do { usb_debug_set_step(n,s); usb_debug_refresh(); delay(50); } while(0)

    usb_debug_reset();
    usb_debug_set_msg("USB init...");
    USB_DBG_SET_STEP(USB_STEP_CLOCK, USB_STATUS_BUSY); // step0=[**]

    USBManager::begin();   // 内部会调用 usbd_msc_reinit

    /* begin() 返回表示 usbd_msc_reinit 执行完毕 */
    USB_DBG_SET_STEP(USB_STEP_CLOCK,   USB_STATUS_OK);
    USB_DBG_SET_STEP(USB_STEP_GPIO,    USB_STATUS_OK);
    USB_DBG_SET_STEP(USB_STEP_PHY,     USB_STATUS_OK);
    USB_DBG_SET_STEP(USB_STEP_DCD,     USB_STATUS_OK);
    USB_DBG_SET_STEP(USB_STEP_SPEED,   USB_STATUS_OK);
    USB_DBG_SET_STEP(USB_STEP_VBUS,    USB_STATUS_OK);
    USB_DBG_SET_STEP(USB_STEP_CONNECT, USB_STATUS_OK);
    USB_DBG_SET_STEP(USB_STEP_DONE,    USB_STATUS_BUSY);  // 等待枚举
    usb_debug_set_msg("Waiting enumeration...");
    usb_debug_refresh();
    delay(50);

    /* 非阻塞：显示等待后直接进入 UI，轮询逻辑交给 loop() */
    usb_poll_start  = millis();
    usb_poll_active = true;
  } else {
    usb_debug_set_msg("USB disabled in settings");
    usb_debug_refresh();
    delay(800);
  }
#else
  usb_debug_set_msg("USB_MSC not compiled in");
  usb_debug_refresh();
  delay(800);
#endif
  /* ================================================= */

  btn_init();

#if HID_ENABLE
  hid_init();
#endif

  buzzer_boot_sound();
}

void loop() {
    btn_scan();
    ui_proc();
    buzzer_proc();
    led_proc();
#if USB_MSC_ENABLE
    if (usb_poll_active) {
        if (USBManager::poll()) {
            usb_mounted = true;
            usb_poll_active = false;
        } else if (millis() - usb_poll_start > 20000) {
            usb_poll_active = false;
        }
    }
    if (usb_mounted) {
        led_set_green();        /* USB 成功 → 覆盖为绿色（不影响 UI 原有 LED 逻辑） */
    }
#endif
}

