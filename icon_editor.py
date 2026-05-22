"""
WouoUI 图标编辑器 — 全功能像素图标编辑工具
=============================================
Version 0.14

支持可变尺寸正方形画布（默认 48×48），
可导入/导出 PNG 和 C 代码，实时预览代码。

功能:
  ✓ 可变画布大小（8×8 ~ 256×256，始终正方形）
  ✓ 画笔粗细（1×1 ~ 4×4）
  ✓ 导入/导出 PNG（支持缩放与裁剪）
  ✓ 导入/导出 C 代码 & 原始十六进制
  ✓ 绘图工具：铅笔、直线、矩形（圆角）、圆形、框选
  ✓ 反色、旋转（90°/180°/270°）、镜像翻转
  ✓ 一键圆角裁剪（圆弧 / WouoUI 原生）
  ✓ C 代码实时预览（C 格式 / 原始十六进制）
  ✓ Pillow 模块自动安装/卸载
  ✓ 多步撤销/重做，复制/粘贴
"""

VERSION = "0.14"
import tkinter as tk
import tkinter.font as tkfont
from tkinter import filedialog, messagebox, ttk
import os
import sys
import math
import subprocess
import threading

def _find_pip():
    py = sys.executable
    pip = os.path.join(os.path.dirname(py), "pip.exe")
    if not os.path.isfile(pip):
        pip = os.path.join(os.path.dirname(py), "pip3.exe")
    if not os.path.isfile(pip):
        pip = "pip"
    return pip


def _pip_window(title, cmd_parts, on_done=None):
    win = tk.Toplevel()
    win.title(title)
    win.resizable(False, False)
    win.grab_set()

    ttk.Label(win, text=title + "...",
              font=("", 10)).pack(padx=24, pady=(12, 2))
    status_label = ttk.Label(win, text="正在启动 pip...",
                             font=("", 9), foreground="#666")
    status_label.pack(padx=24)

    from tkinter.scrolledtext import ScrolledText
    log = ScrolledText(win, font=("Consolas", 9), wrap=tk.WORD,
                       height=10, width=64, bg="#FAFAFA", relief=tk.SUNKEN,
                       borderwidth=2)
    log.pack(padx=12, pady=(6, 4))

    pb = ttk.Progressbar(win, mode="indeterminate", length=360)
    pb.pack(padx=24, pady=(0, 4))
    pb.start()

    cancel_flag = [False]
    proc_holder = [None]
    result = {"ok": False, "text": "", "done": False}

    def do_cancel():
        cancel_flag[0] = True
        p = proc_holder[0]
        if p is not None:
            try:
                p.kill()
            except Exception:
                pass
        win.after(200, win.destroy)

    ttk.Button(win, text="取消", command=do_cancel, width=10).pack(pady=(6, 10))

    win.update_idletasks()
    win.geometry("520x300")
    x = (win.winfo_screenwidth() - 520) // 2
    y = (win.winfo_screenheight() - 300) // 2
    win.geometry(f"+{max(0,x)}+{max(0,y)}")

    def _log(msg):
        line = msg.rstrip()
        if not line:
            return
        win.after(0, lambda: (log.insert(tk.END, line + "\n"),
                              log.see(tk.END),
                              status_label.config(text=line)))

    def _run():
        try:
            proc = subprocess.Popen(
                cmd_parts,
                stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                text=True, bufsize=1
            )
            proc_holder[0] = proc
            for line in iter(proc.stdout.readline, ""):
                if cancel_flag[0]:
                    break
                _log(line)
            proc.stdout.close()
            proc.wait()
            result["ok"] = proc.returncode == 0
            result["done"] = True
        except Exception as e:
            _log(str(e))
            result["text"] = str(e)
            result["done"] = True
        finally:
            if on_done:
                win.after(0, lambda: on_done(result))
            win.after(0, win.destroy)

    threading.Thread(target=_run, daemon=True).start()
    win.wait_window()
    return result


def _ensure_pil():
    try:
        from PIL import Image
        return Image
    except ImportError:
        pass

    pip = _find_pip()
    py = sys.executable

    ret = messagebox.askyesno(
        "缺少 Pillow 模块",
        f"导入/导出 PNG 需要 Pillow 模块。\n\n"
        f"当前 Python: {os.path.basename(py)}\n"
        f"路径: {py}\n\n"
        f"是否自动安装 Pillow？"
    )
    if not ret:
        messagebox.showerror(
            "缺少 Pillow 模块",
            f"当前 Python: {py}\n\n"
            f"未找到 Pillow 模块。请手动运行:\n"
            f'  "{pip}" install Pillow'
        )
        return None

    result = _pip_window("安装 Pillow", [pip, "install", "Pillow"])

    if result["done"] and result["ok"]:
        try:
            from PIL import Image
            return Image
        except ImportError:
            pass

    if not result["done"]:
        return None

    messagebox.showerror(
        "安装失败",
        f"自动安装 Pillow 失败。\n\n"
        f"请手动运行:\n"
        f'  "{pip}" install Pillow\n\n'
        f"错误信息:\n{result['text']}"
    )
    return None

ICON_W = 48
ICON_H = 48
BYTES_PER_ROW = (ICON_W + 7) // 8
CELL_SIZE = 14
CANVAS_W = ICON_W * CELL_SIZE
CANVAS_H = ICON_H * CELL_SIZE
PALETTE = {0: "#FFFFFF", 1: "#000000"}
PALETTE_REV = {0: "黑色(前景)", 1: "白色(背景)"}


# ── 像素矩阵工具函数 ─────────────────────────────────────────────

def new_pixels(fill=0):
    return [[fill] * ICON_W for _ in range(ICON_H)]


def decode_c_bytes(data):
    pixels = new_pixels(0)
    for y in range(ICON_H):
        for x in range(ICON_W):
            idx = y * BYTES_PER_ROW + x // 8
            bit = x % 8
            if idx < len(data):
                pixels[y][x] = (data[idx] >> bit) & 1
    return pixels


def encode_pixels(pixels):
    result = []
    for y in range(ICON_H):
        for bx in range(BYTES_PER_ROW):
            byte_val = 0
            for bit in range(8):
                x = bx * 8 + bit
                if x < ICON_W and pixels[y][x]:
                    byte_val |= (1 << bit)
            result.append(byte_val)
    return result


def infer_square_size(byte_count):
    for n in range(1, 257):
        bpr = (n + 7) // 8
        if n * bpr == byte_count:
            return n
    return None


def format_c_array(byte_data, array_name="icon"):
    lines = [f"PROGMEM const uint8_t {array_name}[{len(byte_data)}] = {{"]
    for i in range(0, len(byte_data), 16):
        chunk = byte_data[i:i+16]
        lines.append("    " + ", ".join(f"0x{b:02X}" for b in chunk) + ",")
    lines.append("};")
    return "\n".join(lines)


def format_raw_hex(byte_data, sep=" ", upper=True, cols=16):
    fmt = "{:02X}" if upper else "{:02x}"
    parts = [fmt.format(b) for b in byte_data]
    if cols and cols > 0:
        lines = [sep.join(parts[i:i+cols]) for i in range(0, len(parts), cols)]
        return "\n".join(lines)
    return sep.join(parts)


def pixels_from_png(filepath, scale_to_fit=True, scale_factor=None):
    from PIL import Image
    img = Image.open(filepath)

    if img.mode in ('P', 'PA'):
        if 'transparency' in img.info:
            img = img.convert('RGBA')
        else:
            img = img.convert('RGB')
    if img.mode == 'RGBA':
        bg = Image.new('RGB', img.size, (255, 255, 255))
        bg.paste(img, (0, 0), img)
        img = bg
    elif img.mode != 'RGB':
        img = img.convert('RGB')

    w, h = img.size
    if w != ICON_W or h != ICON_H:
        if scale_factor is not None and scale_factor > 0:
            nw = max(1, round(w * scale_factor))
            nh = max(1, round(h * scale_factor))
            img = img.resize((nw, nh), Image.LANCZOS)
            if nw >= ICON_W and nh >= ICON_H:
                left = (nw - ICON_W) // 2
                top = (nh - ICON_H) // 2
                img = img.crop((left, top, left + ICON_W, top + ICON_H))
            else:
                bg = Image.new('RGB', (ICON_W, ICON_H), (255, 255, 255))
                left = (ICON_W - nw) // 2
                top = (ICON_H - nh) // 2
                bg.paste(img, (left, top))
                img = bg
        elif scale_to_fit:
            img = img.resize((ICON_W, ICON_H), Image.NEAREST)
        else:
            left = (w - ICON_W) // 2
            top = (h - ICON_H) // 2
            img = img.crop((left, top, left + ICON_W, top + ICON_H))

    img = img.convert('L')
    pixels = new_pixels(0)
    for y in range(ICON_H):
        for x in range(ICON_W):
            p = img.getpixel((x, y))
            pixels[y][x] = 1 if p < 128 else 0
    return pixels


def preview_from_img(img, mode="fit", scale_pct=100):
    from PIL import Image
    if img.mode in ('P', 'PA'):
        if 'transparency' in img.info:
            img = img.convert('RGBA')
        else:
            img = img.convert('RGB')
    if img.mode == 'RGBA':
        bg = Image.new('RGB', img.size, (255, 255, 255))
        bg.paste(img, (0, 0), img)
        img = bg
    elif img.mode != 'RGB':
        img = img.convert('RGB')

    w, h = img.size
    if mode == "fit":
        img = img.resize((ICON_W, ICON_H), Image.NEAREST)
    elif mode == "crop":
        left = (w - ICON_W) // 2
        top = (h - ICON_H) // 2
        img = img.crop((left, top, left + ICON_W, top + ICON_H))
    else:
        factor = scale_pct / 100.0
        nw = max(1, round(w * factor))
        nh = max(1, round(h * factor))
        img = img.resize((nw, nh), Image.LANCZOS)
        if nw >= ICON_W and nh >= ICON_H:
            left = (nw - ICON_W) // 2
            top = (nh - ICON_H) // 2
            img = img.crop((left, top, left + ICON_W, top + ICON_H))
        else:
            bg = Image.new('RGB', (ICON_W, ICON_H), (255, 255, 255))
            left = (ICON_W - nw) // 2
            top = (ICON_H - nh) // 2
            bg.paste(img, (left, top))
            img = bg

    img = img.convert('L')
    pixels = new_pixels(0)
    for y in range(ICON_H):
        for x in range(ICON_W):
            p = img.getpixel((x, y))
            pixels[y][x] = 1 if p < 128 else 0
    return pixels


