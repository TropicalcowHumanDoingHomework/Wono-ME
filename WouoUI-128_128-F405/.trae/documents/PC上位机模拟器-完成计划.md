# PC上位机模拟器 — 完成计划

## 当前状态

模拟器主体代码已基本完成，目录结构如下：

| 文件 | 状态 | 说明 |
|------|------|------|
| `simulator/sim_main.cpp` | ✅ 完成 | Win32窗口+消息循环+UI调度 |
| `simulator/sim_display.cpp` | ✅ 完成 | SimU8g2类实现(2048字节缓冲、像素/线条/盒/字体渲染) |
| `simulator/sim_config.h` | ✅ 完成 | 模拟器配置+SimU8g2类声明(不含C++标准库) |
| `simulator/sim_knob.cpp` | ✅ 完成 | 键盘→编码器事件映射 |
| `simulator/sim_knob.h` | ✅ 完成 | 模拟器旋钮头文件 |
| `simulator/sim_eeprom.cpp` | ✅ 完成 | 文件型EEPORM(wouo_params.bin) |
| `simulator/sim_eeprom.h` | ✅ 完成 | EEPORM头文件 |
| `simulator/sim_stubs.cpp` | ✅ 完成 | Arduino/LED/HID桩函数 |
| `simulator/config.h` | ✅ 完成 | 重定向到sim_config.h |
| `simulator/sim_define_config.h` | ✅ 完成 | 预定义CONFIG_H/DISPLAY_H |
| `simulator/ui_types.h` | ✅ 完成 | 模拟器副本(带CONFIG_H守卫) |
| `simulator/hid_manager.h` | ✅ 完成 | 模拟器副本(移除config.h包含) |
| `simulator/CMakeLists.txt` | ✅ 完成 | CMake构建配置(MSVC/Ninja) |
| `simulator/build.bat` | ✅ 完成 | 一键构建脚本 |
| `simulator/build/` | ⚠️ 已配置 | CMake已生成VS 2022项目 |

## 关键问题

### 问题：uint8_t等类型未定义

**分析**：
- CMake通过 `/FI` 全局强制包含 `sim_config.h`(提供SimU8g2类)和 `simulator/ui_types.h`(提供结构体)
- 通过 `/D CONFIG_H` 阻止原始 `config.h`(含 `<Arduino.h>`)被加载
- 但原始的头文件(`pages.h`, `window.h`, `knob.h`等)在函数声明和结构体定义中大量使用 `uint8_t`、`uint16_t` 等类型
- 这些类型通常由 `<Arduino.h>` → `<stdint.h>` 提供，但 `config.h` 被阻断后无人提供
- `sim_config.h` 明确注释说不能包含C++标准库(担心Windows SDK冲突)

**根本原因**：原始项目头文件中的类型依赖链被切断，需要在不引入Windows SDK冲突的前提下提供 `uint8_t` 等类型。

## 修改方案

### 1. 修复类型缺失

**文件**: `simulator/sim_config.h`
**操作**: 添加 `#include <stdint.h>` (C标准库头文件，不涉及C++ namespace，安全无冲突)
**理由**: `<stdint.h>` 是纯C头文件，在所有Windows SDK版本中都可安全包含，不会与 `<windows.h>` 冲突。

### 2. 构建

**命令**: `cd simulator && cmake --build build --config Release`
**说明**: CMake已配置好VS 2022项目，直接构建即可。路径中的 `&` 字符不影响MSVC编译。

### 3. 迭代修复

根据构建错误逐个修复，预期可能的错误：
- 类型不匹配(如 `int` vs `uint16_t`)
- 缺少桩函数
- 头文件包含路径问题

### 4. 验证

运行生成的 `wouo_sim.exe`，验证：
- 窗口显示(4倍缩放，512×512)
- UI磁贴页面正确渲染
- 键盘方向键=旋钮旋转
- Enter/Space=按键
- EEPROM文件自动创建

## 无需修改的文件

以下原有UI源文件**零修改**复用：
- `ui_state.cpp` — UI状态和参数初始化
- `animation.cpp` — 四种动画算法
- `pages.cpp` — 所有页面渲染
- `menu_data.cpp` — 菜单文本和图标数据
- `window.cpp` — 弹窗系统

## 时间线预估

1. 修复类型 + 构建：约5分钟
2. 迭代修复错误：约10-30分钟
3. 运行验证：约5分钟
