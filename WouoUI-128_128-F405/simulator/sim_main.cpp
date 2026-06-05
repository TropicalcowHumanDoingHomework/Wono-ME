#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "sim_config.h"
#include "ui_types.h"
#include "ui_state.h"
#include "display.h"
#include "animation.h"
#include "menu_data.h"
#include "pages.h"
#include "eeprom_manager.h"
#include "window.h"
#include "sim_knob.h"

#include <cstdio>
#include <cstring>

#define WINDOW_SCALE 4
#define DISP_W_PX (DISP_W * WINDOW_SCALE)
#define DISP_H_PX (DISP_H * WINDOW_SCALE)
#define TARGET_FPS 60
#define FRAME_MS (1000 / TARGET_FPS)

bool sim_running = true;

static HWND g_hwnd = nullptr;
static HBITMAP g_bitmap = nullptr;
static uint32_t* g_pixel_buf = nullptr;
static uint32_t g_timer_id = 0;

static void update_pixel_buffer() {
    uint8_t* buf = u8g2.getBufferPtr();
    int dark = ui.param[DARK_MODE];
    int rot = u8g2.getRotation();

    for (int y = 0; y < DISP_H; ++y) {
        for (int x = 0; x < DISP_W; ++x) {
            int sx, sy;
            switch (rot) {
                case 0: sx = x;               sy = y;               break;
                case 1: sx = y;               sy = DISP_W - 1 - x;  break;
                case 2: sx = DISP_W - 1 - x;  sy = DISP_H - 1 - y;  break;
                case 3: sx = DISP_H - 1 - y;  sy = x;               break;
                default: sx = x; sy = y; break;
            }
            int byte_idx = sy * 16 + (sx >> 3);
            uint8_t bit = (buf[byte_idx] >> (7 - (sx & 7))) & 1;
            if (dark) {
                g_pixel_buf[y * DISP_W + x] = bit ? 0x00FFFFFF : 0x00000000;
            } else {
                g_pixel_buf[y * DISP_W + x] = bit ? 0x00000000 : 0x00FFFFFF;
            }
        }
    }
}

static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE:
            return 0;

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            if (g_pixel_buf && g_bitmap) {
                SetBitmapBits(g_bitmap, DISP_W * DISP_H * 4, g_pixel_buf);
                HDC mem_dc = CreateCompatibleDC(hdc);
                HGDIOBJ old = SelectObject(mem_dc, g_bitmap);
                SetStretchBltMode(hdc, HALFTONE);
                StretchBlt(hdc, 0, 0, DISP_W_PX, DISP_H_PX,
                           mem_dc, 0, 0, DISP_W, DISP_H, SRCCOPY);
                SelectObject(mem_dc, old);
                DeleteDC(mem_dc);
            }
            EndPaint(hwnd, &ps);
            return 0;
        }

        case WM_KEYDOWN: {
            UINT vk = (UINT)wParam;
            if (vk == VK_ESCAPE) {
                PostQuitMessage(0);
                sim_running = false;
                return 0;
            }
            switch (vk) {
                case VK_UP:
                case 0x57: sim_knob_handle_key(81, true); break;
                case VK_DOWN:
                case 0x53: sim_knob_handle_key(82, true); break;
                case VK_RETURN:
                case VK_SPACE: sim_knob_handle_key(40, true); break;
            }
            return 0;
        }

        case WM_KEYUP: {
            UINT vk = (UINT)wParam;
            switch (vk) {
                case VK_RETURN:
                case VK_SPACE: sim_knob_handle_key(40, false); break;
            }
            return 0;
        }

        case WM_MOUSEWHEEL: {
            short delta = GET_WHEEL_DELTA_WPARAM(wParam);
            if (delta > 0) {
                sim_knob_handle_key(81, true);
                sim_knob_handle_key(81, false);
            } else {
                sim_knob_handle_key(82, true);
                sim_knob_handle_key(82, false);
            }
            return 0;
        }

        case WM_LBUTTONDOWN:
            sim_knob_handle_key(40, true);
            return 0;

        case WM_LBUTTONUP:
            sim_knob_handle_key(40, false);
            return 0;

        case WM_TIMER:
            if (sim_running) {
                btn_scan();
                ui_proc();
                update_pixel_buffer();
                InvalidateRect(hwnd, nullptr, FALSE);
            }
            return 0;

        case WM_DESTROY:
            sim_running = false;
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
    BITMAPINFO bmi = {0};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = DISP_W;
    bmi.bmiHeader.biHeight = -DISP_H;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    HDC screen_dc = GetDC(nullptr);
    g_bitmap = CreateDIBSection(screen_dc, &bmi, DIB_RGB_COLORS,
                                (void**)&g_pixel_buf, nullptr, 0);
    ReleaseDC(nullptr, screen_dc);

    const char CLASS_NAME[] = "WouoUISim";
    WNDCLASS wc = {0};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.lpszClassName = CLASS_NAME;

    if (!RegisterClass(&wc)) return 1;

    RECT rect = {0, 0, DISP_W_PX, DISP_H_PX};
    AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);
    int win_w = rect.right - rect.left;
    int win_h = rect.bottom - rect.top;

    g_hwnd = CreateWindowEx(0, CLASS_NAME, "WouoUI Simulator",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, win_w, win_h,
        nullptr, nullptr, hInstance, nullptr);
    if (!g_hwnd) return 1;

    ShowWindow(g_hwnd, nCmdShow);
    UpdateWindow(g_hwnd);

    eeprom_init();
    ui_init();
    tile_param_init();
    setting_param_init();
    lcd_init();
    btn_init();

    update_pixel_buffer();
    InvalidateRect(g_hwnd, nullptr, FALSE);

    g_timer_id = SetTimer(g_hwnd, 1, FRAME_MS, nullptr);

    MSG msg = {0};
    while (sim_running && GetMessage(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    eeprom_write_all_data();
    KillTimer(g_hwnd, g_timer_id);
    if (g_bitmap) DeleteObject(g_bitmap);

    return 0;
}
