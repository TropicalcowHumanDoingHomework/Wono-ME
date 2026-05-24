<p align="center">
  <img src="https://img.shields.io/badge/MCU-STM32F103C8-blue?style=flat-square" alt="F103"/>
  <img src="https://img.shields.io/badge/MCU-STM32F405-00B4AB?style=flat-square" alt="F405"/>
  <img src="https://img.shields.io/badge/Tool-Python-3776AB?style=flat-square&logo=python" alt="Python"/>
  <img src="https://img.shields.io/badge/License-Apache%202.0-green?style=flat-square" alt="License"/>
  <img src="https://img.shields.io/badge/Version-v2.3-orange?style=flat-square" alt="Version"/>
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

- **WouoUI 原版**由 B 站 UP 主 [音游玩的人](https://space.bilibili.com/322152275) 开发，灵感来源于稚晖君的 MonoUI 设计理念
- **F103 移植版**基于 STM32F103C8（72MHz / 20KB SRAM / 64KB Flash），驱动 OLED 128x128（I2C）
- **F405 移植增强版**基于 STM32F405（168MHz / 192KB SRAM / 1024KB Flash），驱动 Sharp Memory LCD 128x128（SPI），增加了 USB MSC 大容量存储、RGB LED 呼吸灯、蜂鸣器等功能
- **图标编辑器**为配套的像素图标绘制工具（Python），支持 PNG 导入/导出、C 代码生成

---

## 项目列表

| 项目 | 平台 | 说明 |
|:-----|:-----|:------|
| [**WouoUI-128_128**](WouoUI-128_128/) | STM32F103C8 | F103 移植版，驱动 OLED 128x128（I2C），72MHz / 20KB SRAM / 64KB Flash |
| [**WouoUI-128_128-F405**](WouoUI-128_128-F405/) | STM32F405 | F405 移植增强版，驱动 Sharp LS013B7DH03 Memory LCD，168MHz / 192KB SRAM / 1024KB Flash |
| [**图标编辑器**](icon_editor.py) | Python 3.8+ | 配套像素图标编辑工具，支持 PNG 导入/导出、C 代码生成、可变画布 |

### WouoUI-128_128（F103 移植版）

基于 **STM32F103C8** 的嵌入式 UI 框架，驱动 **OLED 128x128**（I2C 接口），完整复现 WouoUI 的动画页面系统。

> 详细文档见 [WouoUI-128_128/README.md](WouoUI-128_128/README.md)

### WouoUI-128_128-F405（F405 移植增强版）

基于 **STM32F405** 的增强版嵌入式 UI 框架，驱动 **Sharp LS013B7DH03** 128x128 Memory LCD（SPI 接口），在原版基础上增加了多项外设功能。

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
├── WouoUI-128_128/             # STM32F103C8 移植版
│   ├── WouoUI-128_128.ino      # 主程序入口（Arduino Sketch）
│   ├── config.h                # 全局配置与引脚定义
│   ├── ui_types.h              # 数据结构定义
│   ├── ui_state.cpp / .h       # UI 全局状态
│   ├── display.cpp / .h        # OLED 显示驱动（U8g2）
│   ├── pages.cpp / .h          # 页面渲染与交互逻辑
│   ├── menu_data.cpp / .h      # 菜单文本与图标数据
│   ├── window.cpp / .h         # 弹窗系统
│   ├── knob.cpp / .h           # 旋钮与按键扫描
│   ├── animation.cpp / .h      # 动画引擎
│   ├── eeprom_manager.cpp / .h # EEPROM 持久化（RAM 仿真）
│   ├── hid_manager.cpp / .h    # USB HID
│   ├── README.md               # 详细文档
│   │
│   └── WouoUI_Port/            # 原版 WouoUI 移植层
│       ├── USER/               # STM32 启动文件、系统配置
│       ├── CORE/               # Cortex-M3 核心支持
│       ├── SYSTEM/             # 延时与系统函数
│       ├── STM32F10x_FWLib/    # STM32F10x 标准外设库
│       └── HARDWARE/
│           ├── OLED/           # OLED 驱动、字体、位图
│           └── WouoUI/         # WouoUI 框架核心
│
├── WouoUI-128_128-F405/        # STM32F405 移植增强版
│   ├── WouoUI-128_128-F405.ino # 主程序入口（Arduino Sketch）
│   ├── config.h                # 全局配置与引脚定义（自动 MCU 检测）
│   ├── ui_types.h              # 数据结构定义
│   ├── ui_state.cpp / .h       # UI 全局状态
│   ├── display.cpp / .h        # Sharp Memory LCD 驱动（SPI）
│   ├── pages.cpp / .h          # 页面渲染与交互逻辑
│   ├── menu_data.cpp / .h      # 菜单文本与图标数据
│   ├── window.cpp / .h         # 弹窗系统（增强版）
│   ├── knob.cpp / .h           # 旋钮与按键扫描（中断驱动 + 蜂鸣器）
│   ├── animation.cpp / .h      # 动画引擎（含弹簧物理动画）
│   ├── eeprom_manager.cpp / .h # Flash 仿真 EEPROM（磨损均衡）
│   ├── EEPROM.h                # Flash EEPROM 仿真实现
│   ├── led.cpp / .h            # RGB LED 软件 PWM 呼吸灯
│   ├── hid_manager.cpp / .h    # USB HID 键盘/多媒体
│   ├── usb_manager.cpp / .h    # USB MSC 大容量存储（64KB 虚拟磁盘）
│   ├── README.md               # 详细文档
│   ├── LICENSE                 # Apache 2.0 许可证
│   └── build/                  # 编译输出
│
├── icon_editor.py              # 图标编辑器（Python 工具）
├── README.md                   # 本说明文档
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
- **弹簧物理动画**（F405 版）— `animation_spring()` 基于刚度/阻尼模型的弹簧物理模拟
- **淡出动画** — 棋盘格像素级逐点淡出（支持深色/浅色/全屏遮罩三种模式）
- **列表弯曲效果** — 选中行居中突出，非选中行弯曲淡出边缘
- **选中框超调** — 选中框短暂超出最终尺寸后弹回，增强动态感
- **弹窗展开动画** — 支持滑动与拉伸两种展开效果

### 页面系统

| 页面 | 说明 |
|:-----|:------|
| **Main** | 磁贴主菜单（4 项：Editor / Volt / Setting / About） |
| **Editor** | 功能编辑页（8 个功能槽位 + 演示项） |
| **Knob** | 旋钮常规设置（旋转功能、按键键值，82 项 USB HID 键码可选） |
| **KRF / KPF** | 旋转功能选择 / 按键功能选择 |
| **Volt** | 电压测量与波形显示（10 个模拟通道实时采集） |
| **Setting** | 系统参数调节（25 项：对比度、动画速度、开关选项等） |
| **Animation** | 动画参数调试页（10 项动画参数实时调节） |
| **About** | 版本与硬件信息（MCU、频率、RAM、Flash、作者） |
| **Window** | 模态弹窗系统（参数调节、消息框、列表选择，可叠加任意页面） |

### 弹窗系统

- 独立参数调节窗口（标题、最小值/最大值/步进、目标指针）
- 动画开/关缓动效果
- 可选背景模糊效果（`WIN_BOK`，计算密集）
- 消息框模式与列表选择模式

### 列表系统

- 动态尺寸滚动（超出屏幕高度自动滚动）
- **智能选中框** — 宽度自动适配选中文本长度
- **入场动画** — 选中框从列表顶部零宽度展开
- 三种控件类型：单选按钮、复选框、数值显示

### 旋钮输入

- 兼容 **EC11** / **SIQ-02FVS3** 旋转编码器
- **软件方向可调**（无需交换硬件引脚）
- 休眠模式行为可配置：旋转 → USB HID 音量/亮度，短按 → 发送键码，长按 → 唤醒
- 消抖时间独立可调

### 外设功能（F405 增强版）

| 功能 | 说明 |
|:-----|:------|
| **RGB LED** | 软件 PWM 呼吸灯（8ms 更新周期，255 级 PWM），上电红色，休眠白色呼吸 |
| **蜂鸣器** | 无源蜂鸣器 GPIO 方波驱动，开机/确认/退出音效反馈，音量 0-4 级可调 |
| **USB HID** | 模拟键盘 + 多媒体控制器，旋钮控制音量/亮度 |
| **USB MSC** | 大容量存储，Flash 仿真 64KB USB 虚拟磁盘 |

### 数据持久化

- **F103 版**：RAM 仿真 EEPROM（易失性，断电丢失）
- **F405 版**：Flash 仿真 EEPROM，带磨损均衡，仅休眠时写入，11 位检错 + 1 位容错

### 图标编辑器功能

- 像素级绘图工具（铅笔、直线、矩形、圆形、框选）
- PNG 导入/导出（缩放适配、居中裁剪、自定义缩放）
- C 代码实时预览与导出（`PROGMEM const uint8_t` 格式）
- 画布尺寸 8×8 ~ 256×256 可调
- 反色、旋转、镜像、圆角裁剪
- 多步撤销/重做、复制/粘贴
- 框选移动与旋转

---

## 快速开始

### 环境要求

| 依赖 | 说明 |
|:-----|:------|
| Arduino IDE | 下载 [arduino.cc](https://www.arduino.cc/en/software) |
| STM32F1 支持包 | 用于 F103 版编译（Arduino Board Manager） |
| STM32F4 支持包 | 用于 F405 版编译（Arduino Board Manager） |
| U8g2 库 | 显示驱动库（Arduino Library Manager） |
| USBComposite 库（可选） | 仅 F405 版 USB 功能需要 |
| Python 3.8+ | 图标编辑器运行环境 |
| Pillow 库（可选） | 图标编辑器的 PNG 导入/导出功能需要 |

### F103 版

```bash
# 1. 用 Arduino IDE 打开 WouoUI-128_128/WouoUI-128_128.ino
# 2. 安装 STM32F1 支持包（Arduino Board Manager）
# 3. 安装 U8g2 库（Arduino Library Manager）
# 4. 工具 → 开发板 → STM32F1xx → STM32F103C8
# 5. 连接 ST-Link 下载器
# 6. 点击上传
```

### F405 版

```bash
# 1. 用 Arduino IDE 打开 WouoUI-128_128-F405/WouoUI-128_128-F405.ino
# 2. 安装 STM32F4 支持包（Arduino Board Manager）
# 3. 安装 U8g2 库（Arduino Library Manager）
# 4. 如需 USB 功能：安装 USBComposite 库
# 5. 工具 → 开发板 → STM32F4xx → STM32F405/STM32F407
# 6. 连接 ST-Link 下载器
# 7. 点击上传
```

### 图标编辑器

```bash
# 运行图标编辑器（需要 Python 3.8+）
python icon_editor.py

# 如需 PNG 导入/导出功能，安装 Pillow 库
pip install Pillow
```

---

## 硬件连接

### F103 版引脚定义

| 外设 | 引脚 | 说明 |
|:-----|:-----|:------|
| OLED I2C | PB6 (SCL) / PB7 (SDA) | I2C1 接口 |
| 旋转编码器 A | PA6 | 外部中断 |
| 旋转编码器 B | PA7 | 外部中断 |
| 按键 | PA2 | 编码器内置按钮，低电平触发 |
| ST-Link | SWDIO / SWCLK | 下载调试 |

> 详细引脚配置见各项目的 `config.h` 文件。

### F405 版引脚定义

| 外设 | 引脚 | 说明 |
|:-----|:-----|:------|
| Sharp LCD SPI | PB3 (SCK) / PB5 (MOSI) | SPI3 接口 |
| LCD 控制 | PC6 (CS) / PC7 (EXTCOM) / PD13 (DISP) | 片选/刷新/显示开关 |
| 旋转编码器 A | PE0 | 外部中断 |
| 旋转编码器 B | PE1 | 外部中断 |
| 按键 | PE2 | 编码器内置按钮 |
| RGB LED | PA0 (R) / PA1 (G) / PA2 (B) | 共阳 RGB LED，低电平亮 |
| 蜂鸣器 | PE6 | 无源蜂鸣器，PWM 方波驱动 |
| USB | PA11 (DP) / PA12 (DM) | USB FS 接口 |

> 注意：F405 版包含自动 MCU 检测（`STM32F405xx` / `STM32F407xx`），可在 `config.h` 中按需调整引脚分配。

---

## 技术栈

| 类别 | 技术选型 |
|:-----|:---------|
| 编程语言 | **C++**（Arduino 框架） |
| 目标 MCU | **STM32F103C8**（Cortex-M3, 72MHz） / **STM32F405**（Cortex-M4, 168MHz） |
| 显示屏 | F103: OLED 128x128（SSD1306/SSD1327, I2C） |
| | F405: Sharp LS013B7DH03 Memory LCD 128x128（SPI） |
| 图形库 | **U8g2**（olikraus 单色图形库） |
| 输入设备 | **EC11 / SIQ-02FVS3** 旋转编码器 |
| USB（F405） | **USBComposite** 库（HID + MSC） |
| 工具 | **Python 3.8+**（tkinter + Pillow） |
| IDE | **Arduino IDE**（STM32 核心支持包） |
| 许可证 | **Apache 2.0** |

---

## 许可说明

本项目使用 **Apache 2.0** 许可证。

> - 原版 WouoUI（v2.0）未设置开源协议
> - F103 移植版沿用原版，未单独设置协议
> - F405 移植增强版（v2.3）采用 Apache 2.0 协议发布
> - 图标编辑器同样采用 Apache 2.0 协议发布
>
> 如需商用或借鉴，请在醒目处标注本项目开源地址。

---

## 致谢

- **稚晖君** — MonoUI 设计理念
- **音游玩的人** — WouoUI 原版开发
- **olikraus** — U8g2 单色图形库

## 相关链接

- [WouoUI 原版视频介绍（Bilibili）](https://www.bilibili.com/video/BV1Fi4y1L7er)
- [U8g2 图形库](https://github.com/olikraus/u8g2)
- [USBComposite 库](https://github.com/ARMinARM/USBComposite_stm32f1)
