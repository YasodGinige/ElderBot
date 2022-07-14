import tkinter as tk
from tkinter import messagebox

from serial import Serial
from serial.tools import list_ports

import time

window = tk.Tk()
window.title('ElderBot Authentication')

def upload_wifi():
    ssid = ssid_entry.get()
    pwd = pwd_entry.get()
    device.open()
    device.write(bytes('setWiFi', 'utf-8'))
    time.sleep(0.2)
    device.write(bytes(ssid, 'utf-8'))
    time.sleep(0.2)
    device.write(bytes(pwd, 'utf-8'))
    time.sleep(0.2)
    while (not device.in_waiting): time.sleep(0.01)
    response = str(device.readline())
    device.close()
    if response == "b'Success'": messagebox.showinfo('ElderBot Authentication', 'WiFi connection successful!')
    elif response == "b'Error'": messagebox.showerror('ElderBot Authentication', 'Error in connecting to WiFi')
    else: messagebox.showwarning('ElderBot Authentication', response[2:-1])

def upload_user():
    username = username_entry.get()
    email = email_entry.get()
    pwd = web_pwd_entry.get()
    device.open()
    device.write(bytes('addUser', 'utf-8'))
    time.sleep(0.5)
    device.write(bytes(username, 'utf-8'))
    time.sleep(0.2)
    device.write(bytes(email, 'utf-8'))
    time.sleep(0.2)
    device.write(bytes(pwd, 'utf-8'))
    time.sleep(0.2)
    while (not device.in_waiting): time.sleep(0.01)
    response = str(device.readline())
    device.close()
    if response == "b'UserRegistered'": messagebox.showinfo('ElderBot Authentication', 'User account created successfully!')
    elif response == "b'NoWiFi'": messagebox.showerror('ElderBot Authentication', 'Error in connecting to WiFi')
    elif response == "b'OldUser'": messagebox.showwarning('ElderBot Authentication', 'Email is already registered. Please check email is correct')
    else: messagebox.showwarning('ElderBot Authentication', response[2:-1])


def check_port():
    serial_port = None
    ports = list_ports.comports()
    port_count = 0
    for port, desc, hwid in sorted(ports):
        if 'Silicon Labs' in desc:
            serial_port = port
            port_count+=1
    if port_count == 0:
        status.config(text='No device detected. Please connect the device and press refresh.')
        return None
    elif port_count == 1:
        device = Serial(port = serial_port, baudrate=115200, timeout=0.1)
        device.write(bytes('getUser', 'utf-8'))
        while (not device.in_waiting): time.sleep(0.01)
        user = str(device.readline())
        if user=="b'UNREGISTERED'": status.config(text='Device detected at {}. Device is unregistered.'.format(port))
        else: status.config(text='Device detected at {}. Device is registered to {}.'.format(port, user[2:-1]))
        device.close()
        return device
    else:
        status.config(text='Multiple devices detected. Please connect one device at a time.')
        return None

# Once the UI is finalized convert all .grid()s to .place()s
tk.Label(window, text = 'Welcome to ElderBot').grid(row=0, column=1)
status = tk.Label(window, text = 'Checking device')
status.grid(row=1, column=0)
device = check_port()
tk.Button(window, text = 'Refresh', comman = check_port).grid(row=1, column=2)
tk.Label(window, text = 'WiFi Setup').grid(row=2, column=1)
tk.Label(window, text = 'WiFi SSID').grid(row=3, column=0)
ssid_entry = tk.Entry(window, width=30)
ssid_entry.grid(row=3, column=2)
tk.Label(window, text = 'Password').grid(row=4, column=0)
pwd_entry = tk.Entry(window, width=30)
pwd_entry.grid(row=4, column=2)
tk.Button(window, text = 'Upload WiFi settings', command = upload_wifi).grid(row=5, column=1)
tk.Label(window, text = 'User Setup').grid(row=6, column=1)
tk.Label(window, text = 'Username').grid(row=7, column=0)
username_entry = tk.Entry(window, width=30)
username_entry.grid(row=7, column=2)
tk.Label(window, text = 'Email').grid(row=8, column=0)
email_entry = tk.Entry(window, width=30)
email_entry.grid(row=8, column=2)
tk.Label(window, text = 'Password').grid(row=9, column=0)
web_pwd_entry = tk.Entry(window, width=30)
web_pwd_entry.grid(row=9, column=2)
tk.Button(window, text = 'Create new user', command = upload_user).grid(row=10, column=1)
window.mainloop()

