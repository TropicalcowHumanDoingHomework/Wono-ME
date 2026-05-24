<p align="center">
  <img src="https://img.shields.io/badge/MCU-STM32F405-blue?style=flat-square" alt="MCU"/>
  <img src="https://img.shields.io/badge/Screen-Sharp%20LS013B7DH03-0a0a0a?style=flat-square" alt="Screen"/>
  <img src="https://img.shields.io/badge/Resolution-128x128-ff69b4?style=flat-square" alt="Resolution"/>
  <img src="https://img.shields.io/badge/Language-C%2B%2B-00599C?style=flat-square" alt="Language"/>
  <img src="https://img.shields.io/badge/License-Apache%202.0-green?style=flat-square" alt="License"/>
  <img src="https://img.shields.io/badge/Version-v2.3-orange?style=flat-square" alt="Version"/>
</p>

<h1 align="center">WouoUI v2.3 — STM32F405 移植版</h1>

<p align="center">
  基于 STM32F103C8 原版移植至 <strong>STM32F405</strong>（168MHz / 192KB SRAM / 1024KB Flash）的嵌入式 UI 框架
  <br>
  驱动 <strong>Sharp LS013B7DH03</strong> 128x128 Memory LCD，复现类 UltraLink 丝滑界面
</p>

<p align="center">
  此项目模仿自稚晖君未开源的 <strong>MonoUI</strong>，原版 WouoUI 由 <strong>音游玩的人</strong> 开发，
  <br>
  本仓库为其 <strong>F405 硬件移植增强版</strong>，由 <strong>罗米奇</strong> 维护。
</p>

---

## 目录

