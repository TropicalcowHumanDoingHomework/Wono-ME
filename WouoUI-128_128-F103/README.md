<p align="center">
  <img src="https://img.shields.io/badge/MCU-STM32F103C8-blue?style=flat-square" alt="MCU"/>
  <img src="https://img.shields.io/badge/Screen-OLED%20128x128-0a0a0a?style=flat-square" alt="Screen"/>
  <img src="https://img.shields.io/badge/Resolution-128x128-ff69b4?style=flat-square" alt="Resolution"/>
  <img src="https://img.shields.io/badge/Language-C%2B%2B-00599C?style=flat-square" alt="Language"/>
  <img src="https://img.shields.io/badge/License-No%20License-lightgrey?style=flat-square" alt="License"/>
  <img src="https://img.shields.io/badge/Version-v2.3-orange?style=flat-square" alt="Version"/>
</p>

<h1 align="center">WouoUI v2.3 — STM32F103C8 移植版</h1>

<p align="center">
  基于 <strong>STM32F103C8</strong>（72MHz / 20KB SRAM / 64KB Flash）的嵌入式 UI 框架
  <br>
  驱动 <strong>OLED 128x128</strong>（I2C 接口），复现类 UltraLink 丝滑界面
</p>

<p align="center">
  此项目模仿自稚晖君未开源的 <strong>MonoUI</strong>，原版 WouoUI 由 <strong>音游玩的人</strong> 开发，
  <br>
  本仓库为其 <strong>STM32F103C8 移植版</strong>，由 <strong>罗米奇</strong> 维护。
</p>

---

## 目录

