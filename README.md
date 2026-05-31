<p align="center">
  <img src="https://img.shields.io/badge/MCU-STM32F103C8-blue?style=flat-square" alt="F103"/>
  <img src="https://img.shields.io/badge/MCU-STM32F405-00B4AB?style=flat-square" alt="F405"/>
  <img src="https://img.shields.io/badge/Tool-Python-3776AB?style=flat-square&logo=python" alt="Python"/>
  <img src="https://img.shields.io/badge/License-Apache%202.0-green?style=flat-square" alt="License"/>
  <img src="https://img.shields.io/badge/Version-v2.4-orange?style=flat-square" alt="Version"/>
</p>

<h1 align="center">Wonome — WouoUI 项目合集</h1>

<p align="center">
  <strong>WouoUI</strong> 嵌入式 UI 框架的多平台移植版与配套工具合集
  <br>
  基于稚晖君 <strong>MonoUI</strong> 设计理念，由 <strong>音游玩的人</strong> 原版开发，
  <br>
  <strong>STM32F103C8</strong> 与 <strong>STM32F405</strong> 移植版由 <strong>罗米奇</strong> 移植维护
</p>

<p align="center">
  <a href="#项目简介"><strong>项目简介</strong></a> ·
  <a href="#项目列表"><strong>项目列表</strong></a> ·
  <a href="#项目结构"><strong>项目结构</strong></a> ·
  <a href="#功能特性"><strong>功能特性</strong></a> ·
  <a href="#快速开始"><strong>快速开始</strong></a> ·
  <a href="#硬件连接"><strong>硬件连接</strong></a> ·
  <a href="#许可证"><strong>许可证</strong></a>
</p>

---

## 项目简介

**Wonome** 是以 **WouoUI** 为核心的嵌入式 UI 生态合集，旨在低性能 MCU 上复现 **UltraLink** 风格的丝滑动画界面（类似 Windows Phone 7 Metro 磁贴效果）。

