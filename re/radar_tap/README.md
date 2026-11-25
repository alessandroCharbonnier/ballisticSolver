# Radar Tap

Passive BLE scanner to observe the **FX True Ballistic** ("Pocket Pro") radar.
Standalone research tool — separate from the main device firmware.

## Build & run

```
pio run -e radar_tap -t upload
pio device monitor -b 115200
```

## What it prints

Every BLE advertising packet: name, MAC, RSSI, advertising type, manufacturer
data (hex), service data (hex), and advertised service UUIDs. FX radar
candidates are marked with `>>>` and `<<< FX radar candidate`.

## Goal

1. Confirm the radar's advertising name / MAC.
2. Check whether shot data rides in **broadcast advertising** (the FX app hints
   shots arrive even without pairing). If so, the proxy never needs to pair.

If the radar does not appear at all in the scan, it likely uses BLE 5 extended
advertising, which the classic ESP32 (BLE 4.2) cannot see — that would force a
pivot to an ESP32-C3/S3 or a different capture approach.
