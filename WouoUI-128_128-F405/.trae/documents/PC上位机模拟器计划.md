# PC 上位机模拟器计划

## 概述

为 WouoUI 嵌入式项目创建一个在 Windows 上运行的模拟器，使用 **Win32 API + GDI** 渲染 128×128 屏幕（**无 SDL2/U8g2 等外部依赖**），用键盘/鼠标模拟 EC11 编码器输入。模拟器**直接复用**项目中的 UI 逻辑代码（页面渲染、动画、弹窗等），仅替换硬件抽象层。

## 当前状态分析

### 已完成工作

`simulator/` 目录中已有完整的模拟器源码：

| 文件 | 作用 |
|------|------|
| `sim_main.cpp` | Win32 入口 + 窗口创建 + 消息循环 (WinMain/WndProc) |
| `sim_display.cpp` | **完整 SimU8g2 类**：自包含的像素级渲染实现（drawBox/drawStr/drawXBMP 等）+ 字体位图数据 + GDI 像素缓冲输出 |
| `sim_knob.cpp/.h` | 键盘/鼠标事件 → EC11 编码器模拟（上/下箭头 = 旋转，Enter/Space = 按键） |
| `sim_eeprom.cpp/.h` | 文件（`wouo_params.bin`）持久化模拟 EEPROM |
| `sim_stubs.cpp` | LED/USB/HID 空桩函数 + `millis()`/`delay()` 实现 |
| `config.h` | PC 端配置头文件：类型定义、布局常量、枚举、**SimU8g2 类声明**、Arduino 兼容宏 |
| `CMakeLists.txt` | CMake 构建配置（MSVC `/FI` 强制包含） |
| `build.bat` | 构建脚本 |

### 存在的问题

**核心问题：MSVC `/FI"config.h"` 文件名冲突**

CMakeLists.txt 使用 `/FI"${CMAKE_CURRENT_SOURCE_DIR}/config.h"` 强制优先加载模拟器的 `config.h`。但 MSVC 的 `#include "..."` 指令**优先搜索源文件所在目录**。当编译项目根目录中的源码（如 `../pages.cpp`）时：

1. `/FI"config.h"` 在源文件目录（项目根目录）中找到**原始** `config.h`，而不是模拟器的 `config.h`
2. 原始 `config.h` 包含 `#include <Arduino.h>` 和 `#include <U8g2lib.h>` → PC 上不存在这些 → 编译错误

**解决方案**：将模拟器配置头文件重命名为唯一名称（如 `sim_config.h`），避免与原始 `config.h` 发生文件名冲突。

### 依赖分析

所有复用的项目源码（`pages.cpp`、`ui_state.cpp`、`window.cpp`、`animation.cpp`、`menu_data.cpp`）的依赖链：

```
文件 → ui_types.h → config.h        ← 被 sim_config.h 用 #define CONFIG_H 阻断
文件 → display.h                     ← 被 sim_config.h 用 #define DISPLAY_H 阻断
文件 → knob.h                        ← OK，只声明 extern，函数实现由 sim_knob.cpp 提供
文件 → eeprom_manager.h              ← OK，只声明 extern，函数实现由 sim_eeprom.cpp 提供
文件 → hid_manager.h                 ← OK，stubs 由 sim_stubs.cpp 提供
文件 → usb_manager.h                 ← OK，#else 分支已有内联桩实现
文件 → led.h                         ← OK，stubs 由 sim_stubs.cpp 提供
文件 → math.h / string.h             ← OK，标准 C 库
```

所有硬件依赖均已妥善处理，**唯一的阻塞点是文件名冲突**。

### 技术选型

- **窗口/渲染**: Win32 API + GDI（仅链接 `user32.lib`、`gdi32.lib`，Windows 内置）
- **显示模拟**: 自包含 `SimU8g2` 类（完整实现 U8g2 API 子集），直接操作 2048 字节帧缓冲
- **字体**: 内嵌 `helvB24`（24px 大字体）和 `HelvetiPixel`（8px 小字体）的宽度表和位图数据
- **输入**: `WM_KEYDOWN`/`WM_KEYUP`/`WM_MOUSEWHEEL`/`WM_LBUTTON` 消息映射到 EC11 事件
- **构建**: CMake + Visual Studio 2022
- **外部依赖**: 零（仅 Windows SDK）

## 实施步骤

### 步骤 1：重命名配置头文件

- **操作**：删除 `simulator/config.h`，将内容写入 `simulator/sim_config.h`
- **原因**：唯一文件名避免 MSVC `#include "..."` 搜索顺序问题
- **内容**：保持现有 `simulator/config.h` 的全部内容不变，仅文件名变更

### 步骤 2：更新所有模拟器源文件的 `#include`

**修改以下文件中的 `#include "config.h"` 为 `#include "sim_config.h"`**：

| 文件 | 修改行 |
|------|--------|
| `simulator/sim_main.cpp` | `#include "config.h"` → `#include "sim_config.h"` |
| `simulator/sim_display.cpp` | `#include "config.h"` → `#include "sim_config.h"` |
| `simulator/sim_knob.h` | `#include "config.h"` → `#include "sim_config.h"` |
| `simulator/sim_knob.cpp` | (通过 `sim_knob.h` 间接引用，无需修改) |
| `simulator/sim_eeprom.cpp` | `#include "config.h"` → `#include "sim_config.h"` |
| `simulator/sim_stubs.cpp` | `#include "config.h"` → `#include "sim_config.h"` |

### 步骤 3：更新 CMakeLists.txt

- 修改 `force-include` 指令：`/FI"config.h"` → `/FI"sim_config.h"`
- 修改 `-include` 指令（非 MSVC）：`-include config.h` → `-include sim_config.h`
- 关闭 MSVC 警告 C4819（中文路径编码警告）：追加 `/wd"4819"` 编译选项

### 步骤 4：构建并修复

```bash
cd simulator
cmake -B build -S . -G "Visual Studio 17 2022"
cmake --build build --config Release
```

预期错误类型及处理：
1. **C4819 警告**（中文路径编码）→ 已在步骤 3 添加 `/wd"4819"`
2. **链接错误**（缺少符号）→ 补充 stubs
3. **类型不匹配** → 按编译器提示添加类型转换或调整声明

### 步骤 5：验证

运行 `simulator/build/Release/wouo_sim.exe`：
1. 窗口显示 128×128（缩放 4 倍=512×512）WouoUI 主界面
2. 上/下箭头键可切换磁贴选择
3. Enter/Space 进入子菜单
4. 鼠标滚轮模拟旋钮旋转
5. 鼠标左键模拟旋钮按键
6. 退出时参数保存到 `wouo_params.bin`

## 文件变更清单

| 操作 | 文件 |
|------|------|
| **删除** | `simulator/config.h` |
| **新建** | `simulator/sim_config.h`（内容同原 config.h） |
| **编辑** | `simulator/sim_main.cpp`（改 1 行 include） |
| **编辑** | `simulator/sim_display.cpp`（改 1 行 include） |
| **编辑** | `simulator/sim_knob.h`（改 1 行 include） |
| **编辑** | `simulator/sim_eeprom.cpp`（改 1 行 include） |
| **编辑** | `simulator/sim_stubs.cpp`（改 1 行 include） |
| **编辑** | `simulator/CMakeLists.txt`（改 3 行） |

## 验证步骤

1. 构建成功（无编译错误、无链接错误）
2. 模拟器窗口正常启动，显示 WouoUI 主菜单
3. 键盘方向键可操作菜单切换
4. Enter/Space 可进入子页面
5. 鼠标滚轮可旋转选择
6. 退出后参数持久化到文件
