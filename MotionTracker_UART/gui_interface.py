#!/usr/bin/env python3
"""
STM32 MotionTracker UART GUI Interface
Real-time gyroscope data visualization and motor control monitoring
Author: Generated for STM32F3 Discovery MotionTracker project
"""

import tkinter as tk
from tkinter import ttk, messagebox
import serial
import serial.tools.list_ports
import threading
import time
import matplotlib.pyplot as plt
from matplotlib.backends.backend_tkagg import FigureCanvasTk
from matplotlib.animation import FuncAnimation
import re
from collections import deque
import json
from datetime import datetime

class MotionTrackerGUI:
    def __init__(self, root):
        self.root = root
        self.root.title("STM32 MotionTracker - Gyroscope & Motor Control")
        self.root.geometry("1000x700")
        
        # Serial connection
        self.serial_port = None
        self.is_connected = False
        self.is_running = False
        
        # Data storage
        self.max_data_points = 100
        self.gyro_x = deque(maxlen=self.max_data_points)
        self.gyro_y = deque(maxlen=self.max_data_points)
        self.gyro_z = deque(maxlen=self.max_data_points)
        self.motor_speed = deque(maxlen=self.max_data_points)
        self.timestamps = deque(maxlen=self.max_data_points)
        
        # Current values
        self.current_x = 0.0
        self.current_y = 0.0
        self.current_z = 0.0
        self.current_magnitude = 0.0
        self.current_motor = 0
        
        self.setup_ui()
        self.setup_plot()
        
    def setup_ui(self):
        """Ana arayüzü oluştur"""
        # Ana frame
        main_frame = ttk.Frame(self.root, padding="10")
        main_frame.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))
        
        # Connection Frame
        conn_frame = ttk.LabelFrame(main_frame, text="Bağlantı Ayarları", padding="10")
        conn_frame.grid(row=0, column=0, columnspan=2, sticky=(tk.W, tk.E), pady=(0, 10))
        
        # COM Port Selection
        ttk.Label(conn_frame, text="COM Port:").grid(row=0, column=0, padx=(0, 5))
        self.port_var = tk.StringVar()
        self.port_combo = ttk.Combobox(conn_frame, textvariable=self.port_var, width=15)
        self.port_combo.grid(row=0, column=1, padx=(0, 10))
        
        # Refresh button
        ttk.Button(conn_frame, text="Yenile", command=self.refresh_ports).grid(row=0, column=2, padx=(0, 10))
        
        # Connect button
        self.connect_btn = ttk.Button(conn_frame, text="Bağlan", command=self.toggle_connection)
        self.connect_btn.grid(row=0, column=3, padx=(0, 10))
        
        # Status
        self.status_label = ttk.Label(conn_frame, text="Bağlantı Bekleniyor", foreground="red")
        self.status_label.grid(row=0, column=4)
        
        # Data Display Frame
        data_frame = ttk.LabelFrame(main_frame, text="Anlık Veriler", padding="10")
        data_frame.grid(row=1, column=0, sticky=(tk.W, tk.E, tk.N, tk.S), pady=(0, 10))
        
        # Gyroscope values
        ttk.Label(data_frame, text="Gyroscope X:", font=("Arial", 10, "bold")).grid(row=0, column=0, sticky=tk.W)
        self.x_label = ttk.Label(data_frame, text="0.0 dps", font=("Arial", 12))
        self.x_label.grid(row=0, column=1, padx=(10, 0), sticky=tk.W)
        
        ttk.Label(data_frame, text="Gyroscope Y:", font=("Arial", 10, "bold")).grid(row=1, column=0, sticky=tk.W)
        self.y_label = ttk.Label(data_frame, text="0.0 dps", font=("Arial", 12))
        self.y_label.grid(row=1, column=1, padx=(10, 0), sticky=tk.W)
        
        ttk.Label(data_frame, text="Gyroscope Z:", font=("Arial", 10, "bold")).grid(row=2, column=0, sticky=tk.W)
        self.z_label = ttk.Label(data_frame, text="0.0 dps", font=("Arial", 12))
        self.z_label.grid(row=2, column=1, padx=(10, 0), sticky=tk.W)
        
        ttk.Label(data_frame, text="Magnitude:", font=("Arial", 10, "bold")).grid(row=3, column=0, sticky=tk.W)
        self.mag_label = ttk.Label(data_frame, text="0.0 dps", font=("Arial", 12))
        self.mag_label.grid(row=3, column=1, padx=(10, 0), sticky=tk.W)
        
        ttk.Label(data_frame, text="Motor Hızı:", font=("Arial", 10, "bold")).grid(row=4, column=0, sticky=tk.W)
        self.motor_label = ttk.Label(data_frame, text="0%", font=("Arial", 12, "bold"), foreground="blue")
        self.motor_label.grid(row=4, column=1, padx=(10, 0), sticky=tk.W)
        
        # Motor speed progress bar
        ttk.Label(data_frame, text="Motor Seviyesi:").grid(row=5, column=0, sticky=tk.W, pady=(10, 0))
        self.motor_progress = ttk.Progressbar(data_frame, length=200, mode='determinate')
        self.motor_progress.grid(row=5, column=1, padx=(10, 0), pady=(10, 0), sticky=tk.W)
        
        # Control Frame
        control_frame = ttk.LabelFrame(main_frame, text="Kontroller", padding="10")
        control_frame.grid(row=2, column=0, sticky=(tk.W, tk.E), pady=(0, 10))
        
        ttk.Button(control_frame, text="Verileri Temizle", command=self.clear_data).grid(row=0, column=0, padx=(0, 10))
        ttk.Button(control_frame, text="Verileri Kaydet", command=self.save_data).grid(row=0, column=1, padx=(0, 10))
        ttk.Button(control_frame, text="Grafiği Durdur/Başlat", command=self.toggle_plot).grid(row=0, column=2, padx=(0, 10))
        
        # Motor Kontrol Frame
        motor_control_frame = ttk.LabelFrame(main_frame, text="Motor Kontrol (Test)", padding="10")
        motor_control_frame.grid(row=4, column=0, sticky=(tk.W, tk.E), pady=(10, 0))
        
        # Motor control buttons
        motor_buttons = [
            ("🛑 STOP", 0, "red"),
            ("🐌 25%", 25, "orange"), 
            ("🚶 50%", 50, "blue"),
            ("🏃 75%", 75, "green"),
            ("🚀 100%", 100, "purple")
        ]
        
        for i, (text, speed, color) in enumerate(motor_buttons):
            btn = tk.Button(motor_control_frame, text=text, 
                           command=lambda s=speed: self.send_motor_command(s),
                           width=12, height=2, font=("Arial", 10, "bold"))
            btn.grid(row=0, column=i, padx=5, pady=5)
        
        # Raw data display
        raw_frame = ttk.LabelFrame(main_frame, text="Ham Veri", padding="10")
        raw_frame.grid(row=3, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))
        
        self.raw_text = tk.Text(raw_frame, height=8, width=50, wrap=tk.WORD)
        scrollbar = ttk.Scrollbar(raw_frame, orient="vertical", command=self.raw_text.yview)
        self.raw_text.configure(yscrollcommand=scrollbar.set)
        self.raw_text.grid(row=0, column=0, sticky=(tk.W, tk.E, tk.N, tk.S))
        scrollbar.grid(row=0, column=1, sticky=(tk.N, tk.S))
        
        # Initialize ports
        self.refresh_ports()
        
    def setup_plot(self):
        """Grafik alanını oluştur"""
        # Plot frame
        plot_frame = ttk.LabelFrame(self.root, text="Real-Time Grafikler", padding="10")
        plot_frame.grid(row=0, column=1, sticky=(tk.W, tk.E, tk.N, tk.S), padx=(10, 0))
        
        # Create matplotlib figure
        self.fig, (self.ax1, self.ax2) = plt.subplots(2, 1, figsize=(8, 6))
        
        # Gyroscope plot
        self.ax1.set_title("Gyroscope Verileri (dps)")
        self.ax1.set_ylabel("Açısal Hız (dps)")
        self.ax1.legend(['X', 'Y', 'Z'])
        self.ax1.grid(True)
        
        # Motor speed plot
        self.ax2.set_title("Motor Hızı (%)")
        self.ax2.set_ylabel("Hız (%)")
        self.ax2.set_xlabel("Zaman")
        self.ax2.grid(True)
        
        # Create canvas
        self.canvas = FigureCanvasTk(self.fig, plot_frame)
        self.canvas.get_tk_widget().pack(fill=tk.BOTH, expand=True)
        
        # Animation
        self.ani = FuncAnimation(self.fig, self.update_plot, interval=100, blit=False)
        
    def refresh_ports(self):
        """Mevcut COM portlarını listele"""
        ports = serial.tools.list_ports.comports()
        port_list = [port.device for port in ports]
        self.port_combo['values'] = port_list
        if port_list:
            self.port_combo.set(port_list[0])
    
    def toggle_connection(self):
        """Bağlantıyı aç/kapat"""
        if not self.is_connected:
            self.connect()
        else:
            self.disconnect()
    
    def connect(self):
        """Serial bağlantısını başlat"""
        try:
            port = self.port_var.get()
            if not port:
                messagebox.showerror("Hata", "Lütfen bir COM port seçin!")
                return
                
            self.serial_port = serial.Serial(port, 115200, timeout=1)
            self.is_connected = True
            self.is_running = True
            
            # UI güncellemeleri
            self.connect_btn.config(text="Bağlantıyı Kes")
            self.status_label.config(text="Bağlandı", foreground="green")
            
            # Veri okuma thread'ini başlat
            self.read_thread = threading.Thread(target=self.read_serial_data, daemon=True)
            self.read_thread.start()
            
        except Exception as e:
            messagebox.showerror("Bağlantı Hatası", f"Port açılamadı: {str(e)}")
    
    def disconnect(self):
        """Serial bağlantısını kapat"""
        self.is_running = False
        self.is_connected = False
        
        if self.serial_port and self.serial_port.is_open:
            self.serial_port.close()
        
        # UI güncellemeleri
        self.connect_btn.config(text="Bağlan")
        self.status_label.config(text="Bağlantı Kesildi", foreground="red")
    
    def read_serial_data(self):
        """Serial porttan veri oku"""
        while self.is_running and self.serial_port and self.serial_port.is_open:
            try:
                line = self.serial_port.readline().decode('utf-8').strip()
                if line:
                    self.process_data(line)
            except Exception as e:
                print(f"Veri okuma hatası: {e}")
                break
    
    def process_data(self, data):
        """Gelen veriyi işle"""
        # Ham veriyi göster
        self.raw_text.insert(tk.END, data + "\n")
        self.raw_text.see(tk.END)
        
        # Yeni veri formatı: "MOTOR:25,X:1.2,Y:-0.8,Z:2.5" + eski format
        # STM32'den gelen format: "MOTOR:25,X:1.2,Y:-0.8,Z:2.5"
        motor_pattern = r"MOTOR:(\d+),X:([-\d.]+),Y:([-\d.]+),Z:([-\d.]+)"
        motor_match = re.search(motor_pattern, data)
        
        # Eski format desteği: "Gyro[X:1.2 Y:0.8 Z:-2.5] |2.9| -> Motor:26%"
        old_pattern = r"Gyro\[X:([-\d.]+) Y:([-\d.]+) Z:([-\d.]+)\] \|([-\d.]+)\| -> Motor:(\d+)%"
        old_match = re.search(old_pattern, data)
        
        match = motor_match or old_match
        
        if match:
            if motor_match:  # Yeni format: MOTOR:25,X:1.2,Y:-0.8,Z:2.5
                motor, x, y, z = match.groups()
                self.current_motor = int(motor)
                self.current_x = float(x)
                self.current_y = float(y)
                self.current_z = float(z)
                # Magnitude hesapla
                self.current_magnitude = (self.current_x**2 + self.current_y**2 + self.current_z**2)**0.5
            else:  # Eski format: Gyro[X:1.2 Y:0.8 Z:-2.5] |2.9| -> Motor:26%
                x, y, z, magnitude, motor = match.groups()
                self.current_x = float(x)
                self.current_y = float(y)
                self.current_z = float(z)
                self.current_magnitude = float(magnitude)
                self.current_motor = int(motor)
            
            # Verileri depola
            current_time = datetime.now()
            self.timestamps.append(current_time)
            self.gyro_x.append(self.current_x)
            self.gyro_y.append(self.current_y)
            self.gyro_z.append(self.current_z)
            self.motor_speed.append(self.current_motor)
            
            # UI güncelle
            self.root.after(0, self.update_ui)
    
    def update_ui(self):
        """UI elementlerini güncelle"""
        self.x_label.config(text=f"{self.current_x:.1f} dps")
        self.y_label.config(text=f"{self.current_y:.1f} dps")
        self.z_label.config(text=f"{self.current_z:.1f} dps")
        self.mag_label.config(text=f"{self.current_magnitude:.1f} dps")
        self.motor_label.config(text=f"{self.current_motor}%")
        
        # Progress bar güncelle
        self.motor_progress['value'] = self.current_motor
    
    def update_plot(self, frame):
        """Grafikleri güncelle"""
        if len(self.timestamps) < 2:
            return
        
        # Grafikleri temizle
        self.ax1.clear()
        self.ax2.clear()
        
        # Time axis (son 30 saniye)
        time_range = range(len(self.timestamps))
        
        # Gyroscope plot
        self.ax1.plot(time_range, list(self.gyro_x), 'r-', label='X', linewidth=2)
        self.ax1.plot(time_range, list(self.gyro_y), 'g-', label='Y', linewidth=2)
        self.ax1.plot(time_range, list(self.gyro_z), 'b-', label='Z', linewidth=2)
        self.ax1.set_title("Gyroscope Verileri (dps)")
        self.ax1.set_ylabel("Açısal Hız (dps)")
        self.ax1.legend()
        self.ax1.grid(True)
        
        # Motor speed plot - enhanced
        motor_speeds = list(self.motor_speed)
        self.ax2.plot(time_range, motor_speeds, 'gold', linewidth=4, marker='o', markersize=4)
        self.ax2.fill_between(time_range, motor_speeds, alpha=0.3, color='gold')
        self.ax2.set_title("Motor Hızı (%) - Real-Time", fontsize=14, fontweight='bold')
        self.ax2.set_ylabel("Hız (%)", fontsize=12)
        self.ax2.set_xlabel("Veri Noktası", fontsize=12)
        self.ax2.set_ylim(0, 105)
        self.ax2.grid(True, alpha=0.7)
        
        # Motor hızı renk kodlaması
        if motor_speeds:
            current_speed = motor_speeds[-1]
            if current_speed == 0:
                color = 'red'
            elif current_speed <= 25:
                color = 'orange'
            elif current_speed <= 50:
                color = 'yellow'
            elif current_speed <= 75:
                color = 'lightgreen'
            else:
                color = 'green'
            
            # Son veri noktasını vurgula
            if len(time_range) > 0:
                self.ax2.scatter(time_range[-1], current_speed, color=color, s=100, zorder=5)
                self.ax2.text(time_range[-1], current_speed + 5, f'{current_speed}%', 
                             ha='center', fontweight='bold', fontsize=10)
        
        # Layout ayarla
        self.fig.tight_layout()
    
    def clear_data(self):
        """Tüm verileri temizle"""
        self.gyro_x.clear()
        self.gyro_y.clear()
        self.gyro_z.clear()
        self.motor_speed.clear()
        self.timestamps.clear()
        self.raw_text.delete(1.0, tk.END)
    
    def save_data(self):
        """Verileri dosyaya kaydet"""
        if not self.timestamps:
            messagebox.showwarning("Uyarı", "Kaydedilecek veri yok!")
            return
        
        filename = f"motion_data_{datetime.now().strftime('%Y%m%d_%H%M%S')}.json"
        
        data = {
            'timestamps': [t.isoformat() for t in self.timestamps],
            'gyro_x': list(self.gyro_x),
            'gyro_y': list(self.gyro_y),
            'gyro_z': list(self.gyro_z),
            'motor_speed': list(self.motor_speed)
        }
        
        try:
            with open(filename, 'w') as f:
                json.dump(data, f, indent=2)
            messagebox.showinfo("Başarılı", f"Veriler {filename} dosyasına kaydedildi!")
        except Exception as e:
            messagebox.showerror("Hata", f"Dosya kaydedilemedi: {str(e)}")
    
    def toggle_plot(self):
        """Grafik animasyonunu durdur/başlat"""
        if self.ani.event_source:
            self.ani.event_source.stop()
        else:
            self.ani.event_source.start()
    
    def send_motor_command(self, speed):
        """Motor kontrol komutu gönder"""
        if not self.is_connected or not self.serial_port:
            messagebox.showwarning("Uyarı", "Önce STM32'ye bağlanmalısınız!")
            return
        
        try:
            command = f"MOTOR_SET:{speed}\r\n"
            self.serial_port.write(command.encode())
            
            # Raw text'e ekle
            self.raw_text.insert(tk.END, f"📤 GÖNDERILEN: {command.strip()}\n")
            self.raw_text.see(tk.END)
            
            print(f"Motor komutu gönderildi: {command.strip()}")
            
        except Exception as e:
            messagebox.showerror("Hata", f"Motor komutu gönderilemedi: {str(e)}")

def main():
    root = tk.Tk()
    app = MotionTrackerGUI(root)
    
    def on_closing():
        app.disconnect()
        root.destroy()
    
    root.protocol("WM_DELETE_WINDOW", on_closing)
    root.mainloop()

if __name__ == "__main__":
    main() 