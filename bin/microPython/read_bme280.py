"""
Read BME280/BMP280 sensor data on ESP32 WROOM via I2C
Wiring:
  VCC -> 3.3V
  GND -> GND
  SCL -> GPIO 22
  SDA -> GPIO 21
"""

from machine import Pin, I2C
from time import sleep
from bme280 import BME280

# Initialize I2C (default ESP32 pins)
i2c = I2C(0, scl=Pin(22), sda=Pin(21), freq=400000)

# Scan for devices
devices = i2c.scan()
if devices:
    print("I2C devices found:", ["0x{:02X}".format(d) for d in devices])
else:
    print("No I2C devices found! Check wiring.")

# Initialize sensor
sensor = BME280(i2c)
print("Detected:", sensor.chip_name)
print("-" * 40)

# Read and print data every 2 seconds
while True:
    print("Detected:", sensor.chip_name)
    temp, pressure, humidity = sensor.read()

    print("Temperature: {:.1f} °C / {:.1f} °F".format(temp, temp * 9/5 + 32))
    print("Pressure:    {:.2f} hPa".format(pressure))

    if humidity is not None:
        print("Humidity:    {:.1f} %".format(humidity))
    else:
        print("Humidity:    N/A (BMP280)")

    print("-" * 40)
    sleep(2)
