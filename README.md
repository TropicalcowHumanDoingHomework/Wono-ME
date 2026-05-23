<p align="center">
  <img src="https://img.shields.io/badge/MCU-STM32F103C8-blue?style=flat-square" alt="F103"/>
  <img src="https://img.shields.io/badge/MCU-STM32F405-00B4AB?style=flat-square" alt="F405"/>
  <img src="https://img.shields.io/badge/Tool-Python-3776AB?style=flat-square&logo=python" alt="Python"/>
  <img src="https://img.shields.io/badge/License-Apache%202.0-green?style=flat-square" alt="License"/>
  <img src="https://img.shields.io/badge/Version-v2.3-orange?style=flat-square" alt="Version"/>
</p>

<h1 align="center">Wonome — WouoUI 项目合集</h1>

<p align="center">
  WouoUI 嵌入式 UI 框架的多平台移植版与配套工具合集
  <br>
  基于稚晖君 <strong>MonoUI</strong> 设计理念，由 <strong>音游玩的人</strong> 原版开发，
  <br>
  <strong>STM32F103C8</strong> 与 <strong>STM32F405</strong> 移植版由 <strong>罗米奇</strong> 移植维护
</p>

<p align="center">
  <a href="#项目列表"><strong>项目列表</strong></a> ·
  <a href="#项目结构"><strong>项目结构</strong></a> ·
  <a href="#快速开始"><strong>快速开始</strong></a> ·
  <a href="#许可证"><strong>许可证</strong></a>
</p>

---

## 项目列表

| 项目 | 平台 | 说明 |
|:-----|:-----|:------|
| [**WouoUI-128_128**](WouoUI-128_128/) | STM32F103C8 | F103 移植版，驱动 OLED 128x128（I2C），72MHz / 20KB SRAM / 64KB Flash |
| [**WouoUI-128_128-F405**](WouoUI-128_128-F405/) | STM32F405 | F405 移植增强版，驱动 Sharp LS013B7DH03 Memory LCD，168MHz / 192KB SRAM / 1024KB Flash |
| [**图标编辑器**](icon_editor.py) | Python 3.8+ | 配套像素图标编辑工具，支持 PNG 导入/导出、C 代码生成、可变画布 |

### WouoUI-128_128（F103 移植版）

基于 **STM32F103C8**（72MHz / 20KB SRAM / 64KB Flash）的嵌入式 UI 框架，驱动 **OLED 128x128**（I2C 接口），复现类 UltraLink 丝滑界面。

> 详细文档见 [WouoUI-128_128/README.md](WouoUI-128_128/README.md)

### WouoUI-128_128-F405（F405 移植增强版）

基于 **STM32F405**（168MHz / 192KB SRAM / 1024KB Flash）的嵌入式 UI 框架，驱动 **Sharp LS013B7DH03** 128x128 Memory LCD，在原版基础上增加了 USB MSC 大容量存储、RGB LED 呼吸灯、蜂鸣器等功能。

> 详细文档见 [WouoUI-128_128-F405/README.md](WouoUI-128_128-F405/README.md)

### 图标编辑器

WouoUI 配套的像素图标编辑工具（Python），支持可变尺寸画布（8×8 ~ 256×256）、PNG 导入/导出、C 代码实时预览与导出，直接生成 `PROGMEM const uint8_t` 格式的字节数组，即编即用。

```bash
python icon_editor.py
```

---

## 项目结构

```
Wonome/
│
├── WouoUI-128_128/             # STM32F103C8 移植版
│   ├── WouoUI-128_128.ino      # 主程序入口
│   ├── config.h                # 全局配置与引脚定义
│   ├── ui_types.h              # 数据结构定义
│   ├── pages.cpp / .h          # 页面渲染与交互逻辑
│   ├── menu_data.cpp / .h      # 菜单文本与图标数据
│   ├── window.cpp / .h         # 弹窗系统
│   ├── knob.cpp / .h           # 旋钮与按键扫描
│   ├── animation.cpp / .h      # 动画引擎
│   ├── eeprom_manager.cpp / .h # EEPROM 持久化
│   ├── hid_manager.cpp / .h    # USB HID
│   ├── README.md               # 详细文档
│   └── ...
│
├── WouoUI-128_128-F405/        # STM32F405 移植增强版
│   ├── WouoUI-128_128-F405.ino # 主程序入口
│   ├── config.h                # 全局配置与引脚定义
│   ├── ui_types.h              # 数据结构定义
│   ├── pages.cpp / .h          # 页面渲染与交互逻辑
│   ├── menu_data.cpp / .h      # 菜单文本与图标数据
│   ├── window.cpp / .h         # 弹窗系统
│   ├── knob.cpp / .h           # 旋钮与按键扫描
│   ├── animation.cpp / .h      # 动画引擎
│   ├── eeprom_manager.cpp / .h # EEPROM 持久化
│   ├── hid_manager.cpp / .h    # USB HID
│   ├── led.cpp / .h            # RGB LED 呼吸灯
│   ├── usb_manager.cpp / .h    # USB MSC 大容量存储
│   ├── README.md               # 详细文档
│   └── ...
│
├── icon_editor.py              # 图标编辑器（Python 工具）
├── README.md                   # 本说明文档
└── ...
```

---

## 快速开始

### F103 版

```bash
# 用 Arduino IDE 打开 WouoUI-128_128/WouoUI-128_128.ino
# 安装 STM32F1 支持包
# 安装 U8g2 库
# 选择开发板：STM32F103C8
# 连接 ST-Link，点击上传
```

### F405 版

```bash
# 用 Arduino IDE 打开 WouoUI-128_128-F405/WouoUI-128_128-F405.ino
# 安装 STM32F4 支持包
# 安装 U8g2 库
# 选择开发板：STM32F405/STM32F407
# 连接 ST-Link，点击上传
```

### 图标编辑器

```bash
# 确保 Python 3.8+
python icon_editor.py
# 如需 PNG 导入/导出功能：
pip install Pillow
```

---

## 功能特性

### UI 动画引擎

- **非线性平滑缓动** — 列表、弹窗、进度条全部使用缓动动画
- **双行算法** — 平滑动画核心压缩至两行代码，六类动画权重各自独立可调
- **可打断过渡** — 前一次动画未结束时触发新动画，新旧状态自然融合过渡

### 页面系统

| 页面 | 说明 |
|:-----|:------|
| **Main** | 磁贴主菜单（4 项：Sleep / Editor / Volt / Setting） |
| **Editor** | 功能编辑页 |
| **KNOB / KRF / KPF** | 旋钮设置（旋转功能 / 按键键值，82 项可选） |
| **Volt** | 电压测量与波形显示（10 个模拟通道） |
| **Setting** | 系统参数调节（动画速度、开关选项等） |
| **About** | 版本与硬件信息 |

### 图标编辑器功能

- 像素级绘图（铅笔、直线、矩形、圆形、框选）
- PNG 导入/导出（缩放适配 / 居中裁剪 / 自定义缩放）
- C 代码实时预览与导出
- 反色、旋转、镜像、圆角裁剪
- 多步撤销/重做、复制/粘贴
- 框选移动与旋转

---

## 许可证

本项目使用 **Apache 2.0** 许可证。

> 原版 WouoUI（v2.0）未设置开源协议。F405 移植版（v2.3）采用 Apache 2.0 协议发布。
>
> 图标编辑器同样采用 Apache 2.0 协议发布。
>
> 如需商用或借鉴，请在醒目处标注本项目开源地址。