[硬件要求](#硬件要求) &nbsp;&bull;&nbsp; [功能特性](#功能特性) &nbsp;&bull;&nbsp; [页面说明](#页面说明) &nbsp;&bull;&nbsp; [引脚定义](#硬件引脚定义) &nbsp;&bull;&nbsp; [项目结构](#项目结构) &nbsp;&bull;&nbsp; [快速开始](#快速开始) &nbsp;&bull;&nbsp; [配置说明](#配置说明) &nbsp;&bull;&nbsp; [使用方法](#使用方法) &nbsp;&bull;&nbsp; [常见问题](#常见问题) &nbsp;&bull;&nbsp; [已知限制](#已知限制) &nbsp;&bull;&nbsp; [作者](#作者与致谢) &nbsp;&bull;&nbsp; [贡献指南](#贡献指南) &nbsp;&bull;&nbsp; [版本历史](#版本历史) &nbsp;&bull;&nbsp; [许可证](#许可证)

---

## 硬件要求

| 组件 | 型号 | 说明 |
|:-----|:-----|:------|
| **MCU** | STM32F405 / STM32F407 / STM32F4xx | 168MHz, 192KB SRAM, 1024KB Flash |
| **屏幕** | Sharp LS013B7DH03 | 128x128 Memory LCD，SPI 接口 |
| **编码器** | SIQ-02FVS3（EC11 兼容） | 带按键的旋转编码器 |
| **LED** | RGB LED（共阳极） | 红/绿/蓝，软件 PWM 呼吸灯 |
| **蜂鸣器** | 无源蜂鸣器 | GPIO 方波驱动 |

<details>
<summary><b>已测试的 MCU</b></summary>

- STM32F405xx
- STM32F407xx
- STM32F103C8（原版）

> `config.h` 已内置自动 MCU 检测逻辑，支持 STM32F405/407/F4xx 及 ARM Cortex-M4 架构的自动识别，关于本机页面会自动显示对应硬件信息。

</details>

---

## 功能特性

### UI 动画引擎

- **非线性平滑缓动** — 列表、弹窗、进度条全部使用缓动动画，摒弃突兀的跳变
- **双行算法** — 平滑动画核心压缩至两行代码，六类动画权重各自独立可调：
  - 公式：`a += (target - a) / (param / 10.0f)`，**参数值越大动画越慢**
  - 动画类型：`TILE_ANI`（磁贴）、`LIST_ANI`（列表）、`WIN_ANI`（弹窗）、`SPOT_ANI`（聚光）、`TAG_ANI`（标签）、`FADE_ANI`（消失）
- **列表弯曲** — 新增列表行弯曲效果（`LIST_CUR`），选中行居中突显，非选中行向两侧弯曲淡出
- **选择框过伸** — 选择框入场时可在水平/竖直方向超出最终尺寸再回弹（`BOX_X_OS` / `BOX_Y_OS`）
- **弹窗拉伸** — 弹窗支持滑动和拉伸两种动画样式（`WIN_STYLE` 切换）
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

### 旋钮（编码器）控制

- 兼容 EC11 / SIQ-02FVS3 系列编码器
- **方向软件可调** — 无需交换硬件引脚
- 睡眠模式行为可配：
  - 旋转 → 发送 USB HID 音量调节 / 系统亮度调节 / 禁用
  - 短按 → 发送指定 USB HID 键盘键值 / 禁用
- 长按 → 唤醒并进入主菜单
- 旋转与按键的消抖时长可在设置菜单中独立调整

### RGB LED

- 软件 PWM 呼吸灯（8ms 亮度更新周期 + 255 级 PWM 细度）
- 共阳极 RGB LED 裸寄存器驱动（GPIOA BSRR 直接操作，不依赖 Arduino delay）
- 上电红色常亮，进入睡眠后白色呼吸灯

### 显示设置

- **显示对比度调节**（通过 `u8g2.setContrast()` 实现，`0`=淡 / `1`=浓）
- 屏幕旋转：0° / 90° / 180° / 270°
- 黑暗模式 / 白天模式切换

### USB 功能

| 功能 | 说明 | 默认 |
|:-----|:-----|:----:|
| **USB HID** | 模拟键盘 + 多媒体控制器，旋钮控制音量/亮度，按键发送键值 | 禁用 |
| **USB MSC** | 大容量存储，Flash 模拟 64KB U 盘，可在设置中开关 | 禁用 |

> **注意**：HID 和 MSC 功能默认禁用。如需启用，修改 `config.h` 中 `HID_ENABLE` / `USB_MSC_ENABLE` 为 `1`，重新编译烧录，并将 BOOT0/BOOT1 跳线置 0 后重新上电。

### 数据存储

- **EEPROM 磨损均衡** — 仅在参数修改后、进入睡眠模式时一次性写入，避免重复擦写
- **容错校验** — 初始化时检查 11 个标志位，允许一位误码
- 保存完整参数集：显示对比度、6 类动画除数、列表弯曲度、过伸值、按键阈值、旋钮配置、开关选项等

### 睡眠模式

- 无操作自动进入睡眠
- 睡眠中 RGB LED 切换为白色呼吸灯
- 旋钮旋转/短按可执行预设操作，长按唤醒

---

## 页面说明

```
M_SLEEP（睡眠）
└─ M_MAIN（主菜单 — 磁贴界面）
   ├─ M_EDITOR（参数编辑总页）
   │  ├─ M_WIN_LIST_DEMO（弹窗/列表展示页）
   │  └─ M_KNOB（旋钮设置）
   │     ├─ M_KRF（旋钮旋转功能设置）
   │     └─ M_KPF（旋钮按键功能设置）
   ├─ M_VOLT（电压测量）
   ├─ M_SETTING（系统设置）
   │  ├─ M_ANIMITION（动画调试调参）
   │  └─ M_ABOUT（关于本机）
   └─ M_WINDOW（弹窗，覆盖在任何页面上层）
```

| 页面 | 功能 |
|:-----|:-----|
| **主菜单** | 四个磁贴图标：Editor / Volt / Setting / About |
| **Editor** | 功能编辑总页（13 项：8 个功能槽位 + Win List Demo + Message Box + Knob） |
| **Win List Demo** | 弹窗与列表联动展示页，演示消息弹窗功能 |
| **KNOB** | 旋钮通用设置（旋转功能选择、按键键值选择） |
| **KRF** | 旋转功能列表（禁用 / 音量 / 亮度） |
| **KPF** | 按键功能列表（82 项：字母 A-Z、数字 0-9、F1-F12、方向键、修饰键等） |
| **Volt** | 实时电压测量（10 个模拟通道：PA0-PA7, PB0-PB1）与波形显示 |
| **Setting** | 26 项系统参数：显示对比度、动画速度、弯曲度、过伸值、开关选项等 |
| **Animition** | 动画调试调参页（10 项动画参数独立弹窗调节：Tile/List/Win/Spot/Tag/Fade Ani、List Cur、Box X/Y OS、Win Y OS） |
| **About** | MCU 型号、主频、RAM、Flash、作者信息 |

---

## 硬件引脚定义

### 屏幕 — Sharp LS013B7DH03（SPI3）

| 信号 | 功能 | STM32 引脚 |
|:-----|:-----|:-----------|
| SCL | SPI3 时钟 | PC10 |
| SDA | SPI3 MOSI | PC12 |
| DC | 显示使能 DISP | PC5 |
| CS | SPI 片选 | PC4 |
| RES | 复位 | 未使用（N/C） |

> LS013B7DH03 为 Memory LCD，与普通 OLED 不同：DC 引脚在此用作 DISP（显示使能）而非数据/命令选择。EXTMODE 接 GND 使用内部 VCOM。SPI 模式 0（CPOL=0, CPHA=0），最低时钟 2MHz。

### 旋钮与输出

| 信号 | 功能 | STM32 引脚 |
|:-----|:-----|:-----------|
| AIO | 编码器 A 相 | PC15 |
| BIO | 编码器 B 相 | PC13 |
| SW | 编码器按键 | PC14 |
| BUZ | 蜂鸣器 | PB14 |
| RGB_R | LED 红色通道 | PA8 |
| RGB_G | LED 绿色通道 | PA9 |
| RGB_B | LED 蓝色通道 | PA10 |

---

## 项目结构

```
WouoUI-128_128-F405/
│
├── WouoUI-128_128-F405.ino    # 主程序入口（setup + loop）
│
├── config.h                    # 全局配置、引脚定义、参数与页面枚举
├── ui_types.h                  # 数据结构定义（Menu, UiState, TileState 等）
├── ui_state.h / .cpp           # UI 全局状态变量与初始化
│
├── display.h / .cpp            # 屏幕驱动（U8g2 + SPI3 硬件回调）
├── animation.h / .cpp          # 动画引擎（缓动 animation + 消失 fade）
│
├── pages.h / .cpp              # 所有页面渲染与交互逻辑
├── menu_data.h / .cpp          # 菜单文本与磁贴图标数据
│
├── window.h / .cpp             # 弹窗系统
├── knob.h / .cpp               # 旋钮中断 + 按键扫描 + 蜂鸣器
│
├── eeprom_manager.h / .cpp     # EEPROM 读写与参数持久化
├── EEPROM.h                    # EEPROM 闪存模拟（磨损均衡）
│
├── led.h / .cpp                # RGB LED 裸寄存器驱动 + 软件 PWM 呼吸灯
├── hid_manager.h / .cpp        # USB HID 键盘/多媒体（条件编译）
├── usb_manager.h / .cpp        # USB MSC 大容量存储（条件编译）
│
├── LICENSE                     # Apache 2.0
└── .gitignore
```

---

## 快速开始

### 环境搭建

1. 安装 [Arduino IDE](https://www.arduino.cc/en/software)
2. 安装 STM32F4 支持包：工具 → 开发板管理器 → 搜索 `STM32`
3. 安装所需库：
   - [U8g2](https://github.com/olikraus/u8g2) — OLED/LCD 绘图库
   - [USBComposite](https://github.com/arpruss/USBComposite_stm32f1) — USB 功能（可选）
4. 准备烧录工具：ST-Link / USB 串口模块 / [STM32 Cube Programmer](https://www.st.com/en/development-tools/stm32cubeprog.html)

### 编译烧录

```bash
git clone <repo-url>
# 用 Arduino IDE 打开 WouoUI-128_128-F405.ino
# 选择开发板：STM32F405 或 STM32F407
# 连接烧录器，点击上传
```

> **提示**：首次使用建议保持默认配置，确认屏幕点亮、旋钮操作正常后再调整参数。

### 启用 USB 功能

```bash
# 1. config.h 中设置：
#    #define HID_ENABLE 1
#    #define USB_MSC_ENABLE 1
# 2. 重新编译烧录
# 3. 将 BOOT0 和 BOOT1 跳线均置为 0
# 4. 重新插拔 USB 线连接电脑
```

---

## 配置说明

所有配置统一集中在 `config.h`。

### 编译期常量

| 宏 | 说明 | 默认值 |
|:---|:-----|:-------|
| `UI_DEPTH` | 页面层级最大深度 | `20` |
| `UI_MNUMB` | 最大菜单项数量 | `100` |
| `UI_PARAM` | 系统可调参数数量 | `25` |
| `HID_ENABLE` | USB HID 功能开关 | `0`（禁用） |
| `USB_MSC_ENABLE` | USB 大容量存储开关 | `0`（禁用） |
| `SPI_BUS_CLOCK` | SPI 时钟频率 | `2000000`（2MHz） |
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
| `LIST_CUR` | 列表弯曲程度 | `0` ~ `200` | `0` |
| `BOX_X_OS` | 选择框水平过伸（px） | `0` ~ `40` | `10` |
| `BOX_Y_OS` | 选择框竖直过伸（px） | `0` ~ `40` | `10` |
| `WIN_Y_OS` | 弹窗竖直过伸（px） | `0` ~ `40` | `30` |
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
| `ROTATE_SCR` | 屏幕旋转 | 0° / 90° / 180° / 270° | 0° |
| `BUZ_VOL` | 蜂鸣器音量 | `0` ~ `4` | `2` |
| `USB_ENABLE` | USB 存储开关 | 开 / 关 | 关 |
| `WIN_STYLE` | 弹窗动画样式 | `0`=滑动 / `1`=拉伸 | `0` |
| `FADE_MODE` | 消失动画模式 | `0`=棋盘格渐变 / `1`=整体遮罩 | `0` |

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
| 睡眠时旋转 | USB HID 音量 / 系统亮度（需预设） |
| 睡眠时短按 | 发送键盘键值（需 HID） |
| 睡眠时长按 | 唤醒并进入主菜单 |

### 操作流程

```
上电 ──→ 睡眠（LED 红） ─长按─→ 主菜单磁贴
                                  │
                         旋转切换磁贴，短按进入
                                  │
            ┌──────────┬──────────┼──────────┬──────────┐
            ▼          ▼          ▼          ▼          ▼
         Editor     KNOB       Volt      Setting     About
            │          │                   │
            ▼          ▼                   ▼
          KRF/KPF    KPF/KRF          弹窗调参
```

1. **上电** → 进入睡眠，LED 红色常亮
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

检查 SPI 引脚连接是否正确。LS013B7DH03 的 DISP（DC）引脚需要外部提供方波以维持显示，确认 DISP 与 PC5 连通。EXTMODE 需接 GND 以使用内部 VCOM。
</details>

<details>
<summary><b>HID / MSC 功能无法使用？</b></summary>

两个功能默认禁用。需在 `config.h` 中将 `HID_ENABLE` 和/或 `USB_MSC_ENABLE` 设置为 `1`，重新编译烧录，并将 BOOT0/BOOT1 跳线均置 0 后重新上电。
</details>

<details>
<summary><b>编码器旋转方向与预期相反？</b></summary>

无需改动硬件。进入 Setting → 将"旋钮方向"开关打开即可反转。
</details>

<details>
<summary><b>页面切换时有闪烁？</b></summary>

关闭弹窗背景虚化（`WIN_BOK`）可提升刷新率。若仍闪烁，检查 SPI 时钟频率是否满足 2MHz 最低要求。
</details>

<details>
<summary><b>开启背景虚化后卡顿？</b></summary>

背景虚化需要全页重新绘制，对处理器开销较大。该功能默认关闭，按需开启。
</details>

<details>
<summary><b>EEPROM 配置未保存？</b></summary>

参数仅在进入睡眠模式时写入。修改参数后需让设备进入睡眠（主菜单长按返回）以触发保存。初始化时检查 11 个标志位，允许一位误码，若误码超过一位则恢复出厂设置。
</details>

---

## 已知限制

- **分辨率**：当前仅适配 **128×128** 分辨率。其他分辨率（128×64、128×32、通用版）请参考 WouoUI 原版多分辨率分支。
- **USB 功能**：HID 和 MSC 依赖 [USBComposite](https://github.com/arpruss/USBComposite_stm32f1) 库，启用前需确认该库与目标 MCU 兼容，并正确设置 BOOT0/BOOT1 跳线。
- **背景虚化**：开启 `WIN_BOK` 后弹窗背景虚化需要全页重绘，会显著增加 MCU 负载，仅在必要时开启。
- **EEPROM 与 MSC 共用 Flash**：EEPROM 模拟使用 Flash Sector 11（地址 `0x080C0000`），与 USB MSC 虚拟磁盘共享同一区域。同时启用 EEPROM 写入和 MSC 存储可能导致数据冲突，建议择一使用。
- **Sharp Memory LCD 的 DISP 引脚**：DISP 需要持续方波信号维持显示，断电或 DISP 悬空均会导致屏幕无显示。

---

## 作者与致谢

| 角色 | 作者 | 贡献 |
|:-----|:-----|:-----|
| **设计灵感** | [稚晖君](https://space.bilibili.com/9182439) | MonoUI 原版界面设计灵感 |
| **原版作者** | 音游玩的人 | WouoUI v2.0 原版开发，[项目开源地址](https://github.com/Wonome/WouoUI-128_128-F405) |
| **F405 移植** | [罗米奇](https://space.bilibili.com/549713590) | STM32F405 移植、USB MSC、RGB LED、蜂鸣器、自动 MCU 检测 |

### 参考项目

- [OpenT12](https://github.com/createskyblue/OpenT12) by createskyblue
- [OpenHeat](https://github.com/peng-zhihui/OpenHeat) by peng-zhihui
- [Wokwi 在线仿真](https://www.bilibili.com/video/BV1HA411S7pv/) by 路徍要什么自行车

---

## 贡献指南

欢迎提交 Issue 和 Pull Request。参与贡献前请注意：

- **代码风格**：保持与现有代码一致的 Arduino C++ 风格（4 空格缩进、中文注释、模块化 .h/.cpp 分离）。
- **编译兼容**：提交前请确保在 STM32F405 / F407 目标下 **零编译警告**。
- **config.h 同步**：新增配置项需同步更新 `config.h` 中的注释和默认值。
- **文档同步**：新增或修改功能时，请同步更新本 README 中的对应章节（参数表格、页面层级图等）。

> 如有疑问，请先发起 Issue 讨论后再提交代码修改。

---

## 版本历史

| 版本 | 日期 | 说明 |
|:-----|:-----|:-----|
| **v2.3** | 2025-05 | F405 移植版：支持 STM32F4xx 自动检测、USB MSC 大容量存储、RGB LED 呼吸灯、蜂鸣器、Apache 2.0 许可；新增 `LIST_CUR` 列表弯曲、`BOX_X_OS`/`BOX_Y_OS` 选择框过伸、`WIN_Y_OS` 弹窗过伸、`WIN_STYLE` 弹窗拉伸样式、`FADE_MODE` 消失动画模式、`M_WIN_LIST_DEMO` 演示页面；`UI_PARAM` 扩展至 25 项 |
| **v2.0** | 2024-06 | 重构动画引擎，新增磁贴界面、电压测量、弹窗系统、EEPROM 存储 |
| **v1.0** | 2024-01 | 基础列表 UI 框架 |

---

## 许可证

本项目使用 **Apache 2.0** 许可证。

> 原版 WouoUI（v2.0）未设置开源协议。F405 移植版（v2.3）采用 Apache 2.0 协议发布。
>
> 如需商用或借鉴，请在醒目处标注本项目开源地址。