[硬件要求](#硬件要求) · [功能特性](#功能特性) · [页面说明](#页面说明) · [硬件引脚定义](#硬件引脚定义) · [项目结构](#项目结构) · [快速开始](#快速开始) · [配置说明](#配置说明) · [使用方法](#使用方法) · [常见问题](#常见问题) · [作者与致谢](#作者与致谢) · [版本历史](#版本历史) · [许可证](#许可证)

---

## 硬件要求

| 组件 | 型号 | 说明 |
|:-----|:-----|:------|
| **MCU** | STM32F103C8 / STM32F103xx | 72MHz, 20KB SRAM, 64KB Flash |
| **屏幕** | OLED 128x128（SSD1306 / SSD1327） | 模拟 I2C 接口 |
| **编码器** | EC11 | 带按键的旋转编码器 |

<details>
<summary><b>已测试的 MCU</b></summary>

- STM32F103C8T6（Blue Pill / 兼容板）
- STM32F103VET6（战舰板）
- 其他 STM32F103 系列（需确认 SRAM 容量）

</details>

---

## 功能特性

### UI 动画引擎

- **非线性平滑缓动** — 列表、弹窗、进度条全部使用缓动动画，摒弃突兀的跳变
- **双行算法** — 平滑动画核心压缩至两行代码，六类动画权重各自独立可调：
  - 公式：`a += (target - a) / (param / 10.0f)`，**参数值越大动画越慢**
  - 动画类型：`TILE_ANI`（磁贴）、`LIST_ANI`（列表）、`WIN_ANI`（弹窗）、`SPOT_ANI`（聚光）、`TAG_ANI`（标签）、`FADE_ANI`（消失）
- **可打断过渡** — 前一次动画未结束时触发新动画，新旧状态自然融合过渡，无跳帧感
- **消失动画** — 黑暗模式下渐变为全黑，白天模式下渐变为全白

### 主菜单 — 磁贴界面（Tile）

- 类似 UltraLink 的 Metro 风格磁贴布局
- 图标从中心展开动画，可选择 **从头展开** 或 **从上次位置展开**
- 磁贴支持循环滚动

### 列表菜单

- 列表可无限延长，自动计算行数
- **智能选择框** — 宽度根据选中字符串长度自动伸缩
- **入场动画** — 进入页面时选择框从列表顶部、宽度 0 展开，返回上级时平滑移动到上一级的选择位置
- 三种交互控件：
  - **单选框（Radio Button）** — 额外存储选中项在列表中的位置，展开时根据位置绘制选中圆点
  - **多选框（Check Box）** — 存储数组与列表行数一一对应，不要求连续排列
  - **数值显示** — 与多选框原理相似，仅用于展示，不涉及修改
- 列表展开动画起始位置可配置，支持循环模式

### 弹出窗口

- 独立窗口系统，调用接口简洁
- 支持自定义：标题、最大值、最小值、步进值、目标参数指针
- 窗口弹出/关闭带有缓动动画
- 可选的 **背景虚化** 效果（默认关闭，开启后处理器开销增大）

### 电压测量页面

- 原创波形显示界面
- 128 像素宽实时波形
- 可配置采样倍数（`WAVE_SAMPLE`）与波形幅值范围
- 仿真 ADC（软件生成正弦波数据，用于展示波形动画效果）

### 旋钮（编码器）控制

- 兼容 EC11 系列编码器
- **方向软件可调** — 无需交换硬件引脚
- 睡眠模式行为可配：
  - 旋转 → 发送 USB HID 音量调节 / 系统亮度调节 / 禁用
  - 短按 → 发送指定 USB HID 键盘键值 / 禁用
- 长按 → 唤醒并进入主菜单
- 旋转与按键的消抖时长可在设置菜单中独立调整

### 显示设置

- **显示对比度调节**（通过 `u8g2.setContrast()` 实现，`0`=淡 / `1`=浓）
- 黑暗模式 / 白天模式切换

### 数据存储

- **RAM EEPROM 模拟** — 在 RAM 中模拟 EEPROM 存储，仅修改参数后、进入睡眠模式时写入，断电丢失
- **容错校验** — 初始化时检查 11 个标志位，允许一位误码
- 保存完整参数集：显示对比度、6 类动画除数、按键阈值、旋钮配置、开关选项等

### 睡眠模式

- 无操作自动进入睡眠
- 旋钮旋转/短按可执行预设操作，长按唤醒

---

## 页面说明

```
M_SLEEP（睡眠）
└─ M_MAIN（主菜单 — 磁贴界面）
   ├─ M_EDITOR（参数编辑总页）
   │  └─ M_KNOB（旋钮设置）
   │     ├─ M_KRF（旋钮旋转功能设置）
   │     └─ M_KPF（旋钮按键功能设置）
   ├─ M_VOLT（电压测量）
   ├─ M_SETTING（系统设置）
   │  └─ M_ABOUT（关于本机）
   └─ M_WINDOW（弹窗，覆盖在任何页面上层）
```

| 页面 | 功能 |
|:-----|:-----|
| **主菜单** | 四个磁贴图标：Editor / Volt / Setting / About |
| **Editor** | 功能编辑总页（11 项：Function 0-9 + Knob） |
| **KNOB** | 旋钮通用设置（旋转功能选择、按键键值选择） |
| **KRF** | 旋转功能列表（禁用 / 音量 / 亮度） |
| **KPF** | 按键功能列表（82 项：字母 A-Z、数字 0-9、F1-F12、方向键、修饰键等） |
| **Volt** | 仿真电压测量（10 个模拟通道：PA0-PA7, PB0-PB1）与波形显示 |
| **Setting** | 16 项系统参数：显示对比度、动画速度、开关选项等 |
| **About** | 软件版本、MCU 型号、主频、RAM、Flash、作者信息 |

---

## 硬件引脚定义

### 屏幕 — OLED 128x128（模拟 I2C）

| 信号 | 功能 | STM32 引脚 |
|:-----|:-----|:-----------|
| SCL | I2C 时钟 | PA0 |
| SDA | I2C 数据 | PA1 |
| RES | OLED 复位 | PA2 |

> OLED 使用模拟 I2C 驱动，通过 GPIO 位带操作实现。I2C 设备地址为 0x78（7 位地址 0x3C）。支持 SSD1306 / SH1106 / SSD1327 等 128x128 OLED 驱动 IC。

### 旋钮

| 信号 | 功能 | STM32 引脚 |
|:-----|:-----|:-----------|
| AIO | 编码器 A 相 | PB12 |
| BIO | 编码器 B 相 | PB13 |
| SW | 编码器按键 | PB14（内部上拉） |

---

## 项目结构

```
WouoUI-128_128/
│
├── WouoUI-128_128.ino          # 主程序入口（setup + loop）
│
├── config.h                    # 全局配置、引脚定义、参数与页面枚举
├── ui_types.h                  # 数据结构定义（Menu, UiState, TileState 等）
├── ui_state.c / .h             # UI 全局状态变量与初始化
│
├── u8g2_adapter.c / .h         # U8g2 API 适配层（对接 OLED 驱动）
├── u8g2_global.cpp             # U8g2 全局 C++ 对象
├── display.h                   # 屏幕显存全局声明
│
├── animation.c / .h            # 动画引擎（缓动 animation + 消失 fade）
│
├── pages.c / .h                # 所有页面渲染与交互逻辑
├── menu_data.c / .h            # 菜单文本与磁贴图标数据
│
├── window.c / .h               # 弹窗系统
├── knob.c / .h                 # 旋钮轮询 + 按键扫描
│
├── eeprom_manager.c / .h       # RAM EEPROM 模拟
│
├── hw_abstraction.h            # 硬件抽象层（引脚宏定义）
│
├── oled.c / .h                 # OLED 屏幕驱动（模拟 I2C）
├── oledfont.h                  # OLED 字库
├── bmp.h                       # 位图数据
│
├── core_cm3.c / .h             # Cortex-M3 核心支持
├── startup_stm32f10x_md.s      # 启动文件（中容量）
├── startup_stm32f10x_hd.s      # 启动文件（大容量）
│
├── system_stm32f10x.c          # 系统时钟配置
├── stm32f10x_it.c / .h         # 中断服务函数
│
├── delay.c / .h                # 延时函数
├── sys.c / .h                  # 系统函数
│
├── stm32f10x_conf.h            # 外设库配置文件
│
├── LICENSE                     # No License
└── .gitignore
```

---

## 快速开始

### 环境搭建

1. 安装 [Arduino IDE](https://www.arduino.cc/en/software)
2. 安装 STM32F1 支持包：工具 → 开发板管理器 → 搜索 `STM32`
3. 安装所需库：
   - [U8g2](https://github.com/olikraus/u8g2) — OLED/LCD 绘图库
4. 准备烧录工具：ST-Link / USB 串口模块

### 编译烧录

```bash
git clone <repo-url>
# 用 Arduino IDE 打开 WouoUI-128_128.ino
# 选择开发板：STM32F103C8
# 连接烧录器，点击上传
```

> **提示**：首次使用建议保持默认配置，确认屏幕点亮、旋钮操作正常后再调整参数。

---

## 配置说明

所有配置统一集中在 `config.h`。

### 编译期常量

| 宏 | 说明 | 默认值 |
|:---|:-----|:-------|
| `UI_DEPTH` | 页面层级最大深度 | `20` |
| `UI_MNUMB` | 最大菜单项数量 | `100` |
| `UI_PARAM` | 系统可调参数数量 | `16` |
| `WAVE_SAMPLE` | 电压采样倍数 | `20` |
| `KNOB_PARAM` | 旋钮参数数量 | `4` |
| `EEPROM_CHECK` | EEPROM 校验标志位数量 | `11` |
| `BTN_PARAM_TIMES` | 按键参数放大倍数 | `2` |

### 运行时可调参数（设置菜单）

> 动画参数均为平滑公式除数（`param / 10.0f`），**值越大动画越慢**。
> 开关类参数默认 `0`=关闭，`1`=开启。

| 枚举 | 说明 | 实际范围 | 默认值 |
|:-----|:-----|:---------|:------|
| `DISP_BRI` | 显示对比度（非亮度） | `0` ~ `1` | `1` |
| `TILE_ANI` | 磁贴动画除数 | `10` ~ `100` | `30` |
| `LIST_ANI` | 列表动画除数 | `10` ~ `100` | `60` |
| `WIN_ANI` | 弹窗动画除数 | `10` ~ `100` | `25` |
| `SPOT_ANI` | 聚光动画除数 | `10` ~ `100` | `50` |
| `TAG_ANI` | 标签动画除数 | `10` ~ `100` | `60` |
| `FADE_ANI` | 消失步进间隔（ms） | `0` ~ `255` | `0` |
| `BTN_SPT` | 按键短按判定阈值（x2） | `0` ~ `255` | `25` |
| `BTN_LPT` | 按键长按判定阈值（x2） | `0` ~ `255` | `150` |
| `TILE_UFD` | 磁贴图标从头展开 | 开 / 关 | 开 |
| `LIST_UFD` | 列表从头展开 | 开 / 关 | 开 |
| `TILE_LOOP` | 磁贴循环模式 | 开 / 关 | 关 |
| `LIST_LOOP` | 列表循环模式 | 开 / 关 | 关 |
| `WIN_BOK` | 弹窗背景虚化 | 开 / 关 | 关 |
| `KNOB_DIR` | 旋钮方向反转 | 开 / 关 | 关 |
| `DARK_MODE` | 黑暗模式 | 开 / 关 | 开 |

### 旋钮专用参数（Editor 页面调节）

| 枚举 | 说明 | 可选值 |
|:-----|:-----|:-------|
| `KNOB_ROT` | 睡眠时旋转功能 | `0`=禁用 / `1`=音量 / `2`=亮度 |
| `KNOB_COD` | 睡眠时短按键值 | `0`=禁用 / 键码 |
| `KNOB_ROT_P` | 旋转功能单选框位置 | — |
| `KNOB_COD_P` | 键值单选框位置 | — |

---

## 使用方法

### 基本操作

| 操作 | 效果 |
|:-----|:-----|
| 旋转编码器 | 移动选择项 / 调节参数值 |
| 短按编码器 | 确认 / 进入子菜单 |
| 长按编码器 | 返回上一级 |
| 睡眠时旋转 | USB HID 音量 / 系统亮度（需自行实现 HID 驱动） |
| 睡眠时短按 | 发送键盘键值（需自行实现 HID 驱动） |
| 睡眠时长按 | 唤醒并进入主菜单 |

### 操作流程

```
上电 ──→ 睡眠 ─长按─→ 主菜单磁贴
                         │
                旋转切换磁贴，短按进入
                         │
       ┌──────────┬──────┼──────┬──────────┐
       ▼          ▼      ▼      ▼          ▼
    Editor     KNOB    Volt   Setting    About
       │          │             │
       ▼          ▼             ▼
     KRF/KPF    KPF/KRF      弹窗调参
```

1. **上电** → 进入睡眠
2. **长按唤醒** → 进入主菜单磁贴界面
3. **旋转切换磁贴** → **短按进入** 功能页面
4. **任意页面长按** → 返回上一级，直至回到主菜单
5. **主菜单长按** → 回到睡眠

### 弹出窗口操作

| 操作 | 效果 |
|:-----|:-----|
| 旋转编码器 | 调整数值 |
| 短按 | 确认并关闭 |
| 长按 | 取消并关闭 |

---

## 常见问题

<details>
<summary><b>上传后屏幕无显示？</b></summary>

检查 I2C 引脚连接是否正确（PA0-SCL, PA1-SDA, PA2-RES）。OLED 的 I2C 地址需为 0x3C。确认供电电压为 3.3V。
</details>

<details>
<summary><b>编译时提示内存不足？</b></summary>

STM32F103C8 仅有 20KB SRAM，128x128 帧缓冲区占用 2KB。可尝试优化：减少 `UI_DEPTH` / `UI_MNUMB` 等常量，或改用 STM32F103VE 等更大容量的型号。
</details>

<details>
<summary><b>编码器旋转方向与预期相反？</b></summary>

无需改动硬件。进入 Setting → 将"旋钮方向"开关打开即可反转。
</details>

<details>
<summary><b>电压测量页面无实际电压值？</b></summary>

此版本的 ADC 为软件仿真（`adc_stub_read`），生成正弦波用于演示波形动画效果。如需真实电压测量，需自行实现 STM32F103 的 ADC 外设驱动。
</details>

<details>
<summary><b>EEPROM 配置未保存？</b></summary>

本版本的 EEPROM 为 RAM 模拟，断电后数据会丢失。参数仅在进入睡眠模式时写入。如需持久化存储，需自行实现 STM32F103 内置 Flash 模拟 EEPROM。
</details>

<details>
<summary><b>页面切换时有闪烁？</b></summary>

关闭弹窗背景虚化（`WIN_BOK`）可提升刷新率。若仍闪烁，检查 I2C 时钟频率是否满足要求。
</details>

<details>
<summary><b>开启背景虚化后卡顿？</b></summary>

背景虚化需要全页重新绘制，对处理器开销较大。该功能默认关闭，按需开启。
</details>

---

## 作者与致谢

| 角色 | 作者 | 贡献 |
|:-----|:-----|:-----|
| **设计灵感** | [稚晖君](https://space.bilibili.com/9182439) | MonoUI 原版界面设计灵感 |
| **原版作者** | [音游玩的人](https://space.bilibili.com/9182439) | WouoUI v2.0 原版开发，[项目开源地址](https://github.com/Wonome/WouoUI-128_128) |
| **F103 移植** | [罗米奇](https://space.bilibili.com/549713590) | STM32F103C8 移植、RAM EEPROM 模拟、仿真 ADC、代码重构 |

### 参考项目

- [OpenT12](https://github.com/createskyblue/OpenT12) by createskyblue
- [OpenHeat](https://github.com/peng-zhihui/OpenHeat) by peng-zhihui
- [Wokwi 在线仿真](https://www.bilibili.com/video/BV1HA411S7pv/) by 路徍要什么自行车

---

## 版本历史

| 版本 | 日期 | 说明 |
|:-----|:-----|:-----|
| **v2.3** | — | F103 移植版：适配 STM32F103C8，优化动画引擎，完善磁贴界面与电压测量页面，新增弹窗系统、RAM EEPROM 模拟，仿真 ADC 波形展示 |
| **v2.0** | — | 重构动画引擎，新增磁贴界面、电压测量、弹窗系统、EEPROM 存储 |
| **v1.0** | — | 基础列表 UI 框架 |

---

## 许可证

本项目未设置开源协议。

> 原版 WouoUI（v2.0）未设置开源协议。F405 移植版（v2.3）采用 Apache 2.0 协议发布。
>
> 如需商用或借鉴，请在醒目处标注本项目开源地址。