def pixels_to_png(pixels, filepath, scale=1):
    from PIL import Image
    img = Image.new("1", (ICON_W, ICON_H))
    for y in range(ICON_H):
        for x in range(ICON_W):
            img.putpixel((x, y), 0 if pixels[y][x] == 1 else 255)
    if scale != 1:
        img = img.resize((ICON_W * scale, ICON_H * scale), Image.NEAREST)
    img.save(filepath, "PNG")


# ── 形状绘制工具函数 ─────────────────────────────────────────────

def draw_line(pixels, x0, y0, x1, y1, color=1):
    """Bresenham 画线算法"""
    dx, dy = abs(x1 - x0), -abs(y1 - y0)
    sx = 1 if x0 < x1 else -1
    sy = 1 if y0 < y1 else -1
    err = dx + dy
    while True:
        if 0 <= x0 < ICON_W and 0 <= y0 < ICON_H:
            pixels[y0][x0] = color
        if x0 == x1 and y0 == y1:
            break
        e2 = 2 * err
        if e2 >= dy:
            err += dy
            x0 += sx
        if e2 <= dx:
            err += dx
            y0 += sy


def draw_circle(pixels, cx, cy, r, color=1, fill=False):
    """中点圆算法"""
    if fill:
        for y in range(ICON_H):
            for x in range(ICON_W):
                d2 = (x - cx) ** 2 + (y - cy) ** 2
                if d2 <= r * r:
                    pixels[y][x] = color
    else:
        x, y = r, 0
        d = 1 - r
        while x >= y:
            for dx, dy in [(x, y), (y, x), (-y, x), (-x, y),
                           (-x, -y), (-y, -x), (y, -x), (x, -y)]:
                px, py = cx + dx, cy + dy
                if 0 <= px < ICON_W and 0 <= py < ICON_H:
                    pixels[py][px] = color
            y += 1
            if d <= 0:
                d += 2 * y + 1
            else:
                x -= 1
                d += 2 * (y - x) + 1


def draw_rect(pixels, x0, y0, x1, y1, color=1, fill=False, radius=0):
    """画矩形，支持圆角"""
    x0, x1 = sorted((x0, x1))
    y0, y1 = sorted((y0, y1))
    x0, y0 = max(0, x0), max(0, y0)
    x1, y1 = min(ICON_W-1, x1), min(ICON_H-1, y1)

    if fill:
        for y in range(y0, y1+1):
            for x in range(x0, x1+1):
                if radius > 0:
                    # 检查是否在圆角区域内
                    if x < x0 + radius and y < y0 + radius:
                        if (x - (x0 + radius))**2 + (y - (y0 + radius))**2 > radius**2:
                            continue
                    elif x > x1 - radius and y < y0 + radius:
                        if (x - (x1 - radius))**2 + (y - (y0 + radius))**2 > radius**2:
                            continue
                    elif x < x0 + radius and y > y1 - radius:
                        if (x - (x0 + radius))**2 + (y - (y1 - radius))**2 > radius**2:
                            continue
                    elif x > x1 - radius and y > y1 - radius:
                        if (x - (x1 - radius))**2 + (y - (y1 - radius))**2 > radius**2:
                            continue
                pixels[y][x] = color
    else:
        # 边框
        old = [row[:] for row in pixels]
        for y in range(y0, y1+1):
            for x in range(x0, x1+1):
                on_edge = (y == y0 or y == y1 or x == x0 or x == x1)
                if on_edge:
                    if radius > 0:
                        in_corner = False
                        if x < x0 + radius and y < y0 + radius:
                            in_corner = True
                            if (x - (x0 + radius))**2 + (y - (y0 + radius))**2 <= radius**2:
                                pixels[y][x] = color
                        elif x > x1 - radius and y < y0 + radius:
                            in_corner = True
                            if (x - (x1 - radius))**2 + (y - (y0 + radius))**2 <= radius**2:
                                pixels[y][x] = color
                        elif x < x0 + radius and y > y1 - radius:
                            in_corner = True
                            if (x - (x0 + radius))**2 + (y - (y1 - radius))**2 <= radius**2:
                                pixels[y][x] = color
                        elif x > x1 - radius and y > y1 - radius:
                            in_corner = True
                            if (x - (x1 - radius))**2 + (y - (y1 - radius))**2 <= radius**2:
                                pixels[y][x] = color
                        if not in_corner:
                            pixels[y][x] = color
                    else:
                        pixels[y][x] = color


def center_dialog(win, parent):
    win.update_idletasks()
    x = parent.winfo_rootx() + (parent.winfo_width() - win.winfo_width()) // 2
    y = parent.winfo_rooty() + (parent.winfo_height() - win.winfo_height()) // 2
    win.geometry(f"+{max(0,x)}+{max(0,y)}")


def auto_size(win, pad_x=0, pad_y=0):
    win.update_idletasks()
    w = win.winfo_reqwidth() + pad_x
    h = win.winfo_reqheight() + pad_y
    win.geometry(f"{w}x{h}")


def make_button_bar(parent, buttons, **pack_kw):
    frame = ttk.Frame(parent)
    frame.pack(**pack_kw)
    inner = ttk.Frame(frame)
    inner.pack(anchor=tk.CENTER)
    for text, command in buttons:
        ttk.Button(inner, text=text, command=command, width=10).pack(side=tk.LEFT, padx=4)
    return frame


# ── 主应用 ───────────────────────────────────────────────────────

