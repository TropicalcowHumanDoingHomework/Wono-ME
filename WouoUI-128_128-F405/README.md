<p align="center">
  <img src="https://img.shields.io/badge/MCU-STM32F405-blue?style=flat-square" alt="MCU"/>
  <img src="https://img.shields.io/badge/Screen-Sharp%20LS013B7DH03-0a0a0a?style=flat-square" alt="Screen"/>
  <img src="https://img.shields.io/badge/Resolution-128x128-ff69b4?style=flat-square" alt="Resolution"/>
  <img src="https://img.shields.io/badge/Language-C%2B%2B-00599C?style=flat-square" alt="Language"/>
  <img src="https://img.shields.io/badge/License-Apache%202.0-green?style=flat-square" alt="License"/>
  <img src="https://img.shields.io/badge/Version-v2.4-orange?style=flat-square" alt="Version"/>
</p>

<h1 align="center">WouoUI v2.4 — STM32F405 移植版</h1>

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

[硬件要求](#硬件要求) &bull; [功能特性](#功能特性) &bull; [页面说明](#页面说明) &bull; [引脚定义](#硬件引脚定义) &bull; [项目结构](#项目结构) &bull; [快速开始](#快速开始) &bull; [配置说明](#配置说明) &bull; [使用方法](#使用方法) &bull; [常见问题](#常见问题) &bull; [作者](#作者与致谢) &bull; [版本历史](#版本历史) &bull; [许可证](#许可证)

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
- **弹簧物理动画** — 新增基于弹簧-阻尼模型的物理动画系统：
  - `animation_spring()` — 弹簧力驱动 + 阻尼衰减，产生真实弹性的过冲-回弹效果
  - `animation_bounce()` — 阻尼振荡解析公式，专为弹跳效果优化
  - `animation_gravity()` — 重力落体物理模拟，刚体在地面一侧反复弹跳直至静止
- **高亮条动画模式（`HL_ANI_MODE`）** — 四种模式切换：
  - `0` = Ease（传统缓动，平滑无过冲）
  - `1` = Spring（弹簧物理，带弹性过冲回弹）
  - `2` = Bounce（阻尼振荡，多次来回衰减）
  - `3` = Gravity（重力弹跳，仅在地面一侧弹跳）
- **弹簧参数** — `SPRING_K`（刚度/100）和 `SPRING_D`（阻尼/100）独立可调
- **列表弯曲（`LIST_CUR`）** — 选中行居中突显，非选中行向两侧弯曲淡出
- **选择框过伸（`BOX_X_OS` / `BOX_Y_OS`）** — 选择框入场时可在水平/竖直方向超出最终尺寸再回弹
- **弹窗拉伸（`WIN_STYLE`）** — 弹窗支持滑动和拉伸两种动画样式
- **可打断过渡** — 前一次动画未结束时触发新动画，新旧状态自然融合过渡
- **消失动画** — 两种模式（`FADE_MODE`）：`0`=棋盘格分布淡出 / `1`=整体逐行遮罩淡出；黑暗模式变黑，白天模式变白

### 主菜单 — 磁贴界面（Tile）

- 类似 UltraLink 的 Metro 风格磁贴布局（5 个磁贴：Sleep / Editor / Volt / Anima / Setting）
- 图标从中心展开动画，可选择从头展开或从上次位置展开
- 磁贴支持循环滚动

### 列表菜单

- 列表可无限延长，自动计算行数
- **智能选择框** — 宽度根据选中字符串长度自动伸缩
- **入场动画** — 进入页面时选择框从列表顶部、宽度 0 展开，返回上级时平滑移动到上一级的选择位置
- **参数映射数组** — 支持 `uint8_t* map` 间接索引，菜单项位置与参数索引可分离，灵活复用
- 三种交互控件：
  - **单选框（=）** — 存储选中项的值和位置
  - **多选框（+）** — 存储数组与列表行数对应
  - **数值显示（~）** — 显示参数值
  - **旋钮功能（#/$）** — 显示音量/亮度/按键功能
  - **模式选择（*）** — 显示 Ease/Spring/Bounce/Gravity 等文字标签
- 列表展开动画起始位置可配置，支持循环模式

### 弹出窗口

- **值调节弹窗（`window_value_init`）** — 参数值窗口，旋转调节，短按确认
- **消息弹窗（`window_message_init`）** — 多行文本消息窗口
- **列表选择弹窗（`window_list_select_init`）** — 从预定义列表中选择一项，支持回调
- **确认弹窗（`window_confirm_init`）** — 是/否确认对话框，异步回调通知结果
- 支持自定义最大值、最小值、步进值
- 窗口弹出/关闭带有缓动或弹簧动画
- 可选的背景虚化效果（默认关闭）

### 电压测量页面

- 原创波形显示界面，128 像素宽实时波形
- 10 个模拟通道选择（PA0-PA7, PB0-PB1）
- 可配置采样倍数（`WAVE_SAMPLE`）与波形幅值范围
- **注意**：Volt 页面短按/长按均返回主菜单，不进入子层级

### 旋钮（编码器）控制

- 兼容 EC11 / SIQ-02FVS3 系列编码器
- **方向软件可调** — 无需交换硬件引脚
- 睡眠模式行为可配：旋转 → USB HID 音量/系统亮度/禁用；短按 → 发送 HID 键值
- 长按唤醒并进入主菜单
- 旋转与按键的消抖时长可在设置中独立调整

### RGB LED

- 软件 PWM 呼吸灯（8ms 亮度更新周期 + 255 级 PWM 细度）
- 共阳极 RGB LED 裸寄存器驱动（GPIOA BSRR 直接操作）
- 上电红色常亮 → 进入功能页红色 → 睡眠白色呼吸灯

### 显示设置

- **显示对比度调节**（`u8g2.setContrast()`，0=淡 / 1=浓）
- 屏幕旋转：0° / 90° / 180° / 270°
- 黑暗模式 / 白天模式切换

### USB 功能

| 功能 | 说明 | 默认 |
|:-----|:-----|:----:|
| **USB HID** | 模拟键盘 + 多媒体控制器，旋钮控制音量/亮度，按键发送键值 | 禁用 |
| **USB MSC** | 大容量存储，Flash 模拟 64KB U 盘 | 禁用 |

> 启用：`config.h` 中设 `HID_ENABLE 1` / `USB_MSC_ENABLE 1`，重新编译烧录，BOOT0/BOOT1 置 0 后重新上电。
> **HID 额外步骤**：启用后还需在 `WouoUI-128_128-F405.ino` 的 `setup()` 中取消 `//hid_init();` 的注释。

### 数据存储

- **EEPROM 磨损均衡** — 仅在参数修改后、进入睡眠模式时一次性写入
- **容错校验** — 初始化时检查 11 个标志位，允许一位误码，超过则恢复出厂设置

### 睡眠模式

- 从主菜单选择 Sleep 磁贴手动进入睡眠
- LED 切换为白色呼吸灯；保存 EEPROM 时闪白
- 旋钮旋转/短按可执行预设 HID 操作，长按唤醒

---

## 页面说明

```
M_SLEEP（睡眠）
└─ M_MAIN（主菜单 — 磁贴界面）
   ├─ M_EDITOR（功能编辑总页）
   │  └─ M_KNOB（旋钮设置）
   │     ├─ M_KRF（旋钮旋转功能设置）
   │     └─ M_KPF（旋钮按键功能设置）
   ├─ M_VOLT（电压测量）
   ├─ M_ANIMITION（动画参数调节）
   ├─ M_SETTING（系统设置）
   │  └─ M_ABOUT（关于本机）
   └─ M_WINDOW（弹窗，覆盖在任何页面上层）
```

| 页面 | 菜单项数 | 功能 |
|:-----|:--------|:-----|
| **主菜单** | 5 | 磁贴：Sleep / Editor / Volt / Anima / Setting |
| **Editor** | 12 | 功能槽位 0-6、确认弹窗演示、列表选择演示、消息弹窗、Knob 入口 |
| **KNOB** | 3 | 旋钮通用设置 |
| **KRF** | 7 | 旋转功能选择（禁用 / 音量 / 亮度） |
| **KPF** | 82 | 按键键值选择（A-Z, 0-9, F1-F12, 方向键, 修饰键等） |
| **Volt** | 10 | 实时电压测量（PA0-PA7, PB0-PB1）+ 波形显示 |
| **Anima** | 21 | 动画参数调节：速度、弯曲、过伸、虚化、弹簧刚度和阻尼、高亮条模式等 |
| **Setting** | 10 | 基础系统设置：对比度、黑暗模式、屏幕旋转、蜂鸣器音量、按键时长等 |
| **About** | 8 | MCU 型号、主频、RAM、Flash、作者信息 |

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

> Memory LCD：DC 引脚用作 DISP（显示使能）而非数据/命令选择。EXTMODE 接 GND 使用内部 VCOM。SPI 模式 0，最低时钟 2MHz。

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
├── ui_types.h                  # 数据结构定义
├── ui_state.h / .cpp           # UI 全局状态变量与初始化
│
├── display.h / .cpp            # 屏幕驱动（U8g2 + SPI3 硬件回调）
├── animation.h / .cpp          # 动画引擎（缓动 + 弹簧 + 弹跳 + 消失）
│
├── pages.h / .cpp              # 所有页面渲染与交互逻辑
├── menu_data.h / .cpp          # 菜单文本、图标数据、弹窗测试数据
│
├── window.h / .cpp             # 弹窗系统（值/消息/列表选择/确认）
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

> 首次使用建议保持默认配置，确认屏幕点亮、旋钮操作正常后再调整参数。

### 启用 USB 功能

```bash
# 1. config.h 中设置：
#    #define HID_ENABLE 1
#    #define USB_MSC_ENABLE 1
# 2. 启用 HID 还需在 WouoUI-128_128-F405.ino 的 setup() 中取消 //hid_init(); 的注释
# 3. 重新编译烧录
# 4. 将 BOOT0 和 BOOT1 跳线均置为 0
# 5. 重新插拔 USB 线连接电脑
```

---

## 配置说明

所有配置统一集中在 `config.h`。

### 编译期常量

| 宏 | 说明 | 默认值 |
|:---|:-----|:-------|
| `UI_DEPTH` | 页面层级最大深度 | `20` |
| `UI_MNUMB` | 最大菜单项数量 | `100` |
| `UI_PARAM` | 系统可调参数数量 | `28` |
| `HID_ENABLE` | USB HID 功能开关 | `0`（禁用） |
| `USB_MSC_ENABLE` | USB 大容量存储开关 | `0`（禁用） |
| `SPI_BUS_CLOCK` | SPI 时钟频率 | `2000000`（2MHz） |
| `WAVE_SAMPLE` | 电压采样倍数 | `20` |
| `KNOB_PARAM` | 旋钮参数数量 | `4` |
| `EEPROM_CHECK` | EEPROM 校验标志位数量 | `11` |
| `BTN_PARAM_TIMES` | 按键参数放大倍数 | `2` |
| `WIN_LIST_MAX` | 列表选择弹窗最大选项数 | `20` |
| `WIN_LIST_ITEM_LEN` | 列表弹窗选项文字最大长度 | `24` |

### 运行时可调参数

> 动画参数（TILE_ANI / LIST_ANI / WIN_ANI / SPOT_ANI / TAG_ANI）均为平滑公式除数（`param / 10.0f`），**值越大动画越慢**。
> 弹簧参数（SPRING_K / SPRING_D）实际值为 `param / 100`。
> 开关类参数默认 `0`=关闭，`1`=开启。

#### Setting 页面（基础设置）

| 枚举 | 说明 | 范围 | 默认 |
|:-----|:-----|:----|:----|
| `DISP_BRI` | 显示对比度 | `0` ~ `1` | `1` |
| `DARK_MODE` | 黑暗模式 | 开/关 | 开 |
| `ROTATE_SCR` | 屏幕旋转 | 0° / 90° / 180° / 270° | 0° |
| `BUZ_VOL` | 蜂鸣器音量 | `0` ~ `4` | `2` |
| `BTN_SPT` | 短按阈值（x2） | `0` ~ `255` | `25` |
| `BTN_LPT` | 长按阈值（x2） | `0` ~ `255` | `150` |
| `KNOB_DIR` | 旋钮方向反转 | 开/关 | 关 |
| `USB_ENABLE` | USB 存储开关 | 开/关 | 关 |

#### Anima 页面（动画参数）

| 枚举 | 说明 | 范围 | 默认 |
|:-----|:-----|:----|:----|
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
| `FADE_MODE` | 消失动画模式 | `0`=棋盘格 / `1`=整体遮罩 | `0` |
| `TILE_UFD` | 磁贴从头展开 | 开/关 | 开 |
| `LIST_UFD` | 列表从头展开 | 开/关 | 开 |
| `TILE_LOOP` | 磁贴循环模式 | 开/关 | 关 |
| `LIST_LOOP` | 列表循环模式 | 开/关 | 关 |
| `WIN_BOK` | 弹窗背景虚化 | 开/关 | 关 |
| `WIN_STYLE` | 弹窗动画样式 | `0`=滑动 / `1`=拉伸 | `0` |
| `HL_ANI_MODE` | 高亮条动画模式 | `0`=Ease / `1`=Spring / `2`=Bounce / `3`=Gravity | `0` |
| `SPRING_K` | 弹簧刚度（/100） | `10` ~ `100` | `25` |
| `SPRING_D` | 弹簧阻尼（/100） | `10` ~ `100` | `70` |

### 旋钮专用参数

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
上电 → 主菜单磁贴（5项）─ Sleep 磁贴 ─→ 睡眠（LED 白呼吸）
              │
     旋转切换磁贴，短按进入
              │
   ┌────┬────┼────┬────┐
   ▼    ▼    ▼    ▼    ▼
 Editor Volt Anima Setting About
   │                │
   ▼                ▼
KNOB/KRF/KPF    弹窗调参

睡眠 ─长按─→ 主菜单
```

1. **上电** → 进入主菜单磁贴界面，LED 红色常亮，蜂鸣器响
2. **旋转切换磁贴** → **短按进入** 功能页面
3. **任意页面长按** → 返回上一级，直至回到主菜单
4. **主菜单选择 Sleep 磁贴** → 进入睡眠，LED 白色呼吸灯，EEPROM 自动保存
5. **睡眠时长按** → 回到主菜单

### Editor 特殊功能

| 菜单项 | 操作 |
|:-------|:-----|
| Function 0-6 | 功能槽位（预留，可扩展） |
| Conf Test | 弹出确认对话框（Yes/No），异步回调 |
| List Win Sel | 弹出列表选择弹窗，从 8 个预置项中选择 |
| Message Box | 弹出多行消息弹窗（Hello World） |
| Knob | 进入旋钮设置页 |

---

## 常见问题

<details>
<summary><b>上传后屏幕无显示？</b></summary>

检查 SPI 引脚连接。LS013B7DH03 的 DISP（DC）引脚需方波维持显示，EXTMODE 需接 GND。
</details>

<details>
<summary><b>HID / MSC 功能无法使用？</b></summary>

默认禁用。`config.h` 中设 `HID_ENABLE 1` / `USB_MSC_ENABLE 1`，重新编译烧录，BOOT0/BOOT1 置 0 后重新上电。

> **注意**：启用 HID 后还需在 `WouoUI-128_128-F405.ino` 的 `setup()` 函数中取消 `//hid_init();` 的注释，HID 键盘/多媒体功能才能正常初始化。
</details>

<details>
<summary><b>编码器方向反了？</b></summary>

Setting → 打开"旋钮方向"开关即可反转，无需改硬件。
</details>

<details>
<summary><b>页面切换闪烁/卡顿？</b></summary>

关闭弹窗背景虚化（WIN_BOK）；检查 SPI 时钟是否满足 2MHz 最低要求。弹簧动画（Spring/Bounce/Gravity）比 Ease 模式计算量大，可切回 Ease 模式。
</details>

<details>
<summary><b>EEPROM 配置未保存？</b></summary>

参数仅在进入睡眠模式时写入。修改参数后需让设备进入睡眠以触发保存。11 位校验，允许一位误码，超过则恢复出厂。
</details>

<details>
<summary><b>弹簧动画不弹或抖动？</b></summary>

调节 Anima 页面中的 `Spring K`（刚度）和 `Spring D`（阻尼）。刚度越大回弹越猛，阻尼越大衰减越快。建议从 K=25, D=70 开始微调。
</details>

---

## 作者与致谢

| 角色 | 作者 | 贡献 |
|:-----|:-----|:-----|
| **设计灵感** | [稚晖君](https://space.bilibili.com/1227930) | MonoUI 原版界面设计灵感 |
| **原版作者** | [音游玩的人（RQNG）](https://space.bilibili.com/9182439) | WouoUI v2.0 原版开发 |
| **F405 移植** | [罗米奇](https://space.bilibili.com/549713590) | STM32F405 移植、USB MSC、RGB LED、蜂鸣器、弹簧动画、列表选择弹窗、确认弹窗、参数映射、v2.4 全部增强功能 |

### 参考项目

- [OpenT12](https://github.com/createskyblue/OpenT12) by createskyblue
- [OpenHeat](https://github.com/peng-zhihui/OpenHeat) by peng-zhihui
- [Wokwi 在线仿真](https://www.bilibili.com/video/BV1HA411S7pv/) by 路徍要什么自行车

---

## 版本历史

| 版本 | 说明 |
|:-----|:------|
| **v2.4** | 引入弹簧物理动画系统（Spring / Bounce / Gravity）；`HL_ANI_MODE` 四种高亮条模式（新增 Gravity 重力弹跳）；`FADE_MODE` 两种消失模式；列表选择弹窗 + 确认弹窗（异步回调）；参数映射数组 `map` 实现灵活索引；`M_ANIMITION` 独立动画调节页面；Setting 重组精简为 10 项；新增 `SPRING_K` / `SPRING_D` / `FADE_MODE` / `HL_ANI_MODE` 参数；`UI_PARAM` 扩展至 28 |
| **v2.3** | F405 移植版：STM32F4xx 自动检测、USB MSC、RGB LED 呼吸灯、蜂鸣器、Apache 2.0 许可；新增 `LIST_CUR` / `BOX_X_OS` / `BOX_Y_OS` / `WIN_Y_OS` / `WIN_STYLE` |
| **v2.0** | 重构动画引擎，新增磁贴界面、电压测量、弹窗系统、EEPROM 存储 |
| **v1.0** | 基础列表 UI 框架 |

---

## 许可证

本项目使用 **Apache 2.0** 许可证。

> 原版 WouoUI（v2.0）未设置开源协议。F405 移植版（v2.3+）采用 Apache 2.0 协议发布。
> 如需商用或借鉴，请在醒目处标注本项目开源地址。