import serial
import time

import serial.tools.list_ports
ports = serial.tools.list_ports.comports()
Serial_port='COM1'

for port, desc, hwid in sorted(ports):
    print(port)
    print("{}: {} [{}]".format(port, desc, hwid))
    if 'Silicon Labs' in desc:
        Serial_port=port

arduino = serial.Serial(port=Serial_port, baudrate=115200, timeout=0.1)

def write_read(x):
    arduino.write(bytes(x, 'utf-8'))
    time.sleep(0.05)
    data = arduino.readline()
    return data

while True:
    num = input("Enter a number: ")
    value = write_read(num)
    print(value)

