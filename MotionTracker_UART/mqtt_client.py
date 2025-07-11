#!/usr/bin/env python3
"""
Python MQTT Client for STM32 MotionTracker
Real-time data visualization and motor control
"""

import paho.mqtt.client as mqtt
import json
import matplotlib.pyplot as plt
import matplotlib.animation as animation
from collections import deque
import threading
import time
from datetime import datetime
import tkinter as tk
from tkinter import ttk

class MQTTMotionTracker:
    def __init__(self):
        # MQTT Settings
        self.broker = "broker.hivemq.com"  # Public test broker
        self.port = 1883
        self.topics = {
            'motor_speed': 'motortracker/data/motor_speed',
            'gyro_x': 'motortracker/data/gyro/x', 
            'gyro_y': 'motortracker/data/gyro/y',
            'gyro_z': 'motortracker/data/gyro/z',
            'magnitude': 'motortracker/data/magnitude',
            'all_data': 'motortracker/data/all',
            'motor_control': 'motortracker/control/motor_set'
        }
        
        # Data storage
        self.max_points = 100
        self.motor_data = deque(maxlen=self.max_points)
        self.gyro_x_data = deque(maxlen=self.max_points)
        self.gyro_y_data = deque(maxlen=self.max_points) 
        self.gyro_z_data = deque(maxlen=self.max_points)
        self.timestamps = deque(maxlen=self.max_points)
        
        # Current values
        self.current_motor = 0
        self.current_gyro_x = 0.0
        self.current_gyro_y = 0.0
        self.current_gyro_z = 0.0
        self.current_magnitude = 0.0
        
        # MQTT Client
        self.client = mqtt.Client()
        self.client.on_connect = self.on_connect
        self.client.on_message = self.on_message
        self.connected = False
        
        # GUI
        self.setup_gui()
        
        # Plotting
        self.setup_plot()
        
    def setup_gui(self):
        """Setup Tkinter GUI"""
        self.root = tk.Tk()
        self.root.title("MQTT MotionTracker Client")
        self.root.geometry("400x300")
        
        # Connection frame
        conn_frame = ttk.LabelFrame(self.root, text="MQTT Connection")
        conn_frame.pack(fill=tk.X, padx=10, pady=5)
        
        ttk.Label(conn_frame, text="Broker:").grid(row=0, column=0, sticky=tk.W)
        self.broker_entry = ttk.Entry(conn_frame, value=self.broker)
        self.broker_entry.grid(row=0, column=1, sticky=tk.EW, padx=5)
        
        self.connect_btn = ttk.Button(conn_frame, text="Connect", command=self.connect_mqtt)
        self.connect_btn.grid(row=0, column=2, padx=5)
        
        self.status_label = ttk.Label(conn_frame, text="Disconnected", foreground="red")
        self.status_label.grid(row=1, column=0, columnspan=3)
        
        # Data frame
        data_frame = ttk.LabelFrame(self.root, text="Real-Time Data")
        data_frame.pack(fill=tk.X, padx=10, pady=5)
        
        ttk.Label(data_frame, text="Motor Speed:").grid(row=0, column=0, sticky=tk.W)
        self.motor_label = ttk.Label(data_frame, text="0%", font=("Arial", 12, "bold"))
        self.motor_label.grid(row=0, column=1, sticky=tk.W)
        
        ttk.Label(data_frame, text="Gyro X:").grid(row=1, column=0, sticky=tk.W)
        self.gyro_x_label = ttk.Label(data_frame, text="0.0 dps")
        self.gyro_x_label.grid(row=1, column=1, sticky=tk.W)
        
        ttk.Label(data_frame, text="Gyro Y:").grid(row=2, column=0, sticky=tk.W)
        self.gyro_y_label = ttk.Label(data_frame, text="0.0 dps")
        self.gyro_y_label.grid(row=2, column=1, sticky=tk.W)
        
        ttk.Label(data_frame, text="Gyro Z:").grid(row=3, column=0, sticky=tk.W)
        self.gyro_z_label = ttk.Label(data_frame, text="0.0 dps")
        self.gyro_z_label.grid(row=3, column=1, sticky=tk.W)
        
        # Motor control frame
        control_frame = ttk.LabelFrame(self.root, text="Motor Control")
        control_frame.pack(fill=tk.X, padx=10, pady=5)
        
        speeds = [0, 25, 50, 75, 100]
        for i, speed in enumerate(speeds):
            btn_text = "STOP" if speed == 0 else f"{speed}%"
            btn = ttk.Button(control_frame, text=btn_text, 
                           command=lambda s=speed: self.send_motor_command(s))
            btn.grid(row=0, column=i, padx=2, pady=5)
        
        # Chart button
        ttk.Button(self.root, text="Show Real-Time Charts", 
                  command=self.show_charts).pack(pady=10)
        
    def setup_plot(self):
        """Setup matplotlib for real-time plotting"""
        self.fig, (self.ax1, self.ax2) = plt.subplots(2, 1, figsize=(10, 8))
        
        # Motor speed plot
        self.ax1.set_title("Motor Speed (%)")
        self.ax1.set_ylabel("Speed (%)")
        self.ax1.set_ylim(0, 105)
        self.ax1.grid(True)
        
        # Gyroscope plot
        self.ax2.set_title("Gyroscope Data (dps)")
        self.ax2.set_ylabel("Angular Velocity (dps)")
        self.ax2.set_xlabel("Time")
        self.ax2.grid(True)
        
        plt.tight_layout()
        
    def connect_mqtt(self):
        """Connect to MQTT broker"""
        self.broker = self.broker_entry.get()
        
        try:
            self.client.connect(self.broker, self.port, 60)
            self.client.loop_start()
            
        except Exception as e:
            print(f"MQTT Connection failed: {e}")
            
    def on_connect(self, client, userdata, flags, rc):
        """MQTT connection callback"""
        if rc == 0:
            self.connected = True
            self.status_label.config(text="Connected", foreground="green")
            self.connect_btn.config(text="Disconnect")
            
            # Subscribe to all topics
            for topic in self.topics.values():
                client.subscribe(topic)
                print(f"Subscribed to: {topic}")
                
        else:
            print(f"Failed to connect, return code {rc}")
            
    def on_message(self, client, userdata, msg):
        """MQTT message callback"""
        topic = msg.topic
        payload = msg.payload.decode()
        
        print(f"Received: {topic} = {payload}")
        
        try:
            # Handle different message types
            if topic == self.topics['motor_speed']:
                self.current_motor = int(payload)
                self.update_gui()
                
            elif topic == self.topics['gyro_x']:
                self.current_gyro_x = float(payload)
                
            elif topic == self.topics['gyro_y']:
                self.current_gyro_y = float(payload)
                
            elif topic == self.topics['gyro_z']:
                self.current_gyro_z = float(payload)
                
            elif topic == self.topics['all_data']:
                # Parse JSON data
                data = json.loads(payload)
                self.current_motor = data.get('motor_speed', 0)
                self.current_gyro_x = data.get('gyro_x', 0.0)
                self.current_gyro_y = data.get('gyro_y', 0.0)
                self.current_gyro_z = data.get('gyro_z', 0.0)
                self.current_magnitude = data.get('magnitude', 0.0)
                
                # Add to data storage
                now = datetime.now()
                self.timestamps.append(now)
                self.motor_data.append(self.current_motor)
                self.gyro_x_data.append(self.current_gyro_x)
                self.gyro_y_data.append(self.current_gyro_y)
                self.gyro_z_data.append(self.current_gyro_z)
                
                self.update_gui()
                
        except Exception as e:
            print(f"Error processing message: {e}")
            
    def update_gui(self):
        """Update GUI with current values"""
        self.motor_label.config(text=f"{self.current_motor}%")
        self.gyro_x_label.config(text=f"{self.current_gyro_x:.1f} dps")
        self.gyro_y_label.config(text=f"{self.current_gyro_y:.1f} dps") 
        self.gyro_z_label.config(text=f"{self.current_gyro_z:.1f} dps")
        
    def send_motor_command(self, speed):
        """Send motor control command via MQTT"""
        if self.connected:
            command = json.dumps({"speed": speed})
            self.client.publish(self.topics['motor_control'], command)
            print(f"Motor command sent: {speed}%")
        else:
            print("Not connected to MQTT broker")
            
    def show_charts(self):
        """Show real-time charts"""
        if len(self.timestamps) == 0:
            print("No data to display")
            return
            
        # Clear previous plots
        self.ax1.clear()
        self.ax2.clear()
        
        # Time range for x-axis
        times = list(self.timestamps)
        
        # Motor speed plot
        self.ax1.plot(times, list(self.motor_data), 'b-', linewidth=2, label='Motor Speed')
        self.ax1.set_title("Motor Speed (%)")
        self.ax1.set_ylabel("Speed (%)")
        self.ax1.set_ylim(0, 105)
        self.ax1.grid(True)
        self.ax1.legend()
        
        # Gyroscope plot
        self.ax2.plot(times, list(self.gyro_x_data), 'r-', label='X-axis', linewidth=2)
        self.ax2.plot(times, list(self.gyro_y_data), 'g-', label='Y-axis', linewidth=2)
        self.ax2.plot(times, list(self.gyro_z_data), 'b-', label='Z-axis', linewidth=2)
        self.ax2.set_title("Gyroscope Data (dps)")
        self.ax2.set_ylabel("Angular Velocity (dps)")
        self.ax2.set_xlabel("Time")
        self.ax2.grid(True)
        self.ax2.legend()
        
        # Format x-axis
        import matplotlib.dates as mdates
        self.ax1.xaxis.set_major_formatter(mdates.DateFormatter('%H:%M:%S'))
        self.ax2.xaxis.set_major_formatter(mdates.DateFormatter('%H:%M:%S'))
        self.fig.autofmt_xdate()
        
        plt.tight_layout()
        plt.show()
        
    def run(self):
        """Start the application"""
        self.root.mainloop()
        
if __name__ == "__main__":
    # Install required packages:
    # pip install paho-mqtt matplotlib
    
    app = MQTTMotionTracker()
    app.run() 