class IconEditor:
    def __init__(self):
        self.root = tk.Tk()
        self.root.title(f"WouoUI 图标编辑器 - {ICON_W}×{ICON_H}")
        self.root.minsize(900, 520)

        self.pixels = new_pixels(0)
        self.current_color = 1  # 1=黑色(前景)
        self.tool = "pencil"    # pencil, line, rect, circle
        self.brush_size = 1     # 1×1 ~ 4×4
        self.rect_fill = False
        self.circle_fill = False
        self.rect_radius = 0
        self.drawing = False
        self.drag_start = None
        self.history = []
        self.redo_stack = []
        self.clipboard = None
        self.modified = False
        self.hover_cell = None
        self.preview_outline = None
        self.preview_win = None
        self.has_selection = False
        self.sel_x1 = self.sel_y1 = self.sel_x2 = self.sel_y2 = 0
        self.sel_rect_id = None

        self._build_ui()
        self._render()
        # 设置窗口最小尺寸确保所有控件完整显示
        self.root.geometry("960x780")
        self.root.minsize(900, 780)
        self.root.update_idletasks()
        self.root.eval('tk::PlaceWindow . center')

    # ── UI 构建 ────────────────────────────────────────────────

    def _build_ui(self):
        root = self.root

        # ── 顶部菜单栏 ──
        menubar = tk.Menu(root)
        root.config(menu=menubar)

        file_menu = tk.Menu(menubar, tearoff=0)
        menubar.add_cascade(label="文件", menu=file_menu)
        file_menu.add_command(label="导入 PNG...", command=self.import_png, accelerator="Ctrl+O")
        file_menu.add_command(label="导出 PNG...", command=self.export_png, accelerator="Ctrl+S")
        file_menu.add_separator()
        file_menu.add_command(label="导入 C 代码...", command=self.import_c_code)
        file_menu.add_command(label="导出 C 代码...", command=self.export_c_code, accelerator="Ctrl+E")
        file_menu.add_command(label="导出原始十六进制...", command=self.export_raw_hex)
        file_menu.add_separator()
        file_menu.add_command(label="重置", command=self.reset)
        file_menu.add_command(label="退出", command=root.quit)

        edit_menu = tk.Menu(menubar, tearoff=0)
        menubar.add_cascade(label="编辑", menu=edit_menu)
        edit_menu.add_command(label="撤销", command=self.undo, accelerator="Ctrl+Z")
        edit_menu.add_command(label="重做", command=self.redo, accelerator="Ctrl+Y")
        edit_menu.add_separator()
        edit_menu.add_command(label="全选", command=self.select_all, accelerator="Ctrl+A")
        edit_menu.add_command(label="清空", command=self.clear_all)
        edit_menu.add_separator()
        edit_menu.add_command(label="复制", command=self.copy, accelerator="Ctrl+C")
        edit_menu.add_command(label="粘贴", command=self.paste, accelerator="Ctrl+V")

        canvas_menu = tk.Menu(menubar, tearoff=0)
        menubar.add_cascade(label="画布", menu=canvas_menu)
        canvas_menu.add_command(label="调整画布大小...", command=self._resize_canvas)
        canvas_menu.add_command(label="重置到 48×48", command=lambda: self._resize_canvas(48))

        self._tool_menu = tk.Menu(menubar, tearoff=0, postcommand=self._refresh_pillow_item)
        menubar.add_cascade(label="工具", menu=self._tool_menu)
        self._tool_menu.add_command(label="反色", command=self.invert)
        self._tool_menu.add_separator()
        self._tool_menu.add_command(label="左旋 90°", command=lambda: self.rotate(1))
        self._tool_menu.add_command(label="右旋 90°", command=lambda: self.rotate(-1))
        self._tool_menu.add_command(label="旋转 180°", command=lambda: self.rotate(2))
        self._tool_menu.add_separator()
        self._tool_menu.add_command(label="水平镜像", command=lambda: self.flip("h"))
        self._tool_menu.add_command(label="垂直镜像", command=lambda: self.flip("v"))
        self._tool_menu.add_separator()
        self._tool_menu.add_command(label="圆角裁剪...", command=self.apply_rounded_corners)
        self._tool_menu.add_separator()
        self._pillow_item_idx = self._tool_menu.index(tk.END) + 1
        self._tool_menu.add_command(label="", command=self._pillow_action)

        help_menu = tk.Menu(menubar, tearoff=0)
        menubar.add_cascade(label="帮助", menu=help_menu)
        help_menu.add_command(label="使用说明", command=self.help_dialog)
        help_menu.add_command(label="关于...", command=self.about_dialog)

        self._refresh_pillow_item()

        # ── 主布局 ──
        main_frame = ttk.Frame(root, padding=6)
        main_frame.pack(fill=tk.BOTH, expand=True)

        # 左侧: 画布
        left_frame = ttk.Frame(main_frame)
        left_frame.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)

        canvas_frame = ttk.LabelFrame(left_frame, text="像素编辑区域", padding=4)
        canvas_frame.pack(fill=tk.BOTH, expand=True)

        cv_container = ttk.Frame(canvas_frame)
        cv_container.pack(fill=tk.BOTH, expand=True, padx=4, pady=4)

        self.cv = tk.Canvas(cv_container, width=CANVAS_W+2, height=CANVAS_H+2,
                            bg="#E0E0E0", highlightthickness=0)
        self.cv.grid(row=0, column=0, sticky="nsew")

        v_scroll = ttk.Scrollbar(cv_container, orient=tk.VERTICAL, command=self.cv.yview)
        v_scroll.grid(row=0, column=1, sticky="ns")
        h_scroll = ttk.Scrollbar(cv_container, orient=tk.HORIZONTAL, command=self.cv.xview)
        h_scroll.grid(row=1, column=0, sticky="ew")
        self.cv.configure(xscrollcommand=h_scroll.set, yscrollcommand=v_scroll.set)
        cv_container.grid_rowconfigure(0, weight=1)
        cv_container.grid_columnconfigure(0, weight=1)
        self.cv.bind("<Button-1>", self._mouse_down)
        self.cv.bind("<B1-Motion>", self._mouse_move)
        self.cv.bind("<ButtonRelease-1>", self._mouse_up)
        self.cv.bind("<Motion>", self._mouse_hover)
        # 右键取色
        self.cv.bind("<Button-3>", self._pick_color)

        # ── 右侧工具栏 ──
        right_frame = ttk.Frame(main_frame, width=220)
        right_frame.pack(side=tk.RIGHT, fill=tk.Y, padx=(6, 0))
        right_frame.pack_propagate(False)

        # 颜色选择
        color_frame = ttk.LabelFrame(right_frame, text="颜色", padding=4)
        color_frame.pack(fill=tk.X, pady=(0, 4))

        self.color_btn_frame = ttk.Frame(color_frame)
        self.color_btn_frame.pack()
        self.fg_btn = tk.Button(self.color_btn_frame, bg="#000000", width=4, height=1,
                                relief=tk.SUNKEN, command=lambda: self._set_color(1))
        self.fg_btn.pack(side=tk.LEFT, padx=2)
        self.bg_btn = tk.Button(self.color_btn_frame, bg="#FFFFFF", width=4, height=1,
                                relief=tk.RAISED, command=lambda: self._set_color(0))
        self.bg_btn.pack(side=tk.LEFT, padx=2)
        self.swap_btn = tk.Button(color_frame, text="⇄ 交换", command=self._swap_color)
        self.swap_btn.pack(pady=(2, 0))

        # 绘图工具
        tool_frame = ttk.LabelFrame(right_frame, text="绘图工具", padding=4)
        tool_frame.pack(fill=tk.X, pady=(0, 4))

        tools = [
            ("✏ 铅笔", "pencil"),
            ("╱ 直线", "line"),
            ("▭ 矩形", "rect"),
            ("◯ 圆形", "circle"),
            ("▣ 框选", "select"),
        ]
        self.tool_var = tk.StringVar(value="pencil")
        for text, val in tools:
            rb = ttk.Radiobutton(tool_frame, text=text, variable=self.tool_var,
                                 value=val, command=self._on_tool_change)
            rb.pack(anchor=tk.W, padx=4, pady=1)

        # 选项
        opt_frame = ttk.LabelFrame(right_frame, text="选项", padding=4)
        opt_frame.pack(fill=tk.X, pady=(0, 4))

        self.fill_var = tk.BooleanVar(value=False)
        ttk.Checkbutton(opt_frame, text="填充形状", variable=self.fill_var).pack(anchor=tk.W)
        self.fill_var.trace("w", self._on_tool_change)

        radius_frame = ttk.Frame(opt_frame)
        radius_frame.pack(fill=tk.X, pady=(2, 0))
        ttk.Label(radius_frame, text="圆角半径:").pack(side=tk.LEFT)
        self.radius_var = tk.IntVar(value=0)
        self.radius_spin = ttk.Spinbox(radius_frame, from_=0, to=24, width=4,
                                       textvariable=self.radius_var)
        self.radius_spin.pack(side=tk.RIGHT)

        brush_frame = ttk.Frame(opt_frame)
        brush_frame.pack(fill=tk.X, pady=(2, 0))
        ttk.Label(brush_frame, text="画笔粗细:").pack(side=tk.LEFT)
        self.brush_var = tk.IntVar(value=1)
        def _on_brush_change(*_):
            self.brush_size = self.brush_var.get()
        self.brush_var.trace("w", _on_brush_change)
        self.brush_spin = ttk.Spinbox(brush_frame, from_=1, to=4, width=4,
                                      textvariable=self.brush_var)
        self.brush_spin.pack(side=tk.RIGHT)

        # 操作按钮
        op_frame = ttk.LabelFrame(right_frame, text="操作", padding=4)
        op_frame.pack(fill=tk.X, pady=(0, 4))

        ttk.Button(op_frame, text="🌗 反色", command=self.invert).pack(fill=tk.X, pady=1)
        ttk.Button(op_frame, text="↻ 右旋 90°", command=lambda: self.rotate(-1)).pack(fill=tk.X, pady=1)
        ttk.Button(op_frame, text="↺ 左旋 90°", command=lambda: self.rotate(1)).pack(fill=tk.X, pady=1)
        ttk.Button(op_frame, text="⇔ 水平镜像", command=lambda: self.flip("h")).pack(fill=tk.X, pady=1)
        ttk.Button(op_frame, text="⇕ 垂直镜像", command=lambda: self.flip("v")).pack(fill=tk.X, pady=1)
        ttk.Button(op_frame, text="⭗ 圆角裁剪...", command=self.apply_rounded_corners).pack(fill=tk.X, pady=1)

        # 导入导出
        io_frame = ttk.LabelFrame(right_frame, text="导入/导出", padding=4)
        io_frame.pack(fill=tk.X, pady=(0, 4))

        ttk.Button(io_frame, text="📂 导入 PNG", command=self.import_png).pack(fill=tk.X, pady=1)
        ttk.Button(io_frame, text="💾 导出 PNG", command=self.export_png).pack(fill=tk.X, pady=1)
        ttk.Button(io_frame, text="📄 显示 C 代码", command=self.show_c_code).pack(fill=tk.X, pady=1)
        ttk.Button(io_frame, text="📺 预览窗口", command=self.show_preview_window).pack(fill=tk.X, pady=1)

        # ── 底部状态栏 ──
        self.status_var = tk.StringVar(value="就绪")
        status_bar = ttk.Label(root, textvariable=self.status_var,
                               relief=tk.SUNKEN, anchor=tk.W, padding=(6, 2))
        status_bar.pack(fill=tk.X)

        # ── 键盘快捷键 ──
        root.bind("<Control-o>", lambda e: self.import_png())
        root.bind("<Control-s>", lambda e: self.export_png())
        root.bind("<Control-e>", lambda e: self.export_c_code())
        root.bind("<Control-z>", lambda e: self.undo())
        root.bind("<Control-y>", lambda e: self.redo())
        root.bind("<Control-a>", lambda e: self.select_all())
        root.bind("<Control-c>", lambda e: self.copy())
        root.bind("<Control-v>", lambda e: self.paste())
        root.bind("<Up>", lambda e: self._move_selection(0, -1))
        root.bind("<Down>", lambda e: self._move_selection(0, 1))
        root.bind("<Left>", lambda e: self._move_selection(-1, 0))
        root.bind("<Right>", lambda e: self._move_selection(1, 0))
        root.bind("<Escape>", lambda e: self._clear_selection())
        root.bind("<space>", lambda e: self._rotate_selection(True))
        root.bind("<Shift-space>", lambda e: self._rotate_selection(False))

    # ── 工具事件 ────────────────────────────────────────────────

    def _on_tool_change(self, *_):
        self.tool = self.tool_var.get()
        self.brush_spin.config(state=tk.NORMAL if self.tool == "pencil" else tk.DISABLED)
        self.rect_fill = self.fill_var.get()
        self.circle_fill = self.fill_var.get()
        self.rect_radius = self.radius_var.get()
        if self.tool != "select":
            self._clear_selection()
        if self.tool == "pencil":
            self.cv.config(cursor="pencil")
        else:
            self.cv.config(cursor="crosshair")

    def _set_color(self, c):
        self.current_color = c
        self.fg_btn.config(relief=tk.SUNKEN if c == 1 else tk.RAISED)
        self.bg_btn.config(relief=tk.SUNKEN if c == 0 else tk.RAISED)

    def _swap_color(self):
        self._set_color(1 - self.current_color)

    def _save_state(self):
        self.history.append([row[:] for row in self.pixels])
        if len(self.history) > 50:
            self.history.pop(0)
        self.redo_stack.clear()
        self.modified = True
        self._update_status("")

    def undo(self):
        if self.history:
            self.redo_stack.append([row[:] for row in self.pixels])
            self.pixels = self.history.pop()
            self._render()
            self._update_status("已撤销")
            self.modified = True

    def redo(self):
        if self.redo_stack:
            self.history.append([row[:] for row in self.pixels])
            self.pixels = self.redo_stack.pop()
            self._render()
            self._update_status("已重做")
            self.modified = True

    # ── 画笔粗细 ────────────────────────────────────────────────

    def _draw_brush(self, cx, cy):
        s = self.brush_size
        half = s // 2
        for dy in range(s):
            for dx in range(s):
                px = cx - half + dx
                py = cy - half + dy
                if 0 <= px < ICON_W and 0 <= py < ICON_H:
                    self.pixels[py][px] = self.current_color

    def _draw_brush_line(self, x0, y0, x1, y1):
        dx, dy = abs(x1 - x0), -abs(y1 - y0)
        sx = 1 if x0 < x1 else -1
        sy = 1 if y0 < y1 else -1
        err = dx + dy
        while True:
            self._draw_brush(x0, y0)
            if x0 == x1 and y0 == y1:
                break
            e2 = 2 * err
            if e2 >= dy:
                err += dy
                x0 += sx
            if e2 <= dx:
                err += dx
                y0 += sy

    # ── 鼠标事件 ────────────────────────────────────────────────

    def _mouse_hover(self, event):
        x, y = event.x // CELL_SIZE, event.y // CELL_SIZE
        if 0 <= x < ICON_W and 0 <= y < ICON_H:
            color_name = "前景(黑)" if self.current_color == 1 else "背景(白)"
            tool_names = {"pencil": "铅笔", "line": "直线", "rect": "矩形", "circle": "圆形", "select": "框选"}
            tool_name = tool_names.get(self.tool, self.tool)
            extra = f"  笔刷:{self.brush_size}×{self.brush_size}" if self.tool == "pencil" else ""
            self._update_status(f"工具:{tool_name}{extra}  颜色:{color_name}  坐标 ({x}, {y})  值={self.pixels[y][x]}")
            if self.tool != "select":
                if self.hover_cell is not None:
                    self.cv.delete(self.hover_cell)
                if self.tool == "pencil":
                    s = self.brush_size
                    half = s // 2
                    px = (x - half) * CELL_SIZE + 1
                    py = (y - half) * CELL_SIZE + 1
                    self.hover_cell = self.cv.create_rectangle(
                        px, py, px + s * CELL_SIZE, py + s * CELL_SIZE,
                        outline="#FF0000", width=2, dash=(3, 2))
                else:
                    px = x * CELL_SIZE + 1
                    py = y * CELL_SIZE + 1
                    self.hover_cell = self.cv.create_rectangle(
                        px, py, px+CELL_SIZE, py+CELL_SIZE,
                        outline="#FF0000", width=2, dash=(3, 2))
            else:
                if self.hover_cell is not None:
                    self.cv.delete(self.hover_cell)
                    self.hover_cell = None
        else:
            if self.hover_cell is not None:
                self.cv.delete(self.hover_cell)
                self.hover_cell = None

    def _mouse_down(self, event):
        x, y = event.x // CELL_SIZE, event.y // CELL_SIZE
        if not (0 <= x < ICON_W and 0 <= y < ICON_H):
            return

        self._save_state()
        self.drawing = True
        self.drag_start = (x, y)

        if self.tool == "pencil":
            self._draw_brush(x, y)
            self._render()
        elif self.tool == "select":
            self._clear_selection()
        elif self.tool == "line":
            pass
        elif self.tool == "rect":
            pass
        elif self.tool == "circle":
            pass

    def _mouse_move(self, event):
        x, y = event.x // CELL_SIZE, event.y // CELL_SIZE
        if not (0 <= x < ICON_W and 0 <= y < ICON_H):
            return

        color_name = "前景(黑)" if self.current_color == 1 else "背景(白)"
        tool_names = {"pencil": "铅笔", "line": "直线", "rect": "矩形", "circle": "圆形", "select": "框选"}
        tool_name = tool_names.get(self.tool, self.tool)
        self._update_status(f"工具:{tool_name}  颜色:{color_name}  坐标 ({x}, {y})  值={self.pixels[y][x]}")

        if not self.drawing or self.drag_start is None:
            return

        if self.tool == "pencil":
            sx, sy = self.drag_start
            if (x, y) != (sx, sy):
                self._draw_brush_line(sx, sy, x, y)
                self.drag_start = (x, y)
                self._render()
        else:
            x0, y0 = self.drag_start
            self._render()
            if self.preview_outline is not None:
                self.cv.delete(self.preview_outline)
                self.preview_outline = None

            if self.tool == "select":
                self.preview_outline = []
                xa, xb = sorted((x0, x))
                ya, yb = sorted((y0, y))
                for py in range(ya, yb + 1):
                    self.preview_outline.append(self.cv.create_rectangle(
                        xa*CELL_SIZE+1, py*CELL_SIZE+1,
                        xa*CELL_SIZE+1+CELL_SIZE, py*CELL_SIZE+1+CELL_SIZE,
                        outline="#0066FF", width=1, fill=""))
                    self.preview_outline.append(self.cv.create_rectangle(
                        xb*CELL_SIZE+1, py*CELL_SIZE+1,
                        xb*CELL_SIZE+1+CELL_SIZE, py*CELL_SIZE+1+CELL_SIZE,
                        outline="#0066FF", width=1, fill=""))
                for px in range(xa + 1, xb):
                    self.preview_outline.append(self.cv.create_rectangle(
                        px*CELL_SIZE+1, ya*CELL_SIZE+1,
                        px*CELL_SIZE+1+CELL_SIZE, ya*CELL_SIZE+1+CELL_SIZE,
                        outline="#0066FF", width=1, fill=""))
                    self.preview_outline.append(self.cv.create_rectangle(
                        px*CELL_SIZE+1, yb*CELL_SIZE+1,
                        px*CELL_SIZE+1+CELL_SIZE, yb*CELL_SIZE+1+CELL_SIZE,
                        outline="#0066FF", width=1, fill=""))
            elif self.tool == "line":
                self.preview_outline = []
                lx0, ly0, lx1, ly1 = x0, y0, x, y
                dx, dy = abs(lx1 - lx0), -abs(ly1 - ly0)
                sx = 1 if lx0 < lx1 else -1
                sy = 1 if ly0 < ly1 else -1
                err = dx + dy
                while True:
                    if 0 <= lx0 < ICON_W and 0 <= ly0 < ICON_H:
                        self.preview_outline.append(self.cv.create_rectangle(
                            lx0*CELL_SIZE+1, ly0*CELL_SIZE+1,
                            lx0*CELL_SIZE+1+CELL_SIZE, ly0*CELL_SIZE+1+CELL_SIZE,
                            outline="#FF0000", width=1, fill=""))
                    if lx0 == lx1 and ly0 == ly1:
                        break
                    e2 = 2 * err
                    if e2 >= dy:
                        err += dy
                        lx0 += sx
                    if e2 <= dx:
                        err += dx
                        ly0 += sy
            elif self.tool == "rect":
                 self.preview_outline = []
                 x1, y1 = x, y
                 xa, xb = sorted((x0, x1))
                 ya, yb = sorted((y0, y1))
                 for dy in range(ya, yb+1):
                      for dx in range(xa, xb+1):
                          on_edge = dy == ya or dy == yb or dx == xa or dx == xb
                          if on_edge:
                              show = True
                              r = self.radius_var.get()
                              if r > 0:
                                  if dx < xa + r and dy < ya + r:
                                      show = (dx-xa-r+0.5)**2 + (dy-ya-r+0.5)**2 <= r*r
                                  elif dx > xb - r and dy < ya + r:
                                      show = (dx-xb+r-0.5)**2 + (dy-ya-r+0.5)**2 <= r*r
                                  elif dx < xa + r and dy > yb - r:
                                      show = (dx-xa-r+0.5)**2 + (dy-yb+r-0.5)**2 <= r*r
                                  elif dx > xb - r and dy > yb - r:
                                      show = (dx-xb+r-0.5)**2 + (dy-yb+r-0.5)**2 <= r*r
                              if show:
                                  self.preview_outline.append(self.cv.create_rectangle(
                                      dx*CELL_SIZE+1, dy*CELL_SIZE+1,
                                      dx*CELL_SIZE+1+CELL_SIZE, dy*CELL_SIZE+1+CELL_SIZE,
                                      outline="#FF0000", width=1, fill=""))
            elif self.tool == "circle":
                self.preview_outline = []
                cx = (x0 + x) // 2
                cy = (y0 + y) // 2
                r = int(math.sqrt((x - x0)**2 + (y - y0)**2) // 2)
                if r > 0:
                    fill = self.fill_var.get()
                    if fill:
                        ya = max(0, cy - r)
                        yb = min(ICON_H - 1, cy + r)
                        for py in range(ya, yb + 1):
                            for px in range(ICON_W):
                                if (px - cx)**2 + (py - cy)**2 <= r*r:
                                    self.preview_outline.append(self.cv.create_rectangle(
                                        px*CELL_SIZE+1, py*CELL_SIZE+1,
                                        px*CELL_SIZE+1+CELL_SIZE, py*CELL_SIZE+1+CELL_SIZE,
                                        outline="#FF0000", width=1, fill=""))
                    else:
                        dx, dy = r, 0
                        d = 1 - r
                        while dx >= dy:
                            for sx, sy in [(dx, dy), (dy, dx), (-dy, dx), (-dx, dy),
                                           (-dx, -dy), (-dy, -dx), (dy, -dx), (dx, -dy)]:
                                px, py = cx + sx, cy + sy
                                if 0 <= px < ICON_W and 0 <= py < ICON_H:
                                    self.preview_outline.append(self.cv.create_rectangle(
                                        px*CELL_SIZE+1, py*CELL_SIZE+1,
                                        px*CELL_SIZE+1+CELL_SIZE, py*CELL_SIZE+1+CELL_SIZE,
                                        outline="#FF0000", width=1, fill=""))
                            dy += 1
                            if d <= 0:
                                d += 2 * dy + 1
                            else:
                                dx -= 1
                                d += 2 * (dy - dx) + 1

    def _mouse_up(self, event):
        if not self.drawing:
            return
        self.drawing = False

        if self.preview_outline is not None:
            self.cv.delete(self.preview_outline)
            self.preview_outline = None

        x1, y1 = event.x // CELL_SIZE, event.y // CELL_SIZE
        if not self.drag_start:
            self._render()
            return

        x0, y0 = self.drag_start

        if self.tool == "select":
            xa, xb = sorted((x0, x1))
            ya, yb = sorted((y0, y1))
            if xb - xa > 0 or yb - ya > 0:
                self.has_selection = True
                self.sel_x1, self.sel_y1 = xa, ya
                self.sel_x2, self.sel_y2 = xb, yb
                self._render()
                self._draw_selection()
                self._update_status(f"已框选区域 ({xa},{ya})-({xb},{yb})")
            else:
                self._render()
        elif self.tool == "line":
            draw_line(self.pixels, x0, y0, x1, y1, self.current_color)
        elif self.tool == "rect":
            r = self.radius_var.get()
            draw_rect(self.pixels, x0, y0, x1, y1, self.current_color, self.rect_fill, r)
        elif self.tool == "circle":
            cx, cy = (x0 + x1) // 2, (y0 + y1) // 2
            r = int(math.sqrt((x1 - x0)**2 + (y1 - y0)**2) // 2)
            if r > 0:
                draw_circle(self.pixels, cx, cy, r, self.current_color, self.circle_fill)

        self.drag_start = None
        self._render()

    def _pick_color(self, event):
        x, y = event.x // CELL_SIZE, event.y // CELL_SIZE
        if 0 <= x < ICON_W and 0 <= y < ICON_H:
            self._set_color(self.pixels[y][x])

    # ── 操作函数 ────────────────────────────────────────────────

    def invert(self):
        self._save_state()
        for y in range(ICON_H):
            for x in range(ICON_W):
                self.pixels[y][x] = 1 - self.pixels[y][x]
        self._render()
        self._update_status("已反色")

    def rotate(self, times):
        self._save_state()
        p = self.pixels
        for _ in range(times % 4):
            p = [[p[ICON_H-1-c][r] for c in range(ICON_H)] for r in range(ICON_W)]
        self.pixels = p
        self._render()
        self._update_status(f"已旋转 {'左' if times > 0 else '右'} 90° × {abs(times)}")

    def flip(self, direction):
        self._save_state()
        if direction == "h":
            for y in range(ICON_H):
                self.pixels[y] = self.pixels[y][::-1]
            self._update_status("已水平镜像")
        else:
            self.pixels = self.pixels[::-1]
            self._update_status("已垂直镜像")
        self._render()

    def apply_rounded_corners(self):
        win = tk.Toplevel(self.root)
        win.title("圆角裁剪")
        win.resizable(False, False)
        win.transient(self.root)
        win.grab_set()

        # ── 方式选择 ──
        mode_var = tk.StringVar(value="circle")

        mode_frame = ttk.LabelFrame(win, text="裁剪方式", padding=6)
        mode_frame.pack(fill=tk.X, padx=10, pady=(10, 2))

        mode_sel = ttk.Frame(mode_frame)
        mode_sel.pack()
        ttk.Radiobutton(mode_sel, text="圆弧裁剪", variable=mode_var,
                        value="circle").pack(side=tk.LEFT, padx=6)
        ttk.Radiobutton(mode_sel, text="WouoUI 原生圆角", variable=mode_var,
                        value="wouo").pack(side=tk.LEFT, padx=6)

        # ── 按钮栏 (顶部) ──
        make_button_bar(win, [("确定", lambda: do_apply(None)), ("取消", win.destroy)],
                        fill=tk.X, padx=10, pady=(6, 0))

        content_frame = ttk.Frame(win)
        content_frame.pack(fill=tk.BOTH, expand=True, padx=10)

        # ── 圆弧参数 ──
        circle_frame = ttk.Frame(content_frame)
        circle_frame.pack(fill=tk.X, pady=(4, 0))

        ttk.Label(circle_frame, text="圆角半径:").pack()
        radius_var = tk.IntVar(value=5)

        slider_frame = ttk.Frame(circle_frame)
        slider_frame.pack()
        ttk.Label(slider_frame, text="0").pack(side=tk.LEFT, padx=2)
        slider = ttk.Scale(slider_frame, from_=0, to=24, orient=tk.HORIZONTAL,
                           variable=radius_var, length=280)
        slider.pack(side=tk.LEFT, padx=4)
        ttk.Label(slider_frame, text="24").pack(side=tk.LEFT, padx=2)

        value_label = ttk.Label(circle_frame, text="半径: 5", font=("Consolas", 12))
        value_label.pack(pady=(2, 0))

        # ── 原生说明 ──
        wouo_frame = ttk.Frame(content_frame)

        wouo_label = ttk.Label(wouo_frame,
            text="四角各去除 5 个像素:\n(0,0) (0,1) (1,0) (0,2) (2,0)",
            font=("Consolas", 10), justify=tk.CENTER)
        wouo_label.pack(pady=(4, 0))

        wouo_sub = ttk.Label(wouo_frame,
            text="应用后共切除 20 个像素",
            font=("Consolas", 10), foreground="#666")
        wouo_sub.pack(pady=(2, 0))

        # ── 预览 ──
        preview_frame = ttk.LabelFrame(content_frame, text="左上角圆角效果预览", padding=6)
        preview_frame.pack(fill=tk.BOTH, expand=True, pady=4)

        PV2 = 14
        pv_cv = tk.Canvas(preview_frame, width=10*PV2+2, height=10*PV2+2,
                          bg="#F0F0F0", highlightthickness=1,
                          highlightbackground="#C0C0C0", relief=tk.FLAT)
        pv_cv.pack()

        removed_label = ttk.Label(preview_frame, text="", font=("Consolas", 10))
        removed_label.pack(pady=(4, 0))

        def toggle_mode(*args):
            m = mode_var.get()
            for w in (circle_frame, preview_frame, wouo_frame):
                w.pack_forget()
            if m == "circle":
                circle_frame.pack(fill=tk.X, pady=(4, 0))
                preview_frame.pack(fill=tk.BOTH, expand=True, pady=4)
                update_preview()
                auto_size(win, pad_x=20, pad_y=10)
            else:
                wouo_frame.pack(fill=tk.X, pady=(4, 0))
                auto_size(win, pad_x=20, pad_y=10)
            center_dialog(win, win.master)

        mode_var.trace("w", toggle_mode)

        def update_preview(*args):
            r = radius_var.get()
            value_label.config(text=f"半径: {r}")
            pv_cv.delete("all")
            size = max(r + 2, 10)
            cw = size * PV2 + 2
            pv_cv.config(width=cw, height=cw)
            removed = 0
            kept = 0
            for dy in range(size):
                for dx in range(size):
                    x1 = dx * PV2 + 1
                    y1 = dy * PV2 + 1
                    in_corner = dx <= r and dy <= r
                    if in_corner and dx*dx + dy*dy <= r*r:
                        fill = PALETTE[0]
                        kept += 1
                    elif in_corner:
                        fill = PALETTE[1]
                        removed += 1
                    elif dx == 0 or dy == 0:
                        fill = "#D0D0D0"
                    else:
                        fill = "#F5F5F5"
                    pv_cv.create_rectangle(x1, y1, x1+PV2, y1+PV2,
                                           fill=fill, outline="#E0E0E0", width=1)
            total_cut = 4 * removed
            removed_label.config(text=f"单角保留 {kept} 像素  ●  切除 {removed} 像素  ●  共切除 {total_cut} 像素")
            auto_size(win, pad_x=20, pad_y=10)
            center_dialog(win, win.master)

        radius_var.trace("w", update_preview)
        update_preview()

        def do_apply(event=None):
            m = mode_var.get()
            self._save_state()
            bg_color = self.current_color ^ 1

            if m == "wouo":
                offsets = [(0,0), (0,1), (1,0), (0,2), (2,0)]
                corners = [
                    (0, 0, 1, 1),
                    (ICON_W-1, 0, -1, 1),
                    (0, ICON_H-1, 1, -1),
                    (ICON_W-1, ICON_H-1, -1, -1),
                ]
                for cx, cy, sdx, sdy in corners:
                    for dx, dy in offsets:
                        px, py = cx + sdx * dx, cy + sdy * dy
                        if 0 <= px < ICON_W and 0 <= py < ICON_H:
                            self.pixels[py][px] = bg_color
                self._update_status("已应用 WouoUI 原生圆角 — 每角去除 5 像素")
            else:
                r = radius_var.get()
                if r <= 0:
                    win.destroy()
                    return
                corners = [
                    (0, 0, 1, 1),
                    (ICON_W-1, 0, -1, 1),
                    (0, ICON_H-1, 1, -1),
                    (ICON_W-1, ICON_H-1, -1, -1),
                ]
                for cx, cy, sdx, sdy in corners:
                    for dy in range(r + 1):
                        for dx in range(r + 1):
                            if dx*dx + dy*dy <= r*r:
                                px, py = cx + sdx * dx, cy + sdy * dy
                                if 0 <= px < ICON_W and 0 <= py < ICON_H:
                                    self.pixels[py][px] = bg_color
                self._update_status(f"已应用圆角裁剪 r={r}")

            self._render()
            win.destroy()

        win.bind("<Return>", do_apply)

    def about_dialog(self):
        win = tk.Toplevel(self.root)
        win.title("关于")
        win.resizable(False, False)
        win.transient(self.root)
        win.grab_set()

        ttk.Label(win, text="WouoUI 图标编辑器",
                  font=("", 16, "bold")).pack(pady=(20, 4))
        ttk.Label(win, text=f"Version {VERSION}",
                  font=("", 10)).pack()
        ttk.Label(win, text="作者: 罗米奇",
                  font=("", 10)).pack(pady=(8, 2))

        link_frame = ttk.Frame(win)
        link_frame.pack(pady=(4, 2))

        ttk.Label(link_frame, text="GitHub: ").pack(side=tk.LEFT)
        github_link = tk.Label(link_frame,
            text="https://github.com/TropicalcowHumanDoingHomework",
            font=("", 9), fg="#0000FF", cursor="hand2")
        github_link.pack(side=tk.LEFT)
        def open_github(e):
            import webbrowser
            webbrowser.open("https://github.com/TropicalcowHumanDoingHomework")
        github_link.bind("<Button-1>", open_github)

        bili_frame = ttk.Frame(win)
        bili_frame.pack(pady=(2, 8))

        ttk.Label(bili_frame, text="Bilibili: ").pack(side=tk.LEFT)
        bili_link = tk.Label(bili_frame,
            text="https://space.bilibili.com/549713590",
            font=("", 9), fg="#0000FF", cursor="hand2")
        bili_link.pack(side=tk.LEFT)
        def open_bilibili(e):
            import webbrowser
            webbrowser.open("https://space.bilibili.com/549713590?spm_id_from=333.1007.0.0")
        bili_link.bind("<Button-1>", open_bilibili)

        ttk.Button(win, text="确定", command=win.destroy, width=12).pack(pady=(10, 12))
        auto_size(win, pad_x=20, pad_y=16)
        center_dialog(win, win.master)

    def help_dialog(self):
        win = tk.Toplevel(self.root)
        win.title("使用说明")
        win.minsize(520, 400)
        win.transient(self.root)
        win.grab_set()

        text = tk.Text(win, font=("微软雅黑", 10) if "微软雅黑" in tkfont.families() else ("Helvetica", 10),
                       wrap=tk.WORD, bg="#FAFAFA", relief=tk.FLAT, padx=16, pady=12)
        scroll_y = ttk.Scrollbar(win, orient=tk.VERTICAL, command=text.yview)
        text.configure(yscrollcommand=scroll_y.set)
        text.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        scroll_y.pack(side=tk.RIGHT, fill=tk.Y)

        text.tag_configure("h1", font=("微软雅黑", 14, "bold") if "微软雅黑" in tkfont.families() else ("Helvetica", 14, "bold"),
                           spacing1=8, spacing3=4)
        text.tag_configure("h2", font=("微软雅黑", 11, "bold") if "微软雅黑" in tkfont.families() else ("Helvetica", 11, "bold"),
                            spacing1=12, spacing3=2, foreground="#2B579A")
        text.tag_configure("body", spacing1=2, spacing3=2, font=("微软雅黑", 10))
        text.tag_configure("item", spacing1=2, spacing3=1, lmargin1=24, lmargin2=48)
        text.tag_configure("sep", font=("", 1), spacing1=4, spacing3=4)

        def ins(tag, txt):
            text.insert(tk.END, txt + "\n", tag)

        ins("h1", f"WouoUI 图标编辑器 v{VERSION} — 使用说明")
        ins("sep", "")

        ins("h2", "简介")
        ins("body", f"专为 WouoUI 项目设计的正方形单色图标编辑工具（默认 {ICON_W}×{ICON_H}）。")
        ins("body", f"画布大小可在 8×8 ~ 256×256 范围内自由调整，始终保持正方形。")

        ins("sep", "")
        ins("h2", "文件操作")
        ins("item", f"• 导入 PNG    打开图片并转换为 {ICON_W}×{ICON_H} 单色图标，支持缩放与裁剪")
        ins("item", "• 导出 PNG    将当前图标导出为 PNG 图片（可调放大倍数 1×~16×）")
        ins("item", "• 导入 C 代码  从 .c/.h/.txt 文件导入图标数据")
        ins("item", "• 导出 C 代码  导出 C 语言数组或原始十六进制文本")
        ins("item", "• 导出原始十六进制  纯十六进制文本，空格分隔，每行 16 个")

        ins("sep", "")
        ins("h2", "画布操作")
        ins("item", "• 画布 → 调整画布大小  改变画布边长（8~256，始终正方形）")
        ins("item", "• 画布 → 重置到 48×48  一键恢复默认尺寸")
        ins("item", "  调整尺寸时原图居中放置（放大）或从中心裁剪（缩小）")
        ins("item", "  画布较大时出现滚动条，可拖动浏览")

        ins("sep", "")
        ins("h2", "编辑操作")
        ins("item", "• 撤销 / 重做  Ctrl+Z / Ctrl+Y，支持多步撤销")
        ins("item", "• 全选          填充全部像素为黑色")
        ins("item", "• 清空          清空所有像素")
        ins("item", "• 复制 / 粘贴   复制当前图标并在需要时粘贴恢复")

        ins("sep", "")
        ins("h2", "绘图工具")
        ins("item", "• 铅笔   点击或拖拽绘制，右侧可调节笔刷粗细（1×1 ~ 4×4）")
        ins("item", "• 直线   拖拽绘制一条直线，预览显示逐像素红色轮廓")
        ins("item", "• 矩形   拖拽绘制矩形，可设置填充和圆角半径（0~24）")
        ins("item", "• 圆形   拖拽绘制圆形，可设置填充")
        ins("item", "• 框选   绘制矩形选区，可用方向键移动，Space 旋转")

        ins("sep", "")
        ins("h2", "颜色")
        ins("item", "• 前景色（黑色）— 当前绘图颜色")
        ins("item", "• 背景色（白色）— 橡皮擦颜色")
        ins("item", "• 点击「交换」按钮快速切换前景/背景")
        ins("item", "• 右键点击画布像素可拾取该点颜色")

        ins("sep", "")
        ins("h2", "操作功能")
        ins("item", "• 反色 — 所有像素黑白反转")
        ins("item", "• 旋转 — 左旋 90° / 右旋 90° / 旋转 180°")
        ins("item", "• 镜像 — 水平翻转 / 垂直翻转")
        ins("item", "• 圆角裁剪 — 圆弧裁剪（半径可调）或 WouoUI 原生圆角")

        ins("sep", "")
        ins("h2", "预览与代码")
        ins("item", f"• 「预览窗口」按钮打开放大预览（自动根据画布大小调整倍率）")
        ins("item", "• C 代码实时预览，双击可自动复制到剪贴板")
        ins("item", "• 支持 C 语言格式和原始十六进制两种显示")

        ins("sep", "")
        ins("h2", "Pillow 模块管理")
        ins("item", "• 首次导入/导出 PNG 时自动检测并提示安装 Pillow")
        ins("item", "• 工具 → 安装/卸载 Pillow 模块... 可随时管理")
        ins("item", "• 安装过程实时显示 pip 输出日志")

        ins("sep", "")
        ins("h2", "快捷键")
        ins("item", "  Ctrl+O  导入 PNG     Ctrl+S  导出 PNG")
        ins("item", "  Ctrl+E  导出 C 代码  Ctrl+Z  撤销")
        ins("item", "  Ctrl+Y  重做         Ctrl+A  全选")
        ins("item", "  Ctrl+C  复制         Ctrl+V  粘贴")
        ins("item", "  ↑↓←→   移动选区     Space   旋转选区")

        text.config(state=tk.DISABLED)
        auto_size(win, pad_x=30, pad_y=16)
        win.minsize(480, 360)
        center_dialog(win, win.master)

    def select_all(self):
        self._save_state()
        for y in range(ICON_H):
            for x in range(ICON_W):
                self.pixels[y][x] = 1
        self._render()
        self._update_status("已全选(填充)")

    def clear_all(self):
        self._save_state()
        self.pixels = new_pixels(0)
        self._render()
        self._update_status("已清空")

    def copy(self):
        self.clipboard = [row[:] for row in self.pixels]
        self._update_status("已复制")

    def paste(self):
        if self.clipboard is None:
            return
        self._save_state()
        self.pixels = [row[:] for row in self.clipboard]
        self._render()
        self._update_status("已粘贴")

    def reset(self):
        if messagebox.askyesno("重置", "确定要重置为空白图标吗？"):
            self.pixels = new_pixels(0)
            self.history.clear()
            self._render()
            self._update_status("已重置")

    # ── 导入导出 ────────────────────────────────────────────────

    def import_png(self):
        Image = _ensure_pil()
        if not Image:
            return
        path = filedialog.askopenfilename(
            title="导入 PNG 图片",
            filetypes=[("PNG 图片", "*.png"), ("所有文件", "*.*")])
        if not path:
            return
        try:
            tmp_img = Image.open(path)
            w, h = tmp_img.size
            del tmp_img

            mode = "fit"
            scale_pct = 100
            if w != ICON_W or h != ICON_H:
                win = tk.Toplevel(self.root)
                win.title("选择导入方式")
                win.resizable(False, False)
                win.transient(self.root)
                win.grab_set()

                # ── 左: 选项 ──
                left_pane = ttk.Frame(win, padding=(10, 8))
                left_pane.pack(side=tk.LEFT, fill=tk.Y)

                ttk.Label(left_pane, text=f"图片尺寸 {w}×{h}",
                          font=("", 10)).pack(anchor=tk.W, pady=(0, 6))

                mode_var = tk.StringVar(value="fit")

                ttk.Radiobutton(left_pane, text=f"缩放到 {ICON_W}×{ICON_H}", variable=mode_var, value="fit").pack(anchor=tk.W, pady=2)
                ttk.Radiobutton(left_pane, text=f"从中心裁剪到 {ICON_W}×{ICON_H}", variable=mode_var, value="crop").pack(anchor=tk.W, pady=2)
                ttk.Radiobutton(left_pane, text="自定义缩放比例:", variable=mode_var, value="scale").pack(anchor=tk.W, pady=2)

                scale_frame = ttk.Frame(left_pane)
                scale_frame.pack(padx=16, fill=tk.X)

                scale_var = tk.DoubleVar(value=100)
                scale_slider = ttk.Scale(scale_frame, from_=5, to=300,
                                         variable=scale_var, orient=tk.HORIZONTAL, length=160)
                scale_slider.pack(side=tk.LEFT, padx=(0, 6))

                scale_label = ttk.Label(scale_frame, text="100%", width=6, anchor=tk.W)
                scale_label.pack(side=tk.LEFT)

                def update_scale_label(*args):
                    scale_label.config(text=f"{scale_var.get():.0f}%")
                scale_var.trace("w", update_scale_label)

                def disable_scale(*args):
                    state = tk.NORMAL if mode_var.get() == "scale" else tk.DISABLED
                    scale_slider.configure(state=state)
                mode_var.trace("w", disable_scale)
                disable_scale()

                ttk.Label(left_pane, text="原图尺寸 → 结果尺寸",
                          font=("", 9)).pack(pady=(10, 2))
                size_label = ttk.Label(left_pane, text=f"{w}×{h} → {ICON_W}×{ICON_H}",
                                       font=("Consolas", 10))
                size_label.pack()

                result = [None, None, None]

                def do_ok():
                    result[0] = mode_var.get()
                    if result[0] == "scale":
                        result[1] = scale_var.get()
                    win.destroy()

                def do_cancel():
                    win.destroy()

                btn_frame = ttk.Frame(left_pane)
                btn_frame.pack(pady=(16, 4))
                ttk.Button(btn_frame, text="确定", command=do_ok, width=10).pack(side=tk.LEFT, padx=4)
                ttk.Button(btn_frame, text="取消", command=do_cancel, width=10).pack(side=tk.LEFT, padx=4)

                # ── 右: 预览 ──
                right_pane = ttk.Frame(win, padding=(0, 8, 10, 8))
                right_pane.pack(side=tk.RIGHT, fill=tk.BOTH, expand=True)

                pv_frame = ttk.LabelFrame(right_pane, text=f" 预览 ({ICON_W}×{ICON_H}) ", padding=6)
                pv_frame.pack(fill=tk.BOTH, expand=True)

                PV_CELL = 10
                pv_cv = tk.Canvas(pv_frame, width=ICON_W*PV_CELL+2, height=ICON_H*PV_CELL+2,
                                  bg="#F0F0F0", highlightthickness=1,
                                  highlightbackground="#C0C0C0", relief=tk.FLAT)
                pv_cv.pack()

                info_frame = ttk.Frame(pv_frame)
                info_frame.pack(fill=tk.X, pady=(6, 0))
                black_var = tk.StringVar(value="黑: 0")
                white_var = tk.StringVar(value="白: 0")
                ttk.Label(info_frame, textvariable=black_var,
                          font=("Consolas", 9), foreground="#333").pack(side=tk.LEFT, padx=(0, 12))
                ttk.Label(info_frame, textvariable=white_var,
                          font=("Consolas", 9), foreground="#333").pack(side=tk.LEFT)

                def render_preview(*args):
                    m = mode_var.get()
                    sp = scale_var.get() if m == "scale" else 100
                    pv_cv.delete("all")
                    try:
                        pil_img = Image.open(path)
                        pp = preview_from_img(pil_img, mode=m, scale_pct=sp)
                        black_cnt = 0
                        white_cnt = 0
                        for py in range(ICON_H):
                            for px in range(ICON_W):
                                v = pp[py][px]
                                color = PALETTE[v]
                                if v:
                                    black_cnt += 1
                                else:
                                    white_cnt += 1
                                x1 = px * PV_CELL + 1
                                y1 = py * PV_CELL + 1
                                pv_cv.create_rectangle(x1, y1, x1+PV_CELL, y1+PV_CELL,
                                                       fill=color, outline="#D0D0D0", width=1)
                        black_var.set(f"黑: {black_cnt}")
                        white_var.set(f"白: {white_cnt}")
                        if m == "scale":
                            size_label.config(text=f"{w}×{h} → 缩放 {sp:.0f}% → {ICON_W}×{ICON_H}")
                        elif m == "crop":
                            size_label.config(text=f"{w}×{h} → 居中裁剪 → {ICON_W}×{ICON_H}")
                        else:
                            size_label.config(text=f"{w}×{h} → 直接缩放 → {ICON_W}×{ICON_H}")
                    except Exception:
                        pass

                mode_var.trace("w", render_preview)
                scale_var.trace("w", render_preview)
                render_preview()

                auto_size(win, pad_x=20, pad_y=10)
                center_dialog(win, win.master)

                self.root.wait_window(win)

                if result[0] is None:
                    return
                mode = result[0]
                if mode == "scale":
                    scale_pct = result[1]

            self._save_state()
            if mode == "fit":
                self.pixels = pixels_from_png(path, scale_to_fit=True)
            elif mode == "crop":
                self.pixels = pixels_from_png(path, scale_to_fit=False)
            else:
                self.pixels = pixels_from_png(path, scale_factor=scale_pct / 100.0)
            self._render()
            self._update_status(f"已导入: {os.path.basename(path)} ({w}×{h})")
        except Exception as e:
            messagebox.showerror("导入失败", str(e))

    def export_png(self):
        Image = _ensure_pil()
        if not Image:
            return
        scale = self._choose_export_scale()
        if scale is None:
            return
        path = filedialog.asksaveasfilename(
            title="导出 PNG 图片",
            defaultextension=".png",
            filetypes=[("PNG 图片", "*.png")])
        if not path:
            return
        try:
            pixels_to_png(self.pixels, path, scale=scale)
            self._update_status(f"已导出: {os.path.basename(path)} ({scale}x)")
        except Exception as e:
            messagebox.showerror("导出失败", str(e))

    def _refresh_pillow_item(self):
        try:
            from PIL import Image
            has = True
        except ImportError:
            has = False
        label = "卸载 Pillow 模块..." if has else "安装 Pillow 模块..."
        try:
            self._tool_menu.entryconfig(self._pillow_item_idx, label=label)
        except Exception:
            pass

    def _pillow_action(self):
        try:
            from PIL import Image
            installed = True
        except ImportError:
            installed = False

        if installed:
            if not messagebox.askyesno("确认卸载",
                "确定要卸载 Pillow 模块吗？\n\n卸载后将无法导入/导出 PNG，\n但其他功能不受影响。"):
                return
            pip = _find_pip()
            result = _pip_window("卸载 Pillow", [pip, "uninstall", "Pillow", "-y"])
            if result["done"] and result["ok"]:
                messagebox.showinfo("卸载完成", "Pillow 模块已成功卸载。")
            elif result["done"]:
                messagebox.showerror("卸载失败",
                    f"卸载 Pillow 失败。\n\n错误信息:\n{result['text']}")
        else:
            ret = messagebox.askyesno(
                "安装 Pillow 模块",
                "导入/导出 PNG 需要 Pillow 模块。\n\n是否自动安装？"
            )
            if not ret:
                return
            pip = _find_pip()
            result = _pip_window("安装 Pillow", [pip, "install", "Pillow"])
            if result["done"] and result["ok"]:
                try:
                    from PIL import Image as _2
                    messagebox.showinfo("安装成功", "Pillow 模块已成功安装。")
                except ImportError:
                    messagebox.showerror("安装失败",
                        f"自动安装 Pillow 失败。\n\n请手动运行:\n  \"{pip}\" install Pillow")
            elif result["done"]:
                messagebox.showerror("安装失败",
                    f"自动安装 Pillow 失败。\n\n错误信息:\n{result['text']}")
        self._update_status("")

    def _resize_canvas(self, preset=None):
        g = globals()
        old_size = g['ICON_W']

        if preset is not None:
            new_size = preset
        else:
            win = tk.Toplevel(self.root)
            win.title("调整画布大小")
            win.resizable(False, False)
            win.transient(self.root)
            win.grab_set()

            ttk.Label(win, text="画布边长（像素）:", font=("", 10)).pack(padx=24, pady=(14, 4))
            ttk.Label(win, text="画布始终保持正方形", font=("", 9), foreground="#888").pack()

            var = tk.IntVar(value=old_size)
            spin = ttk.Spinbox(win, from_=8, to=256, width=6,
                               textvariable=var, font=("Consolas", 12))
            spin.pack(pady=6)

            info = ttk.Label(win, font=("", 9), foreground="#666")
            info.pack()

            def on_change(*_):
                v = var.get()
                info.config(text=f"尺寸: {v}×{v}  ({v*v} 像素)  存储: {((v+7)//8)*v} 字节")
            var.trace("w", on_change)
            on_change()

            btn_frame = ttk.Frame(win)
            btn_frame.pack(pady=(10, 14))

            result = [None]
            def do_ok():
                v = var.get()
                if v < 1:
                    return
                result[0] = v
                win.destroy()
            def do_cancel():
                win.destroy()

            ttk.Button(btn_frame, text="确定", command=do_ok, width=10).pack(side=tk.LEFT, padx=6)
            ttk.Button(btn_frame, text="取消", command=do_cancel, width=10).pack(side=tk.LEFT, padx=6)

            auto_size(win, pad_x=24, pad_y=16)
            center_dialog(win, win.master)
            self.root.wait_window(win)
            if result[0] is None:
                return
            new_size = result[0]

        if new_size == old_size:
            return

        if not messagebox.askyesno("确认调整",
            f"将画布从 {old_size}×{old_size} 调整为 {new_size}×{new_size}。\n\n"
            f"{'原图将居中放置' if new_size > old_size else '将从中心裁剪原图'}。\n"
            "确认继续？"):
            return

        offset = (new_size - old_size) // 2
        new_px = [[0] * new_size for _ in range(new_size)]
        for y in range(old_size):
            for x in range(old_size):
                ny = y + offset
                nx = x + offset
                if 0 <= ny < new_size and 0 <= nx < new_size:
                    new_px[ny][nx] = self.pixels[y][x]

        self._save_state()
        self.pixels = new_px

        g['ICON_W'] = new_size
        g['ICON_H'] = new_size
        g['BYTES_PER_ROW'] = (new_size + 7) // 8
        g['CANVAS_W'] = new_size * g['CELL_SIZE']
        g['CANVAS_H'] = new_size * g['CELL_SIZE']

        self.has_selection = False
        self.sel_rect_id = None

        if self.preview_win is not None and self.preview_win.winfo_exists():
            self.preview_win.destroy()
            self.preview_win = None
            self.preview_cv = None

        self.cv.config(scrollregion=(0, 0, CANVAS_W + 2, CANVAS_H + 2))
        self.root.title(f"WouoUI 图标编辑器 - {new_size}×{new_size}")
        self._render()
        self._update_status(f"画布已调整为 {new_size}×{new_size}")

    def _choose_export_scale(self):
        win = tk.Toplevel(self.root)
        win.title("选择导出倍率")
        win.resizable(False, False)
        win.transient(self.root)
        win.grab_set()

        ttk.Label(win, text="放大倍数:", font=("", 10)).pack(pady=(14, 4))

        scale_var = tk.IntVar(value=4)

        slider_frame = ttk.Frame(win)
        slider_frame.pack()
        ttk.Label(slider_frame, text="1×").pack(side=tk.LEFT, padx=2)
        slider = ttk.Scale(slider_frame, from_=1, to=16, orient=tk.HORIZONTAL,
                           variable=scale_var, length=200)
        slider.pack(side=tk.LEFT, padx=6)
        ttk.Label(slider_frame, text="16×").pack(side=tk.LEFT, padx=2)

        value_label = ttk.Label(win, text=f"4× (图片尺寸 {ICON_W}×{ICON_H} → {ICON_W*4}×{ICON_H*4})",
                                font=("Consolas", 10))
        value_label.pack(pady=(6, 2))

        def update_label(*args):
            s = scale_var.get()
            value_label.config(text=f"{s}× ({ICON_W}×{ICON_H} → {ICON_W*s}×{ICON_H*s})")
        scale_var.trace("w", update_label)

        result = [None]

        def do_ok():
            result[0] = scale_var.get()
            win.destroy()

        def do_cancel():
            win.destroy()

        btn_frame = ttk.Frame(win)
        btn_frame.pack(pady=(10, 8))
        ttk.Button(btn_frame, text="确定", command=do_ok, width=10).pack(side=tk.LEFT, padx=6)
        ttk.Button(btn_frame, text="取消", command=do_cancel, width=10).pack(side=tk.LEFT, padx=6)

        auto_size(win, pad_x=24, pad_y=16)
        center_dialog(win, win.master)

        self.root.wait_window(win)
        return result[0]

    def import_c_code(self):
        path = filedialog.askopenfilename(
            title="导入 C 代码",
            filetypes=[("C 文件", "*.c *.cpp *.h"), ("文本文件", "*.txt"), ("所有文件", "*.*")])
        if not path:
            return
        try:
            with open(path) as f:
                text = f.read()
            data = self._parse_c_array(text)
            if data and len(data) == ICON_H * BYTES_PER_ROW:
                self._save_state()
                self.pixels = decode_c_bytes(data)
                self._render()
                self._update_status(f"已导入 C 代码: {os.path.basename(path)}")
            else:
                expected = ICON_H * BYTES_PER_ROW
                got = len(data) if data else 0
                hint = ""
                if data:
                    inferred = infer_square_size(len(data))
                    if inferred is not None:
                        hint = f"\n数据 {got} 字节, 推测为 {inferred}×{inferred} 图标"
                messagebox.showerror("导入失败",
                    f"数据尺寸不匹配\n\n"
                    f"当前画布: {ICON_W}×{ICON_H}  (需要 {expected} 字节)\n"
                    f"导入数据: {got} 字节{hint}")
        except Exception as e:
            messagebox.showerror("导入失败", str(e))

    def _parse_c_array(self, text):
        import re
        m = re.search(r'\{([^}]+)\}', text, re.DOTALL)
        body = m.group(1) if m else text
        hex_pairs = re.findall(r'0x([0-9A-Fa-f]{2})', body)
        if not hex_pairs:
            hex_pairs = re.findall(r'\b([0-9A-Fa-f]{2})\b', body)
        return [int(x, 16) for x in hex_pairs]

    def export_c_code(self):
        path = filedialog.asksaveasfilename(
            title="导出 C 代码",
            defaultextension=".h",
            filetypes=[("C 头文件", "*.h"), ("C 源文件", "*.cpp"), ("所有文件", "*.*")])
        if not path:
            return
        try:
            data = encode_pixels(self.pixels)
            name = os.path.splitext(os.path.basename(path))[0]
            code = format_c_array(data, name)
            with open(path, "w") as f:
                f.write(code)
            self._update_status(f"已导出: {os.path.basename(path)}")
        except Exception as e:
            messagebox.showerror("导出失败", str(e))

    def export_raw_hex(self):
        path = filedialog.asksaveasfilename(
            title="导出原始十六进制",
            defaultextension=".txt",
            filetypes=[("文本文件", "*.txt"), ("所有文件", "*.*")])
        if not path:
            return
        try:
            data = encode_pixels(self.pixels)
            hex_str = format_raw_hex(data)
            with open(path, "w") as f:
                f.write(hex_str)
            self._update_status(f"已导出: {os.path.basename(path)}")
        except Exception as e:
            messagebox.showerror("导出失败", str(e))

    def show_c_code(self):
        from tkinter.scrolledtext import ScrolledText
        win = tk.Toplevel(self.root)
        win.title("代码预览")
        win.geometry("640x420")
        win.transient(self.root)
        win.resizable(False, False)

        data = encode_pixels(self.pixels)

        toolbar = ttk.Frame(win)
        toolbar.pack(fill=tk.X, padx=6, pady=(6, 0))

        mode = tk.StringVar(value="c")

        def _update_view(*_):
            if mode.get() == "c":
                content = format_c_array(data, "main_icon_pic")
            else:
                content = format_raw_hex(data)
            text.delete("1.0", tk.END)
            text.insert(tk.END, content)
            win.title("代码预览 — " + ("C 语言格式" if mode.get() == "c" else "原始十六进制"))

        ttk.Radiobutton(toolbar, text="C 语言格式", variable=mode,
                        value="c", command=_update_view).pack(side=tk.LEFT, padx=(0, 8))
        ttk.Radiobutton(toolbar, text="原始十六进制", variable=mode,
                        value="hex", command=_update_view).pack(side=tk.LEFT)

        text = ScrolledText(win, font=("Consolas", 10), wrap=tk.NONE,
                            height=18)
        text.pack(fill=tk.BOTH, expand=True, padx=6, pady=6)
        text.insert(tk.END, format_c_array(data, "main_icon_pic"))
        text.config(state=tk.NORMAL)

        def copy_all():
            content = text.get("1.0", tk.END).rstrip()
            win.clipboard_clear()
            win.clipboard_append(content)
            self._update_status("代码已复制到剪贴板")

        ttk.Button(win, text="复制到剪贴板", command=copy_all).pack(pady=(0, 6))

    def show_preview_window(self):
        if self.preview_win is not None and self.preview_win.winfo_exists():
            self._refresh_preview()
            self.preview_win.lift()
            return

        pv_cell = min(8, max(2, 400 // max(ICON_W, ICON_H)))
        win = tk.Toplevel(self.root)
        win.title(f"图标预览 ({pv_cell}×)")
        win.resizable(False, False)
        win.transient(self.root)

        cv = tk.Canvas(win, width=ICON_W * pv_cell, height=ICON_H * pv_cell,
                       bg="#FFFFFF", highlightthickness=0)
        cv.pack(padx=8, pady=8)

        self.preview_win = win
        self.preview_cv = cv
        self._preview_cell = pv_cell

        def on_close():
            self.preview_win = None
            self.preview_cv = None
            win.destroy()

        win.protocol("WM_DELETE_WINDOW", on_close)
        self._refresh_preview()

    def _refresh_preview(self):
        if self.preview_win is None or not self.preview_win.winfo_exists():
            return
        if self.preview_cv is None:
            return
        cv = self.preview_cv
        pv_cell = getattr(self, "_preview_cell", 8)
        cv.delete("all")
        for y in range(ICON_H):
            for x in range(ICON_W):
                color = PALETTE[self.pixels[y][x]]
                px, py = x * pv_cell, y * pv_cell
                cv.create_rectangle(px, py, px + pv_cell, py + pv_cell,
                                    fill=color, outline=color, width=1)

    # ── 框选操作 ────────────────────────────────────────────────

    def _clear_selection(self):
        if self.sel_rect_id is not None:
            self.cv.delete(self.sel_rect_id)
            self.sel_rect_id = None
        self.has_selection = False

    def _draw_selection(self):
        if self.sel_rect_id is not None:
            self.cv.delete(self.sel_rect_id)
        xa, ya = min(self.sel_x1, self.sel_x2), min(self.sel_y1, self.sel_y2)
        xb, yb = max(self.sel_x1, self.sel_x2), max(self.sel_y1, self.sel_y2)
        self.sel_rect_id = self.cv.create_rectangle(
            xa*CELL_SIZE+1, ya*CELL_SIZE+1,
            xb*CELL_SIZE+1+CELL_SIZE, yb*CELL_SIZE+1+CELL_SIZE,
            outline="#0066FF", width=2, dash=(4, 3))

    def _move_selection(self, dx, dy):
        if not self.has_selection:
            return
        w = abs(self.sel_x2 - self.sel_x1)
        h = abs(self.sel_y2 - self.sel_y1)
        nx1 = self.sel_x1 + dx
        ny1 = self.sel_y1 + dy
        nx2 = nx1 + w
        ny2 = ny1 + h
        if nx1 < 0 or ny1 < 0 or nx2 >= ICON_W or ny2 >= ICON_H:
            return
        self._save_state()
        region = []
        for dy_off in range(h + 1):
            row = []
            for dx_off in range(w + 1):
                row.append(self.pixels[self.sel_y1 + dy_off][self.sel_x1 + dx_off])
            region.append(row)
        bg = self.current_color ^ 1
        for dy_off in range(h + 1):
            for dx_off in range(w + 1):
                self.pixels[self.sel_y1 + dy_off][self.sel_x1 + dx_off] = bg
        for dy_off in range(h + 1):
            for dx_off in range(w + 1):
                self.pixels[ny1 + dy_off][nx1 + dx_off] = region[dy_off][dx_off]
        self.sel_x1, self.sel_y1 = nx1, ny1
        self.sel_x2, self.sel_y2 = nx2, ny2
        self._render()
        self._draw_selection()
        self._update_status(f"框选已移动至 ({nx1},{ny1})-({nx2},{ny2})")

    def _rotate_selection(self, clockwise=True):
        if not self.has_selection:
            return
        xa = min(self.sel_x1, self.sel_x2)
        ya = min(self.sel_y1, self.sel_y2)
        xb = max(self.sel_x1, self.sel_x2)
        yb = max(self.sel_y1, self.sel_y2)
        w = xb - xa
        h = yb - ya
        if w < 0 or h < 0:
            return

        region = []
        for y in range(ya, yb + 1):
            row = []
            for x in range(xa, xb + 1):
                row.append(self.pixels[y][x])
            region.append(row)

        if clockwise:
            rotated = [[0] * (h + 1) for _ in range(w + 1)]
            for orig_y in range(h + 1):
                for orig_x in range(w + 1):
                    rotated[orig_x][h - orig_y] = region[orig_y][orig_x]
        else:
            rotated = [[0] * (h + 1) for _ in range(w + 1)]
            for orig_y in range(h + 1):
                for orig_x in range(w + 1):
                    rotated[w - orig_x][orig_y] = region[orig_y][orig_x]

        self._save_state()

        bg = self.current_color ^ 1
        for y in range(ya, yb + 1):
            for x in range(xa, xb + 1):
                self.pixels[y][x] = bg

        new_w = h
        new_h = w
        cx = (xa + xb) // 2
        cy = (ya + yb) // 2
        nx1 = cx - new_w // 2
        ny1 = cy - new_h // 2
        nx2 = nx1 + new_w
        ny2 = ny1 + new_h

        if nx1 < 0 or ny1 < 0 or nx2 >= ICON_W or ny2 >= ICON_H:
            self._update_status("旋转后超出边界，已撤销")
            for y in range(ya, yb + 1):
                for x in range(xa, xb + 1):
                    self.pixels[y][x] = region[y - ya][x - xa]
            self._render()
            return

        for y in range(new_h + 1):
            for x in range(new_w + 1):
                self.pixels[ny1 + y][nx1 + x] = rotated[y][x]

        self.sel_x1, self.sel_y1 = nx1, ny1
        self.sel_x2, self.sel_y2 = nx2, ny2
        self._render()
        self._draw_selection()
        dir_str = "顺时针" if clockwise else "逆时针"
        self._update_status(f"框选已{dir_str}旋转 ({nx1},{ny1})-({nx2},{ny2})")

    # ── 渲染 ────────────────────────────────────────────────────

    def _render(self):
        self.cv.delete("all")
        self.cv.config(scrollregion=(0, 0, CANVAS_W + 2, CANVAS_H + 2))
        for y in range(ICON_H):
            for x in range(ICON_W):
                color = PALETTE[self.pixels[y][x]]
                px = x * CELL_SIZE + 1
                py = y * CELL_SIZE + 1
                self.cv.create_rectangle(px, py, px+CELL_SIZE, py+CELL_SIZE,
                                         fill=color, outline="#CCCCCC", width=1)
        if self.has_selection:
            self._draw_selection()
        self._refresh_preview()

    def _update_status(self, msg):
        if msg:
            self.status_var.set(msg)
        else:
            xy = ""
            try:
                sel = self.cv.tk.call(self.cv, "index", "current")
                mx, my = self.cv.canvasx(0), self.cv.canvasy(0)
            except:
                pass

    def run(self):
        self.root.mainloop()


if __name__ == "__main__":
    app = IconEditor()
    app.run()
