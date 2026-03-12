"""
BME280/BMP280 MicroPython Driver (I2C)
Supports both BME280 (temp + humidity + pressure) and BMP280 (temp + pressure)
"""

from micropython import const
from ustruct import unpack

BME280_I2C_ADDR = const(0x76)

class BME280:
    def __init__(self, i2c, addr=BME280_I2C_ADDR):
        self.i2c = i2c
        self.addr = addr
        self._is_bme280 = False

        # Check chip ID
        chip_id = self._read_byte(0xD0)
        if chip_id == 0x60:
            self._is_bme280 = True  # BME280
        elif chip_id == 0x58:
            self._is_bme280 = False  # BMP280
        else:
            raise RuntimeError("Chip not found. ID: 0x{:02X}".format(chip_id))

        # Read calibration data
        self._read_calibration()

        # Set config: standby 1000ms, filter x16
        self._write_byte(0xF5, 0xB0)

        if self._is_bme280:
            # Humidity oversampling x1
            self._write_byte(0xF2, 0x01)

        # Temp oversampling x2, pressure oversampling x16, normal mode
        self._write_byte(0xF4, 0x57)

    def _read_byte(self, reg):
        return self.i2c.readfrom_mem(self.addr, reg, 1)[0]

    def _write_byte(self, reg, val):
        self.i2c.writeto_mem(self.addr, reg, bytes([val]))

    def _read_calibration(self):
        # Temperature & pressure calibration (0x88-0x9F)
        cal = self.i2c.readfrom_mem(self.addr, 0x88, 26)
        self.dig_T1 = unpack('<H', cal[0:2])[0]
        self.dig_T2 = unpack('<h', cal[2:4])[0]
        self.dig_T3 = unpack('<h', cal[4:6])[0]
        self.dig_P1 = unpack('<H', cal[6:8])[0]
        self.dig_P2 = unpack('<h', cal[8:10])[0]
        self.dig_P3 = unpack('<h', cal[10:12])[0]
        self.dig_P4 = unpack('<h', cal[12:14])[0]
        self.dig_P5 = unpack('<h', cal[14:16])[0]
        self.dig_P6 = unpack('<h', cal[16:18])[0]
        self.dig_P7 = unpack('<h', cal[18:20])[0]
        self.dig_P8 = unpack('<h', cal[20:22])[0]
        self.dig_P9 = unpack('<h', cal[22:24])[0]

        if self._is_bme280:
            # Humidity calibration
            self.dig_H1 = self._read_byte(0xA1)
            hcal = self.i2c.readfrom_mem(self.addr, 0xE1, 7)
            self.dig_H2 = unpack('<h', hcal[0:2])[0]
            self.dig_H3 = hcal[2]
            self.dig_H4 = (hcal[3] << 4) | (hcal[4] & 0x0F)
            self.dig_H5 = (hcal[5] << 4) | ((hcal[4] >> 4) & 0x0F)
            self.dig_H6 = unpack('<b', bytes([hcal[6]]))[0]

    def _read_raw(self):
        # Read raw data (pressure + temp + humidity)
        if self._is_bme280:
            data = self.i2c.readfrom_mem(self.addr, 0xF7, 8)
        else:
            data = self.i2c.readfrom_mem(self.addr, 0xF7, 6)

        raw_press = (data[0] << 12) | (data[1] << 4) | (data[2] >> 4)
        raw_temp = (data[3] << 12) | (data[4] << 4) | (data[5] >> 4)
        raw_hum = None
        if self._is_bme280:
            raw_hum = (data[6] << 8) | data[7]

        return raw_temp, raw_press, raw_hum

    def _compensate_temp(self, raw_temp):
        var1 = (((raw_temp >> 3) - (self.dig_T1 << 1)) * self.dig_T2) >> 11
        var2 = (((((raw_temp >> 4) - self.dig_T1) *
                  ((raw_temp >> 4) - self.dig_T1)) >> 12) * self.dig_T3) >> 14
        self.t_fine = var1 + var2
        temp = (self.t_fine * 5 + 128) >> 8
        return temp / 100.0

    def _compensate_pressure(self, raw_press):
        var1 = self.t_fine - 128000
        var2 = var1 * var1 * self.dig_P6
        var2 = var2 + ((var1 * self.dig_P5) << 17)
        var2 = var2 + (self.dig_P4 << 35)
        var1 = ((var1 * var1 * self.dig_P3) >> 8) + ((var1 * self.dig_P2) << 12)
        var1 = ((1 << 47) + var1) * self.dig_P1 >> 33
        if var1 == 0:
            return 0.0
        p = 1048576 - raw_press
        p = (((p << 31) - var2) * 3125) // var1
        var1 = (self.dig_P9 * (p >> 13) * (p >> 13)) >> 25
        var2 = (self.dig_P8 * p) >> 19
        p = ((p + var1 + var2) >> 8) + (self.dig_P7 << 4)
        return p / 25600.0

    def _compensate_humidity(self, raw_hum):
        h = self.t_fine - 76800
        h = (((((raw_hum << 14) - (self.dig_H4 << 20) -
               (self.dig_H5 * h)) + 16384) >> 15) *
             (((((((h * self.dig_H6) >> 10) *
                  (((h * self.dig_H3) >> 11) + 32768)) >> 10) +
                2097152) * self.dig_H2 + 8192) >> 14))
        h = h - (((((h >> 15) * (h >> 15)) >> 7) * self.dig_H1) >> 4)
        h = max(0, min(h, 419430400))
        return (h >> 12) / 1024.0

    def read(self):
        """Returns (temperature_C, pressure_hPa, humidity_%) as a tuple.
        Humidity is None if using BMP280."""
        raw_temp, raw_press, raw_hum = self._read_raw()
        temp = self._compensate_temp(raw_temp)
        press = self._compensate_pressure(raw_press)
        hum = None
        if self._is_bme280 and raw_hum is not None:
            hum = self._compensate_humidity(raw_hum)
        return temp, press, hum

    @property
    def chip_name(self):
        return "BME280" if self._is_bme280 else "BMP280"
