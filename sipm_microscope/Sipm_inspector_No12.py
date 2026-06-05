# =============================================================================
#  Yale SiPM Automated Tester & Velmex Controller  —  FULL STABLE VERSION
# =============================================================================

import tkinter as tk
from tkinter import filedialog, messagebox, scrolledtext
import ttkbootstrap as tb
from ttkbootstrap.constants import *
import serial
import threading
import time
import os
import json
import cv2
import usb.core
from PIL import Image, ImageTk

class UnifiedSiPMApp:
    def __init__(self, root):
        self.root = root
        self.root.title("Yale SiPM Automated Tester & Velmex Controller")
        self.root.geometry("1500x900") 
        
        # --- VELMEX STATE ---
        self.ser = None
        self.is_connected = False
        self.motion_event = threading.Event()
        
        # --- POSITION TRACKING ---
        self.current_x = 0
        self.current_y = 0
        
        # --- CALIBRATION & NAMED HOMES ---
        self.calib_file = "calibration.json"
        self.calib = {
            'x_min': None, 'x_max': None, 
            'y_min': None, 'y_max': None, 
            'named_homes': {} 
        }
        self.load_calibration_file()
        
        # --- TEST & CAMERA STATE ---
        self.is_running = False
        self.cancel_flag = False
        self.saved_positions = [] 
        self.cap = None
        self.camera_active = False 
        self.camera_lock = threading.Lock() 
        self.latest_frame = None            
        
        # Grid parameters (460 positions)
        self.GRID_COLS = 23
        self.GRID_ROWS = 20
        self.grid_colors = ["#444444"] * (self.GRID_COLS * self.GRID_ROWS)
        self.grid_rects = {}
        
        # --- DIRECTORY MANAGEMENT ---
        self.base_dir = os.path.abspath("SiPM_Images")
        self.save_dir = os.path.join(self.base_dir, "Tray_001")
        if not os.path.exists(self.save_dir):
            os.makedirs(self.save_dir)
            
        self.create_widgets()

    # --- FILE I/O ---
    def load_calibration_file(self):
        if os.path.exists(self.calib_file):
            try:
                with open(self.calib_file, 'r') as f:
                    data = json.load(f)
                    self.calib.update(data)
                if 'named_homes' not in self.calib:
                    self.calib['named_homes'] = {}
            except Exception as e:
                print(f"Error loading calibration file: {e}")

    def save_calibration_file(self):
        try:
            with open(self.calib_file, 'w') as f:
                json.dump(self.calib, f, indent=4)
            self.log("Calibration data auto-saved successfully.")
        except Exception as e:
            messagebox.showerror("Error", f"Failed to save calibration: {e}")

    # --- GUI CONSTRUCTION ---
    def create_widgets(self):
        # --- TOP: Connection ---
        conn_frame = tb.LabelFrame(self.root, text="Velmex Connection & Status")
        conn_frame.pack(padx=10, pady=10, fill="x")
        
        tb.Label(conn_frame, text="Port (e.g. COM3 or /dev/ttyUSB0):").pack(side="left", padx=5)
        self.port_entry = tb.Entry(conn_frame, width=20)
        self.port_entry.insert(0, "COM3")
        self.port_entry.pack(side="left", padx=5)
        
        self.connect_btn = tb.Button(conn_frame, text="Connect", bootstyle=PRIMARY, command=self.toggle_connection)
        self.connect_btn.pack(side="left", padx=10)
        
        self.pos_label = tb.Label(conn_frame, text="Current Pos: X=0, Y=0", font=("Arial", 12, "bold"))
        self.pos_label.pack(side="right", padx=20)

        # --- MAIN BODY SPLIT ---
        main_body = tb.Frame(self.root)
        main_body.pack(padx=10, pady=5, fill="both", expand=True)

        # LEFT SIDE: Manual Control & Logs
        left_panel = tb.Frame(main_body, width=330)
        left_panel.pack(side="left", fill="y", padx=(0, 10))

        ctrl_frame = tb.LabelFrame(left_panel, text="Manual Control & Machine Homing")
        ctrl_frame.pack(fill="x", pady=5)

        step_frame = tb.Frame(ctrl_frame)
        step_frame.pack(pady=10)
        tb.Label(step_frame, text="Step Size:").pack(side="left", padx=5)
        self.step_entry = tb.Entry(step_frame, width=12)
        self.step_entry.insert(0, "1000")
        self.step_entry.pack(side="left")

        # Directional Buttons
        btn_grid = tb.Frame(ctrl_frame)
        btn_grid.pack(pady=5)

        tb.Button(btn_grid, text="Y- (Up)", bootstyle=SECONDARY, command=lambda: self.move_axis(2, -1)).grid(row=0, column=1, pady=5)
        tb.Button(btn_grid, text="HW Home Y", bootstyle=WARNING, command=lambda: self.hardware_home(2)).grid(row=1, column=1, pady=5)
        tb.Button(btn_grid, text="Y+ (Down / Next Row)", bootstyle=SECONDARY, command=lambda: self.move_axis(2, 1)).grid(row=2, column=1, pady=5)
        
        tb.Button(btn_grid, text="X- (Left)", bootstyle=SECONDARY, command=lambda: self.move_axis(1, -1)).grid(row=1, column=0, padx=5)
        tb.Button(btn_grid, text="X+ (Right)", bootstyle=SECONDARY, command=lambda: self.move_axis(1, 1)).grid(row=1, column=2, padx=5)
        
        tb.Button(btn_grid, text="HW Home X", bootstyle=WARNING, command=lambda: self.hardware_home(1)).grid(row=2, column=0, padx=5)

        # Sync Zero Button
        tb.Button(btn_grid, text="Sync Zero (Set Current as 0,0)", bootstyle=INFO, command=self.sync_zero).grid(row=3, column=0, columnspan=3, pady=10, sticky="ew", padx=5)

        # Emergency Stop
        tb.Button(ctrl_frame, text="⛔ STOP / KILL MOTOR", bootstyle="danger-outline", command=self.kill_motion).pack(pady=5, fill="x", padx=10)

        # System Log
        log_frame = tb.LabelFrame(left_panel, text="System Log")
        log_frame.pack(fill="both", expand=True, pady=5)
        
        self.log_area = scrolledtext.ScrolledText(
            log_frame, height=10, state='disabled', 
            font=("Consolas", 9), bg="#1e1e1e", fg="#d4d4d4", insertbackground="white"
        )
        self.log_area.pack(fill="both", expand=True, padx=5, pady=5)

        # RIGHT SIDE: Notebook
        self.notebook = tb.Notebook(main_body, bootstyle=INFO)
        self.notebook.pack(side="right", fill="both", expand=True)

        self._build_tab_dashboard()
        self._build_tab_calibration()

    def _build_tab_dashboard(self):
        """1. Tab: Camera, Tracker and Controls unified"""
        tab_run = tb.Frame(self.notebook)
        self.notebook.add(tab_run, text="1. Inspection Dashboard")

        # --- DIRECTORY MANAGER (TOP) ---
        folder_frame = tb.LabelFrame(tab_run, text="Image Save Location (Folder Manager)")
        folder_frame.pack(fill="x", padx=10, pady=5)

        tb.Label(folder_frame, text="Base Path:").grid(row=0, column=0, padx=5, pady=5, sticky="e")
        self.base_path_entry = tb.Entry(folder_frame, width=40)
        self.base_path_entry.insert(0, self.base_dir)
        self.base_path_entry.grid(row=0, column=1, padx=5, pady=5, sticky="w")
        tb.Button(folder_frame, text="Browse", bootstyle=SECONDARY, command=self.browse_base_path).grid(row=0, column=2, padx=5)

        tb.Label(folder_frame, text="Tray Number / Name:").grid(row=1, column=0, padx=5, pady=5, sticky="e")
        self.tray_name_entry = tb.Entry(folder_frame, width=20)
        self.tray_name_entry.insert(0, "Tray_001")
        self.tray_name_entry.grid(row=1, column=1, sticky="w", padx=5, pady=5)
        tb.Button(folder_frame, text="Create & Set Folder", bootstyle=INFO, command=self.apply_save_folder).grid(row=1, column=2, padx=5)

        self.lbl_current_save = tb.Label(folder_frame, text=f"Current Output: {self.save_dir}", font=("Arial", 9, "italic"))
        self.lbl_current_save.grid(row=2, column=0, columnspan=3, pady=5)

        # --- ACTION FRAME (BOTTOM) ---
        action_frame = tb.Frame(tab_run)
        action_frame.pack(side="bottom", fill="x", padx=10, pady=15)
        
        self.start_btn = tb.Button(action_frame, text="START INSPECTION", bootstyle=SUCCESS, command=self.start_inspection_thread)
        self.start_btn.pack(side="left", padx=20, expand=True)
        self.stop_btn = tb.Button(action_frame, text="EMERGENCY STOP", bootstyle=DANGER, command=self.stop_process, state=tk.DISABLED)
        self.stop_btn.pack(side="right", padx=20, expand=True)

        # --- CONTENT FRAME (MIDDLE - SIDE BY SIDE) ---
        content_frame = tb.Frame(tab_run)
        content_frame.pack(fill="both", expand=True, padx=10, pady=10)

        # Left Side: Video Feed Container
        video_container = tb.Frame(content_frame)
        video_container.pack(side="left", fill="y", padx=(0, 5))
        
        self.video_frame = tb.Frame(video_container, width=640, height=480)
        self.video_frame.pack(pady=0)
        self.video_frame.pack_propagate(False) 
        
        self.video_label = tk.Label(self.video_frame, bg="black", text="No Camera Feed", fg="white", font=("Arial", 14))
        self.video_label.pack(fill="both", expand=True)
        
        self.cam_toggle_btn = tb.Button(video_container, text="Start Live View", bootstyle=INFO, command=self.toggle_live_camera)
        self.cam_toggle_btn.pack(pady=10)

        # Right Side: Live Progress Tracker Container
        tracker_container = tb.Frame(content_frame)
        tracker_container.pack(side="right", fill="both", expand=True, padx=(5, 0))

        legend_frame = tb.Frame(tracker_container)
        legend_frame.pack(fill="x", pady=(0, 5)) 
        for color, text in [("#444444", "Pending"), ("#f39c12", "Active"), ("#18bc9c", "Completed"), ("#e74c3c", "Error")]:
            f = tk.Frame(legend_frame, bg=color, width=18, height=18, bd=1, relief=tk.SOLID)
            f.pack(side="left", padx=(5, 0))
            f.pack_propagate(False)
            tb.Label(legend_frame, text=text).pack(side="left", padx=(5, 10))

        tracker_frame = tb.LabelFrame(tracker_container, text="SiPM Tray Layout (23 x 20)")
        tracker_frame.pack(fill="both", expand=True)
        
        self.grid_canvas = tk.Canvas(tracker_frame, bg="#222222", highlightthickness=0)
        self.grid_canvas.pack(fill="both", expand=True)
        self.grid_canvas.bind("<Configure>", self.draw_tray_grid)

    def _build_tab_calibration(self):
        tab_setup = tb.Frame(self.notebook)
        self.notebook.add(tab_setup, text="2. Calibration & Tray Setup")
        
        # --- NAMED HOMES & CALIBRATION ---
        calib_frame = tb.LabelFrame(tab_setup, text="Named Custom Homes & Soft Limits")
        calib_frame.pack(fill="x", pady=10, padx=10)

        tb.Label(calib_frame, text="Home Name:").grid(row=0, column=0, padx=10, pady=10, sticky="e")
        self.home_name_entry = tb.Entry(calib_frame, width=20)
        self.home_name_entry.grid(row=0, column=1, padx=5, sticky="w")
        tb.Button(calib_frame, text="Save Current Pos as Home", bootstyle=WARNING, command=self.save_named_home).grid(row=0, column=2, padx=5, sticky="w")

        tb.Label(calib_frame, text="Saved Homes:").grid(row=1, column=0, padx=10, pady=5, sticky="e")
        self.saved_homes_combo = tb.Combobox(calib_frame, state="readonly", width=18)
        self.saved_homes_combo.grid(row=1, column=1, padx=5, sticky="w")
        tb.Button(calib_frame, text="Go To Selected Home", bootstyle=PRIMARY, command=self.go_to_named_home).grid(row=1, column=2, padx=5, sticky="w")
        tb.Button(calib_frame, text="Delete", bootstyle=DANGER, command=self.delete_named_home).grid(row=1, column=3, padx=5, sticky="w")
        
        self.update_home_combo()
        tb.Separator(calib_frame, orient='horizontal').grid(row=2, column=0, columnspan=5, pady=15, sticky='ew')
        
        tb.Label(calib_frame, text="X Axis Limit:").grid(row=3, column=0, padx=10, pady=5, sticky="e")
        self.lbl_xmin = tb.Label(calib_frame, text=f"Start: {self.calib['x_min']}")
        self.lbl_xmin.grid(row=3, column=1, padx=10)
        tb.Button(calib_frame, text="Set X Start", bootstyle=SECONDARY, command=lambda: self.set_calib('x_min')).grid(row=3, column=2, padx=5)
        self.lbl_xmax = tb.Label(calib_frame, text=f"End: {self.calib['x_max']}")
        self.lbl_xmax.grid(row=3, column=3, padx=10)
        tb.Button(calib_frame, text="Set X End", bootstyle=SECONDARY, command=lambda: self.set_calib('x_max')).grid(row=3, column=4, padx=5)
        
        tb.Label(calib_frame, text="Y Axis Limit:").grid(row=4, column=0, padx=10, pady=5, sticky="e")
        self.lbl_ymin = tb.Label(calib_frame, text=f"Start: {self.calib['y_min']}")
        self.lbl_ymin.grid(row=4, column=1, padx=10)
        tb.Button(calib_frame, text="Set Y Start", bootstyle=SECONDARY, command=lambda: self.set_calib('y_min')).grid(row=4, column=2, padx=5)
        self.lbl_ymax = tb.Label(calib_frame, text=f"End: {self.calib['y_max']}")
        self.lbl_ymax.grid(row=4, column=3, padx=10)
        tb.Button(calib_frame, text="Set Y End", bootstyle=SECONDARY, command=lambda: self.set_calib('y_max')).grid(row=4, column=4, padx=5)
        
        # --- TRAY SETUP ---
        tray_frame = tb.LabelFrame(tab_setup, text="Tray Test Setup (460 Positions)")
        tray_frame.pack(fill="both", expand=True, pady=5)
        
        list_frame = tb.Frame(tray_frame)
        list_frame.pack(side="left", fill="both", expand=True, padx=5, pady=5)
        
        self.pos_listbox = tk.Listbox(list_frame, width=35, bg="#2b2b2b", fg="white", selectbackground="#0078D7", font=("Consolas", 10))
        self.pos_listbox.pack(side="left", fill="both", expand=True)
        scrollbar = tb.Scrollbar(list_frame, orient="vertical", command=self.pos_listbox.yview)
        scrollbar.pack(side="right", fill="y")
        self.pos_listbox.config(yscrollcommand=scrollbar.set)

        btn_frame = tb.Frame(tray_frame)
        btn_frame.pack(side="right", fill="y", padx=20, pady=10)

        tb.Button(btn_frame, text="Record Current Pos", bootstyle=INFO, command=self.record_position).pack(fill="x", pady=5)
        tb.Button(btn_frame, text="Clear Point List", bootstyle=DANGER, command=self.clear_positions).pack(fill="x", pady=5)
        tb.Button(btn_frame, text="Save Points (JSON)", bootstyle=SECONDARY, command=self.save_setup).pack(fill="x", pady=5)
        tb.Button(btn_frame, text="Load Points (JSON)", bootstyle=SECONDARY, command=self.load_setup).pack(fill="x", pady=5)
        
        tb.Separator(btn_frame, orient='horizontal').pack(fill='x', pady=15)
        tb.Label(btn_frame, text="Auto-Grid (X: 3.8mm, Y: 4.3mm spacing)", font=("Arial", 10, "bold")).pack()
        
        grid_step_frame = tb.Frame(btn_frame)
        grid_step_frame.pack(fill="x", pady=5)
        
        tb.Label(grid_step_frame, text="Steps / 1 mm (X):").pack(side="left")
        self.steps_per_mm_x_entry = tb.Entry(grid_step_frame, width=8)
        self.steps_per_mm_x_entry.insert(0, "400") 
        self.steps_per_mm_x_entry.pack(side="left", padx=5)
        
        tb.Label(grid_step_frame, text="Steps / 1 mm (Y):").pack(side="left", padx=(10,0))
        self.steps_per_mm_y_entry = tb.Entry(grid_step_frame, width=8)
        self.steps_per_mm_y_entry.insert(0, "400") 
        self.steps_per_mm_y_entry.pack(side="left", padx=5)

        tb.Button(btn_frame, text="Generate 460 Points", bootstyle=SUCCESS, command=self.generate_grid).pack(fill="x", pady=10)

    # --- FOLDER MANAGEMENT ---
    def browse_base_path(self):
        d = filedialog.askdirectory(initialdir=self.base_path_entry.get())
        if d:
            self.base_path_entry.delete(0, tk.END)
            self.base_path_entry.insert(0, d)
            self.apply_save_folder()

    def apply_save_folder(self):
        base = self.base_path_entry.get()
        tray = self.tray_name_entry.get()
        full_path = os.path.join(base, tray)
        try:
            os.makedirs(full_path, exist_ok=True)
            self.save_dir = full_path
            self.lbl_current_save.config(text=f"Current Output: {os.path.abspath(self.save_dir)}")
            self.log(f"Save directory configured: {self.save_dir}")
        except Exception as e:
            messagebox.showerror("Folder Error", f"Cannot create folder: {e}")

    # --- NAMED HOMES MANAGEMENT ---
    def update_home_combo(self):
        homes = list(self.calib.get('named_homes', {}).keys())
        self.saved_homes_combo['values'] = homes
        if homes:
            self.saved_homes_combo.current(0)
        else:
            self.saved_homes_combo.set('')

    def save_named_home(self):
        name = self.home_name_entry.get().strip()
        if not name:
            messagebox.showwarning("Input Error", "Please provide a name for the Home position.")
            return
        self.calib['named_homes'][name] = {'x': self.current_x, 'y': self.current_y}
        self.save_calibration_file()
        self.update_home_combo()
        self.saved_homes_combo.set(name)
        self.log(f"Named Home '{name}' saved at X:{self.current_x}, Y:{self.current_y}.")

    def go_to_named_home(self):
        name = self.saved_homes_combo.get()
        if not name or name not in self.calib['named_homes']:
            messagebox.showwarning("Selection Error", "Please select a valid Named Home.")
            return
        target = self.calib['named_homes'][name]
        dx = target['x'] - self.current_x
        dy = target['y'] - self.current_y
        self.log(f"Moving to Named Home '{name}'...")
        self.send_command("C")
        time.sleep(0.1)
        cmd = ""
        if dx != 0: cmd += f"I1M{dx}, "
        if dy != 0: cmd += f"I2M{dy}, "
        if cmd: self.send_command(cmd + "R")
        self.current_x = target['x']
        self.current_y = target['y']
        self.update_ui_labels()

    def delete_named_home(self):
        name = self.saved_homes_combo.get()
        if name in self.calib['named_homes']:
            del self.calib['named_homes'][name]
            self.save_calibration_file()
            self.update_home_combo()
            self.log(f"Named Home '{name}' deleted.")

    def sync_zero(self):
        self.current_x = 0
        self.current_y = 0
        self.update_ui_labels()
        self.log("Synchronized! Current physical position is now Software 0,0.")

    # --- PROGRESS TRACKER LOGIC ---
    def draw_tray_grid(self, event=None):
        self.grid_canvas.delete("all")
        self.grid_rects = {}
        cw = self.grid_canvas.winfo_width()
        ch = self.grid_canvas.winfo_height()
        if cw < 50 or ch < 50: return
        cols = self.GRID_COLS
        rows = self.GRID_ROWS
        pad = 3
        margin_x = 20
        margin_y = 20
        box_w = (cw - 2 * margin_x - (cols - 1) * pad) // cols
        box_h = (ch - 2 * margin_y - (rows - 1) * pad) // rows
        start_x = (cw - (cols * box_w + (cols - 1) * pad)) // 2
        start_y = (ch - (rows * box_h + (rows - 1) * pad)) // 2

        for i in range(cols * rows):
            r = i // cols
            c = i % cols
            x0 = start_x + c * (box_w + pad)
            y0 = start_y + r * (box_h + pad)
            rect_id = self.grid_canvas.create_rectangle(
                x0, y0, x0 + box_w, y0 + box_h, 
                fill=self.grid_colors[i], outline="#111111", width=1
            )
            self.grid_rects[i] = rect_id

    def update_grid_color(self, index, color):
        if 0 <= index < len(self.grid_colors): 
            self.grid_colors[index] = color
        if index in self.grid_rects: 
            self.grid_canvas.itemconfig(self.grid_rects[index], fill=color)

    # --- UI HELPERS ---
    def log(self, message):
        self.log_area.config(state='normal')
        self.log_area.insert(tk.END, message + "\n")
        self.log_area.see(tk.END)
        self.log_area.config(state='disabled')

    def update_ui_labels(self):
        self.pos_label.config(text=f"Current Pos: X={self.current_x}, Y={self.current_y}", foreground="white")
        self.lbl_xmin.config(text=f"Start: {self.calib['x_min']}")
        self.lbl_xmax.config(text=f"End: {self.calib['x_max']}")
        self.lbl_ymin.config(text=f"Start: {self.calib['y_min']}")
        self.lbl_ymax.config(text=f"End: {self.calib['y_max']}")

    # --- SOFTWARE LIMITS ---
    def check_software_limits(self, target_x, target_y):
        if self.calib['x_min'] is not None and self.calib['x_max'] is not None:
            x_low = min(self.calib['x_min'], self.calib['x_max'])
            x_high = max(self.calib['x_min'], self.calib['x_max'])
            if not (x_low <= target_x <= x_high): return False, f"X={target_x} out of range"

        if self.calib['y_min'] is not None and self.calib['y_max'] is not None:
            y_low = min(self.calib['y_min'], self.calib['y_max'])
            y_high = max(self.calib['y_min'], self.calib['y_max'])
            if not (y_low <= target_y <= y_high): return False, f"Y={target_y} out of range"
        return True, "OK"

    def set_calib(self, key):
        val = self.current_x if 'x' in key else self.current_y
        self.calib[key] = val
        self.update_ui_labels()
        self.save_calibration_file() 
        self.log(f"Calibration limit '{key}' set to {val} and saved.")

    # --- VELMEX COMMUNICATION & MOVEMENT ---
    def toggle_connection(self):
        if not self.is_connected:
            port = self.port_entry.get()
            try:
                self.ser = serial.Serial(port, 57600, timeout=0.1)
                self.is_connected = True
                self.connect_btn.config(text="Disconnect", bootstyle=DANGER)
                self.log(f"Connected to: {port}")
                
                self.send_command("F") 
                time.sleep(0.1)
                self.send_command("C") 
                time.sleep(0.1)
                
                self.send_command("S1M700, S2M700, A1M3, A2M3, R")
                self.log("Soft motor dynamics applied automatically.")
                
                self.read_thread = threading.Thread(target=self.read_from_port, daemon=True)
                self.read_thread.start()
            except Exception as e: 
                messagebox.showerror("Connection Error", str(e))
        else:
            self.stop_process()
            self.is_connected = False
            if self.ser and self.ser.is_open: self.ser.close()
            self.connect_btn.config(text="Connect", bootstyle=PRIMARY)
            self.log("Disconnected.")

    def send_command(self, cmd):
        if self.is_connected and self.ser:
            self.ser.write((cmd + "\r").encode('ascii'))
            self.log(f">> {cmd}")

    def send_and_wait(self, cmd):
        self.motion_event.clear()
        self.send_command("C")
        time.sleep(0.1)
        self.send_command(cmd)
        while not self.cancel_flag and self.is_connected:
            if self.motion_event.wait(timeout=0.2): return True
        return False

    def read_from_port(self):
        while self.is_connected:
            if self.ser and self.ser.is_open:
                try:
                    if self.ser.in_waiting > 0:
                        data = self.ser.read(self.ser.in_waiting)
                        if data:
                            text = data.decode('ascii', errors='ignore').strip()
                            if text:
                                if "^" in text: self.motion_event.set()
                                else: self.root.after(0, lambda t=text: self.log(f"<< {t}"))
                except Exception as e: 
                    self.root.after(0, lambda err=e: self.log(f"Serial Error: {err}"))
                    break
            time.sleep(0.05)

    def move_axis(self, motor, direction):
        try:
            steps = int(self.step_entry.get())
            if direction < 0: steps = -steps
            
            target_x = self.current_x + (steps if motor == 1 else 0)
            target_y = self.current_y + (steps if motor == 2 else 0)
            
            safe, reason = self.check_software_limits(target_x, target_y)
            if not safe:
                messagebox.showerror("Limit Reached", f"Movement blocked: {reason}")
                return

            self.send_command("C")
            time.sleep(0.1)
            self.send_command(f"I{motor}M{steps}, R")
            
            self.current_x = target_x
            self.current_y = target_y
            self.update_ui_labels()
        except ValueError:
            messagebox.showwarning("Input Error", "Step size must be an integer.")

    def hardware_home(self, motor):
        self.send_command("C")
        time.sleep(0.1)
        self.send_command(f"I{motor}M-0, R") 
        if motor == 1: self.current_x = 0
        if motor == 2: self.current_y = 0
        self.log(f"Motor {motor} homed against hardware limit. Pos is now 0.")
        self.update_ui_labels()

    def kill_motion(self):
        self.send_command("K")
        time.sleep(0.05)
        self.send_command("C")
        self.stop_process()
        self.update_ui_labels()
        self.log("MOTION KILLED! Position tracking might be inaccurate now.")

    # --- POINTS MANAGEMENT ---
    def record_position(self):
        self.saved_positions.append({"x": self.current_x, "y": self.current_y})
        self.refresh_listbox()

    def refresh_listbox(self):
        self.pos_listbox.delete(0, tk.END)
        for i, pos in enumerate(self.saved_positions): 
            self.pos_listbox.insert(tk.END, f"{i+1}. X: {pos['x']} | Y: {pos['y']}")

    def clear_positions(self): 
        self.saved_positions.clear()
        self.refresh_listbox()
    
    def save_setup(self):
        filepath = filedialog.asksaveasfilename(defaultextension=".json", filetypes=[("JSON files", "*.json")])
        if filepath: 
            with open(filepath, 'w') as f: json.dump(self.saved_positions, f, indent=4)
            self.log(f"Points saved to {filepath}")

    def load_setup(self):
        filepath = filedialog.askopenfilename(filetypes=[("JSON files", "*.json")])
        if filepath: 
            with open(filepath, 'r') as f: self.saved_positions = json.load(f)
            self.refresh_listbox()
            self.log(f"Points loaded from {filepath}")

    def generate_grid(self):
        try:
            steps_per_mm_x = float(self.steps_per_mm_x_entry.get())
            steps_per_mm_y = float(self.steps_per_mm_y_entry.get())
        except ValueError: 
            messagebox.showwarning("Input Error", "'Steps per 1 mm' values must be valid numbers.")
            return
            
        self.clear_positions()
        start_x, start_y = self.current_x, self.current_y
        dx = int(3.8 * steps_per_mm_x)
        dy = int(4.3 * steps_per_mm_y)
        
        for r in range(self.GRID_ROWS):
            for c in range(self.GRID_COLS): 
                self.saved_positions.append({"x": start_x + (c * dx), "y": start_y - (r * dy)})
                
        self.refresh_listbox()
        self.log(f"Auto-grid generated: X: 3.8mm ({dx} steps), Y: 4.3mm ({dy} steps).")

    # --- CAMERA BACKGROUND THREAD WORKER ---
    def camera_worker(self):
        """Worker loop that continuously drains frames from the camera to avoid timeouts."""
        while self.camera_active:
            if self.cap and self.cap.isOpened():
                ret, frame = self.cap.read()
                if ret:
                    with self.camera_lock:
                        self.latest_frame = frame.copy()
            time.sleep(0.01)

    # --- ORIGINAL LED CONTROL ---
    def check_and_turn_off_led(self):
        self.log("Checking Dino-Lite and turning off LED...")
        dev = usb.core.find(idVendor=0xeb1a, idProduct=0x2801)
        if dev is None:
            self.log("Warning: Dino-Lite USB not found for LED control.")
            return False
        try: 
            dev.ctrl_transfer(0x40, 0x01, 0x0000, 0x0000, b'') 
            self.log("Dino-Lite LED turned off!")
            return True
        except Exception as e: 
            self.log(f"LED hardware error: {e}")
            return False

    # --- CAMERA & LIVE FEED CONTROLS ---
    def toggle_live_camera(self):
        if not self.camera_active:
            self.log("Initializing camera...")
            
            self.check_and_turn_off_led()
            
            backend = cv2.CAP_DSHOW if os.name == 'nt' else cv2.CAP_V4L2
            
            # --- INTELLIGENT CAMERA FINDER FOR LINUX ---
            self.cap = None
            self.log("Probing video devices (looking for actual video feed)...")
            
            for cam_idx in range(10):
                if os.name != 'nt':
                    target = f"/dev/video{cam_idx}"
                    if not os.path.exists(target):
                        continue 
                else:
                    target = cam_idx 
                    
                temp_cap = cv2.VideoCapture(target, backend)
                
                if temp_cap.isOpened():
                    success = False
                    for _ in range(3): 
                        ret, _ = temp_cap.read()
                        if ret:
                            success = True
                            break
                        time.sleep(0.1)
                        
                    if success:
                        self.cap = temp_cap
                        self.log(f"Successfully connected to camera at {target}.")
                        break
                    else:
                        self.log(f"Target {target} opened but gave no video. Skipping.")
                        temp_cap.release()
                else:
                    temp_cap.release()
            
            if self.cap and self.cap.isOpened():
                self.cap.set(cv2.CAP_PROP_FRAME_WIDTH, 2560)
                self.cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 1920)
                
                self.camera_active = True
                self.latest_frame = None
                
                self.cam_thread = threading.Thread(target=self.camera_worker, daemon=True)
                self.cam_thread.start()
                
                self.cam_toggle_btn.config(text="Stop Live View", bootstyle=WARNING)
                self.log("Camera continuous background thread started successfully.")
                self.update_video_stream()
            else:
                self.log("ERROR: Failed to connect to the camera!")
        else:
            self.camera_active = False
            time.sleep(0.1) 
            self.cam_toggle_btn.config(text="Start Live View", bootstyle=INFO)
            self.video_label.config(image='')
            if self.cap: 
                self.cap.release()
                self.cap = None
            self.log("Camera stream stopped.")

    def update_video_stream(self):
        if self.camera_active:
            frame = None
            with self.camera_lock:
                if self.latest_frame is not None:
                    frame = self.latest_frame.copy()
            
            if frame is not None and not self.is_running:
                display_frame = cv2.resize(frame, (640, 480))
                cv2image = cv2.cvtColor(display_frame, cv2.COLOR_BGR2RGBA)
                img = Image.fromarray(cv2image)
                imgtk = ImageTk.PhotoImage(image=img)
                self.video_label.imgtk = imgtk
                self.video_label.configure(image=imgtk)
                
            self.root.after(30, self.update_video_stream)

    def display_static_frame(self, frame):
        """Displays a single static frame during the automated test."""
        display_frame = cv2.resize(frame, (640, 480))
        cv2image = cv2.cvtColor(display_frame, cv2.COLOR_BGR2RGBA)
        img = Image.fromarray(cv2image)
        imgtk = ImageTk.PhotoImage(image=img)
        self.video_label.imgtk = imgtk
        self.video_label.configure(image=imgtk)

    # --- AUTOMATED INSPECTION ---
    def start_inspection_thread(self):
        if not self.is_connected or not self.saved_positions: return
        self.is_running = True
        self.cancel_flag = False
        self.start_btn.config(state=tk.DISABLED)
        self.stop_btn.config(state=tk.NORMAL)
        threading.Thread(target=self.run_inspection, daemon=True).start()

    def stop_process(self):
        self.is_running = False
        self.cancel_flag = True
        self.start_btn.config(state=tk.NORMAL)
        self.stop_btn.config(state=tk.DISABLED)

    def run_inspection(self):
        self.root.after(0, lambda: self.log("\n--- STARTING TRAY INSPECTION ---"))
        self.root.after(0, self.apply_save_folder)
        
        self.grid_colors = ["#444444"] * (self.GRID_COLS * self.GRID_ROWS)
        for idx in range(min(len(self.saved_positions), len(self.grid_colors))):
            self.root.after(0, self.update_grid_color, idx, "#444444")
        
        # Automatically spin up the camera loop if it isn't running yet
        if not self.camera_active:
            self.root.after(0, self.toggle_live_camera)
            time.sleep(4.0) 
            
        total_points = len(self.saved_positions)
        
        for i, pos in enumerate(self.saved_positions):
            if self.cancel_flag: 
                self.root.after(0, self.update_grid_color, i, "#e74c3c")
                break

            self.root.after(0, self.update_grid_color, i, "#f39c12")
            target_x, target_y = pos['x'], pos['y']
            
            safe, reason = self.check_software_limits(target_x, target_y)
            if not safe:
                self.root.after(0, lambda r=reason: self.log(f"ABORT: Saved point violates limits! ({r})"))
                self.root.after(0, self.update_grid_color, i, "#e74c3c")
                break

            dx = target_x - self.current_x
            dy = target_y - self.current_y
            self.root.after(0, lambda idx=i, t=total_points: self.log(f"Moving to point {idx+1}/{t}..."))

            if dx != 0:
                if not self.send_and_wait(f"I1M{dx}, R"): break
                self.current_x = target_x
                self.root.after(0, self.update_ui_labels)

            if dy != 0:
                if not self.send_and_wait(f"I2M{dy}, R"): break
                self.current_y = target_y
                self.root.after(0, self.update_ui_labels)

            # STEP 1: Wait 2 seconds for mechanical vibrations to settle completely
            time.sleep(2.0)
            
            if self.cancel_flag: 
                self.root.after(0, self.update_grid_color, i, "#e74c3c")
                break

            # STEP 2: Safely extract the freshest frame from the non-blocking background thread
            frame = None
            with self.camera_lock:
                if self.latest_frame is not None:
                    frame = self.latest_frame.copy()
            
            if frame is not None:
                row, col = i // self.GRID_COLS, i % self.GRID_COLS
                filename = f"SiPM_R{row:02d}_C{col:02d}.png"
                cv2.imwrite(os.path.join(self.save_dir, filename), frame)
                
                self.root.after(0, lambda f=filename: self.log(f"Captured: {f}"))
                self.root.after(0, lambda f=frame: self.display_static_frame(f))
                self.root.after(0, self.update_grid_color, i, "#18bc9c")
            else:
                self.root.after(0, self.update_grid_color, i, "#e74c3c")

            # STEP 3: Wait remaining 1 second (Guarantees exactly 3 seconds window per chip)
            time.sleep(1.0)

        self.root.after(0, self.stop_process)
        self.root.after(0, lambda: self.log("--- INSPECTION COMPLETE ---"))

if __name__ == "__main__":
    root = tb.Window(themename="darkly")
    app = UnifiedSiPMApp(root)
    root.mainloop()