- **WouoUI 原版**由 B 站 UP 主 [音游玩的人](https://space.bilibili.com/9182439) 开发，灵感来源于稚晖君的 MonoUI 设计理念
- **F103 移植版**（v2.3）基于 STM32F103C8（72MHz / 20KB SRAM / 64KB Flash），驱动 OLED 128x128（I2C），同时支持 Arduino IDE 和 Keil MDK 编译
- **F405 移植增强版**（v2.4）基于 STM32F405（168MHz / 192KB SRAM / 1024KB Flash），驱动 Sharp Memory LCD 128x128（SPI），Arduino 工程
- **图标编辑器**为配套的像素图标绘制工具（Python / tkinter），支持 PNG 导入/导出、C 代码生成

---

## 项目列表

| 项目 | 平台 | 版本 | 工程类型 | 说明 |
|:-----|:-----|:----:|:---------|:------|
| [**WouoUI-128_128-F103**](WouoUI-128_128-F103/) | STM32F103C8 | v2.3 | Arduino / Keil MDK | F103 移植版，驱动 OLED 128x128（I2C），72MHz / 20KB SRAM / 64KB Flash |
| [**WouoUI-128_128-F405**](WouoUI-128_128-F405/) | STM32F405 | v2.4 | Arduino | F405 移植增强版，驱动 Sharp LS013B7DH03 Memory LCD，168MHz / 192KB SRAM / 1024KB Flash |
| [**图标编辑器**](icon_editor.py) | Python 3.8+ | — | 脚本 | 配套像素图标编辑工具，支持 PNG 导入/导出、C 代码生成、可变画布 |

### WouoUI-128_128-F103（F103 移植版 v2.3）

基于 **STM32F103C8**（72MHz / 20KB SRAM / 64KB Flash）的嵌入式 UI 框架，驱动 **OLED 128x128**（I2C 接口），同时支持 **Arduino IDE** 和 **Keil MDK** 编译，完整复现 WouoUI 的动画页面系统。

> 详细文档见 [WouoUI-128_128-F103/README.md](WouoUI-128_128-F103/README.md)

### WouoUI-128_128-F405（F405 移植增强版 v2.4）

基于 **STM32F405**（168MHz / 192KB SRAM / 1024KB Flash）的增强版嵌入式 UI 框架，驱动 **Sharp LS013B7DH03** 128x128 Memory LCD（SPI 接口），Arduino 工程，在原版基础上新增弹簧物理动画系统、USB MSC 大容量存储、USB HID、RGB LED 呼吸灯、蜂鸣器等功能。

> 详细文档见 [WouoUI-128_128-F405/README.md](WouoUI-128_128-F405/README.md)

### 图标编辑器

WouoUI 配套的像素图标编辑工具（Python / tkinter），支持可变尺寸画布（8×8 ~ 256×256）、PNG 导入/导出、C 代码实时预览与导出，直接生成 `PROGMEM const uint8_t` 格式的字节数组，即编即用。

```bash
python icon_editor.py
```

---

## 项目结构

```
Wonome/
│
├── WouoUI-128_128-F103/            # STM32F103C8 移植版（Arduino / Keil MDK）
│   ├── WouoUI-128_128-F103.ino     # 主程序入口（Arduino Sketch）
│   ├── README.md                   # 详细文档
│   │
│   └── WouoUI_Port/                # 原版 WouoUI 移植层
│       ├── USER/                   # Keil MDK 工程 + 系统配置
│       │   ├── main.c              # 主函数
│       │   ├── stm32f10x_it.c / .h # 中断服务
│       │   ├── stm32f10x_conf.h    # 外设库配置
│       │   ├── system_stm32f10x.c / .h # 系统时钟
│       │   ├── WouoUI.uvprojx      # Keil MDK 工程文件
│       │   └── ...
│       ├── CORE/                   # Cortex-M3 核心支持
│       │   ├── core_cm3.c / .h
│       │   ├── startup_stm32f10x_md.s
│       │   └── startup_stm32f10x_hd.s
│       ├── SYSTEM/                 # 延时与系统函数
│       │   ├── delay/
│       │   │   └── delay.c / .h
│       │   └── sys/
│       │       └── sys.c / .h
│       ├── STM32F10x_FWLib/        # STM32F10x 标准外设库
│       │   ├── src/                # 源文件
│       │   │   ├── stm32f10x_gpio.c
│       │   │   ├── stm32f10x_rcc.c
│       │   │   ├── stm32f10x_tim.c
│       │   │   └── ...
│       │   └── inc/                # 头文件
│       │       └── ...
│       ├── HARDWARE/
│       │   ├── OLED/               # OLED 驱动
│       │   │   ├── oled.c / .h     # 模拟 I2C OLED 驱动
│       │   │   ├── oledfont.h      # 字库
│       │   │   └── bmp.h           # 位图数据
│       │   └── WouoUI/             # WouoUI 框架核心
│       │       ├── config.h        # 全局配置与引脚定义
│       │       ├── ui_types.h      # 数据结构定义
│       │       ├── ui_state.c / .h # UI 全局状态
│       │       ├── display.h       # 显示接口声明
│       │       ├── u8g2_adapter.c / .h  # U8g2 API 适配层
│       │       ├── u8g2_global.cpp # U8g2 全局 C++ 对象
│       │       ├── animation.c / .h     # 动画引擎
│       │       ├── pages.cpp / .h       # 页面渲染与交互逻辑
│       │       ├── menu_data.c / .h     # 菜单文本与图标数据
│       │       ├── window.c / .h        # 弹窗系统
│       │       ├── knob.c / .h          # 旋钮轮询 + 按键扫描
│       │       ├── eeprom_manager.c / .h   # RAM EEPROM 模拟
│       │       └── hw_abstraction.h    # 硬件抽象层
│       └── OBJ/                   # 编译输出（.o / .hex / .axf）
│
├── WouoUI-128_128-F405/            # STM32F405 移植增强版（Arduino）
│   ├── WouoUI-128_128-F405.ino     # 主程序入口（Arduino Sketch）
│   ├── config.h                    # 全局配置（含自动 MCU 检测）
│   ├── ui_types.h                  # 数据结构定义
│   ├── ui_state.cpp / .h           # UI 全局状态
│   ├── display.cpp / .h            # Sharp Memory LCD 驱动（U8g2 + SPI3）
│   ├── animation.cpp / .h          # 动画引擎（缓动 + 弹簧 + 弹跳 + 淡出）
│   ├── pages.cpp / .h              # 页面渲染与交互逻辑
│   ├── menu_data.cpp / .h          # 菜单文本与图标数据
│   ├── window.cpp / .h             # 弹窗系统（增强版：值/消息/列表选择/确认）
│   ├── knob.cpp / .h               # 旋钮中断 + 按键扫描 + 蜂鸣器
│   ├── eeprom_manager.cpp / .h     # Flash 仿真 EEPROM（磨损均衡）
│   ├── EEPROM.h                    # Flash EEPROM 仿真实现
│   ├── led.cpp / .h                # RGB LED 软件 PWM 呼吸灯
│   ├── hid_manager.cpp / .h        # USB HID 键盘/多媒体
│   ├── usb_manager.cpp / .h        # USB MSC 大容量存储（64KB 虚拟磁盘）
│   ├── usb_debug.cpp / .h          # USB 调试输出
│   ├── README.md                   # 详细文档
│   ├── LICENSE                     # Apache 2.0
│   └── icons_export/               # 导出的图标 PNG 文件
│
├── icon_editor.py                  # 图标编辑器（Python 单文件工具）
├── README.md                       # 本说明文档
```

---

## 功能特性

### 动画引擎

WouoUI 的核心亮点 —— 在极低硬件资源上实现流畅的动画效果。

- **非线性平滑缓动** — 列表、弹窗、进度条全部使用缓动动画
- **双行算法** — 平滑动画核心仅两行代码，六类动画权重各自独立可调
  ```c
  a += (target - a) / (param / 10.0f);
  ```
- **可打断过渡** — 前一次动画未结束时触发新动画，新旧状态自然融合过渡
- **弹簧物理动画**（F405）— `animation_spring()`（弹簧力 + 阻尼衰减）、`animation_bounce()`（阻尼振荡解析公式）、`animation_gravity()`（重力落体弹跳）；四种高亮条模式可选：`0`=Ease / `1`=Spring / `2`=Bounce / `3`=Gravity
- **淡出动画**（F405）— 支持棋盘格分布淡出与整体逐行遮罩淡出两种模式（`FADE_MODE`）
- **列表弯曲效果**（`LIST_CUR`）— 选中行居中突出，非选中行弯曲淡出边缘
- **选中框超调**（`BOX_X_OS` / `BOX_Y_OS`）— 选中框短暂超出最终尺寸后弹回，增强动态感
- **弹窗展开动画**（`WIN_STYLE`）— 支持滑动与拉伸两种展开效果

### 页面系统

| 页面 | F103 | F405 | 说明 |
|:-----|:----:|:----:|:------|
| **Main** | 4 项 | 5 项 | 磁贴主菜单（F103: Sleep/Editor/Volt/Setting；F405: Sleep/Editor/Volt/Anima/Setting） |
| **Editor** | 12 项 | 12 项 | 功能编辑页（功能槽位 + 弹窗演示 + Knob 入口） |
| **Knob / KRF / KPF** | ✓ | ✓ | 旋钮常规设置（旋转功能、按键键值，82 项 USB HID 键码可选） |
| **Volt** | ✓ | ✓ | 电压测量与波形显示（10 个模拟通道：PA0-PA7, PB0-PB1） |
| **Setting** | 21 项 | 10 项 | 系统参数调节（对比度、动画速度、开关选项等） |
| **Anima** | ✗ | 21 项 | F405 独有动画参数调试页（弯曲、超调、弹簧、淡出模式等） |
| **About** | ✓ | ✓ | 版本与硬件信息（MCU、频率、RAM、Flash、作者），F103 从 Setting 进入，F405 同理 |
| **Window** | ✓ | ✓ | 模态弹窗系统，可叠加任意页面 |

### 弹窗系统

| 功能 | F103 | F405 |
|:-----|:----:|:----:|
| 值调节弹窗（参数调节，旋转确认） | ✓ | ✓ |
| 消息弹窗（多行文本） | ✗ | ✓ |
| 列表选择弹窗（预定义列表 + 异步回调） | ✗ | ✓ |
| 确认对话框（是/否 + 异步回调） | ✗ | ✓ |
| 可选背景模糊效果（`WIN_BOK`） | ✓ | ✓ |

### 列表系统

- 动态尺寸滚动（超出屏幕高度自动计算行数）
- **智能选中框** — 宽度自动适配选中文本长度
- **入场动画** — 选中框从列表顶部零宽度展开，返回上级平滑移动到上一级位置
- 控件类型（行首符号决定）：`~`数值 / `+`多选框 / `=`单选框 / `#`旋钮功能 / `$`按键键值 / `*`模式选择
- **参数映射数组**（F405）— 菜单项位置与参数索引可分离，灵活复用

### 外设对比

| 功能 | F103 版 | F405 版 |
|:-----|:--------|:--------|
| **屏幕** | OLED 128x128（模拟 I2C） | Sharp LS013B7DH03 Memory LCD（硬件 SPI3） |
| **旋钮** | EC11 编码器（轮询扫描） | SIQ-02FVS3 编码器（外部中断） |
| **蜂鸣器** | ✗ | ✓ TIM12_CH1 PWM，音量 0-4 级可调 |
| **RGB LED** | ✗ | ✓ 软件 PWM 呼吸灯（共阳极），GPIOA BSRR 驱动 |
| **USB HID** | ✗（框架预留） | ✓ 键盘 + 多媒体控制器（条件编译） |
| **USB MSC** | ✗ | ✓ Flash 仿真 64KB 虚拟磁盘（条件编译） |
| **EEPROM** | RAM 仿真（易失性） | Flash 仿真（磨损均衡，11 位检错 + 1 位容错） |

### 图标编辑器功能

- 像素级绘图工具（铅笔、直线、矩形、圆形、框选）
- PNG 导入/导出（缩放适配、居中裁剪、自定义缩放）
- C 代码实时预览与导出（`PROGMEM const uint8_t` 格式）
- 画布尺寸 8×8 ~ 256×256 可调，始终正方形
- 反色、旋转、镜像、圆角裁剪
- 多步撤销/重做、复制/粘贴
- 框选移动与旋转（Space 键旋转选区）

---

## 快速开始

### 环境要求

| 依赖 | 说明 |
|:-----|:------|
| Arduino IDE | 下载 [arduino.cc](https://www.arduino.cc/en/software) |
| Keil MDK（F103 可选） | Keil MDK-ARM v5+，用于 WouoUI_Port 工程 |
| STM32F1 支持包（F103） | Arduino Board Manager 安装 |
| STM32F4 支持包（F405） | Arduino Board Manager 安装 |
| U8g2 库 | Arduino Library Manager 安装 |
| USBComposite 库（F405 可选） | USB 功能需要 |
| Python 3.8+ | 图标编辑器运行环境 |
| Pillow 库（可选） | 图标编辑器 PNG 导入/导出 |

### F103 版

**方式一：Arduino IDE**

```bash
# 1. Arduino IDE 打开 WouoUI-128_128-F103/WouoUI-128_128-F103.ino
# 2. 安装 STM32F1 支持包（Board Manager）
# 3. 安装 U8g2 库（Library Manager）
# 4. 工具 → 开发板 → STM32F1xx → STM32F103C8
# 5. 连接 ST-Link 下载器，点击上传
```

**方式二：Keil MDK**

```bash
# 1. Keil MDK 打开 WouoUI_Port/USER/WouoUI.uvprojx
# 2. 选择 STM32F103C8 目标
# 3. 连接 ST-Link 下载器
# 4. 编译并下载
```

### F405 版

```bash
# 1. Arduino IDE 打开 WouoUI-128_128-F405.ino
# 2. 安装 STM32F4 支持包（Board Manager）
# 3. 安装 U8g2 库（Library Manager）
# 4. 如需 USB 功能：安装 USBComposite 库
# 5. 工具 → 开发板 → STM32F4xx → STM32F405/STM32F407
# 6. 连接 ST-Link，点击上传
```

> **F405 USB 功能启用**：`config.h` 设 `HID_ENABLE 1` / `USB_MSC_ENABLE 1`，取消 `WouoUI-128_128-F405.ino` 中 `hid_init()` 注释，重新烧录，BOOT0/BOOT1 置 0 后重新上电。

### 图标编辑器

```bash
# 运行（需要 Python 3.8+）
python icon_editor.py

# 如需 PNG 导入/导出
pip install Pillow
```

---

## 硬件连接

### F103 版（OLED + EC11 编码器）

| 外设 | 信号 | STM32 引脚 | 说明 |
|:-----|:-----|:-----------|:------|
| OLED | SCL | PB12 | 模拟 I2C 时钟 |
| | SDA | PB13 | 模拟 I2C 数据 |
| | RES | PB14 | OLED 复位 |
| 编码器 | AIO | PB12 | 编码器 A 相 |
| | BIO | PB13 | 编码器 B 相 |
| | SW | PB14 | 按键（内部上拉） |

> OLED 支持 SSD1306 / SH1106 / SSD1327 等 128x128 驱动 IC。注意 OLED 与编码器共用引脚，通过分时复用逻辑模拟 I2C。

### F405 版（Sharp Memory LCD + 编码器 + RGB LED + 蜂鸣器）

| 外设 | 信号 | STM32 引脚 | 说明 |
|:-----|:-----|:-----------|:------|
| Sharp LCD | SCL | PC10 | SPI3 时钟 |
| | SDA | PC12 | SPI3 MOSI |
| | DISP | PC5 | 显示使能 |
| | CS | PC4 | SPI 片选 |
| 编码器 | AIO | PC15 | A 相（外部中断） |
| | BIO | PC13 | B 相（方向判断） |
| | SW | PC14 | 按键（低有效） |
| 蜂鸣器 | BUZ | PB14 | TIM12_CH1 PWM |
| RGB LED | R | PA8 | 共阳极红色 |
| | G | PA9 | 共阳极绿色 |
| | B | PA10 | 共阳极蓝色 |
| USB | DP / DM | PA11 / PA12 | USB FS |

> Sharp LS013B7DH03 为反射式 Memory LCD，DC 引脚用作 DISP（显示使能），EXTMODE 接 GND（内部 VCOM），SPI 模式 0，最低 2MHz。

---

## 技术栈

| 类别 | 技术选型 |
|:-----|:---------|
| 编程语言 | **C / C++** |
| 目标 MCU | **STM32F103C8**（Cortex-M3, 72MHz）/ **STM32F405**（Cortex-M4, 168MHz） |
| 显示屏 | F103: OLED 128x128（SSD1306/SSD1327, 模拟 I2C） |
| | F405: Sharp LS013B7DH03 Memory LCD 128x128（硬件 SPI3） |
| 图形库 | **U8g2**（olikraus 单色图形库） |
| 输入设备 | **EC11 / SIQ-02FVS3** 旋转编码器 |
| 开发环境 | F103: **Arduino IDE / Keil MDK-ARM** · F405: **Arduino IDE** |
| USB（F405） | **USBComposite** 库 |
| 工具脚本 | **Python 3.8+**（tkinter + Pillow） |
| 许可证 | **Apache 2.0** |

---

## 许可说明

本项目合集使用 **Apache 2.0** 许可证。

> - 原版 WouoUI（v2.0）未设置开源协议
> - F103 移植版（v2.3）未单独设置协议
> - F405 移植增强版（v2.4）采用 Apache 2.0 协议发布
> - 图标编辑器同样采用 Apache 2.0 协议发布
>
> 如需商用或借鉴，请在醒目处标注本项目开源地址。

---

## 致谢

- **稚晖君** — MonoUI 设计理念
- **音游玩的人（RQNG）** — WouoUI 原版开发
- **olikraus** — U8g2 单色图形库

## 相关链接

- [WouoUI 原版（Bilibili）](https://www.bilibili.com/video/BV1Fi4y1L7er)
- [U8g2 图形库](https://github.com/olikraus/u8g2)
- [USBComposite 库](https://github.com/ARMinARM/USBComposite_stm32f1)
