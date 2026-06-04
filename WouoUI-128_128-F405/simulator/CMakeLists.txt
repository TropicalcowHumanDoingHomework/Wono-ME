cmake_minimum_required(VERSION 3.16)
project(WouoUISimulator LANGUAGES CXX)

# ============================================================
# 项目根目录
# ============================================================
set(PROJ_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/..")

# ============================================================
# 模拟器源文件
# ============================================================
set(SIM_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/sim_main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/sim_display.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/sim_knob.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/sim_eeprom.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/sim_stubs.cpp
)

# ============================================================
# 复用的项目源文件（完整 UI 逻辑，零修改）
# ============================================================
set(UI_SOURCES
    ${PROJ_ROOT}/ui_state.cpp
    ${PROJ_ROOT}/animation.cpp
    ${PROJ_ROOT}/pages.cpp
    ${PROJ_ROOT}/menu_data.cpp
    ${PROJ_ROOT}/window.cpp
)

# ============================================================
# 可执行文件
# ============================================================
add_executable(wouo_sim ${SIM_SOURCES} ${UI_SOURCES})

# ============================================================
# 包含路径
# simulator/ 目录优先（shadow 原始头文件）
# ============================================================
target_include_directories(wouo_sim PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}      # 模拟器的 config.h, display.h 等
    ${PROJ_ROOT}                     # 原始项目目录（ui_types.h, pages.h 等）
)

# ============================================================
# 全局定义 CONFIG_H 和 DISPLAY_H，阻止原始配置头文件（含 Arduino 依赖）被加载
# 同时强制包含模拟器配置头文件，提供 SimU8g2 类和枚举定义
# 使用裸文件名而非绝对路径，避免路径中的特殊字符（如 &）干扰 MSVC
# ============================================================
target_compile_definitions(wouo_sim PRIVATE CONFIG_H DISPLAY_H _CRT_SECURE_NO_WARNINGS)

if(MSVC)
    target_compile_options(wouo_sim PRIVATE
        /utf-8
        /FI"sim_define_config.h"
        /FI"sim_config.h"
        /FI"ui_types.h"
        /wd"4819"
    )
else()
    target_compile_options(wouo_sim PRIVATE
        -include sim_define_config.h
        -include sim_config.h
        -include ui_types.h
    )
endif()

# ============================================================
# 链接 Win32 库
# ============================================================
target_link_libraries(wouo_sim PRIVATE user32 gdi32)

# ============================================================
# 编译设置
# ============================================================
target_compile_features(wouo_sim PRIVATE cxx_std_11)

if(MSVC)
    target_compile_options(wouo_sim PRIVATE /W3 /wd"4100" /wd"4189")
else()
    target_compile_options(wouo_sim PRIVATE -Wall -Wextra -Wno-unused-parameter)
endif()

# 子系统设为 Windows（不显示控制台窗口）
set_target_properties(wouo_sim PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/build"
)
if(MSVC)
    set_target_properties(wouo_sim PROPERTIES
        LINK_FLAGS "/SUBSYSTEM:WINDOWS"
    )
endif()

# ============================================================
# 打印构建信息
# ============================================================
message(STATUS "WouoUI Simulator (Win32)")
message(STATUS "  Project root: ${PROJ_ROOT}")
message(STATUS "  Output dir:   ${CMAKE_CURRENT_SOURCE_DIR}/build")
message(STATUS "  Dependencies: user32, gdi32 (Windows built-in)")
