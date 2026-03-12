"""
MicroPython Blink Program for ESP32 WROOM DevKit
Built-in LED is typically on GPIO 2
"""

from machine import Pin
from time import sleep

led = Pin(2, Pin.OUT)

while True:
    led.on()
    sleep(0.5)
    led.off()
    sleep(0.5)
