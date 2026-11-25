# FX True Ballistic Radar — BLE Protocol (reverse-engineered)

Reverse-engineered 2026-11-25 via APK decompilation (jadx) + live ESP32 BLE tap.
All findings below verified against real hardware.

## Hardware identity

The radar's BLE radio is an **Adafruit Bluefruit LE module**:

- Chip: **nRF51822** (Nordic nRF51), SoftDevice **S110 8.0.0**
- Model string: `BLEFRIEND32`
- Manufacturer: `Adafruit Industries`
- Firmware: `0.8.1 - Apr 10 2019` (Adafruit Bluefruit LE UART bridge)

The doppler radar MCU talks UART to this module; the module bridges it over BLE.

## Advertising

- Name: `vXfS3c6k` (fixed)
- MAC: `d0:3a:c4:71:d4:ad` (random static — stable across reboots)
- Advertised service: `6e400001-b5a3-f393-e0a9-e50e24dcca9e` (Nordic UART Service)
- Legacy advertising — visible to BLE 4.2 (classic ESP32) and BLE 5 alike.

## GATT tree

| Service | Characteristics | Notes |
|---------|-----------------|-------|
| `0x1800` | `0x2a00` name, `0x2a01`, `0x2a04` | Generic Access |
| `0x1801` | `0x2a05` indicate | Generic Attribute |
| `00001530-1212-efde-1523-785feabcd123` | `00001531`/`32`/`34` | Adafruit Buttonless DFU |
| `0x180a` | `0x2a24` model, `0x2a26` fw, `0x2a28` sw, `0x2a29` mfr, `0x2a27` hw | Device Info |
| `6e400001-b5a3-f393-e0a9-e50e24dcca9e` | `6e400003` NOTIFY (TX), `6e400002` WRITE (RX) | NUS (advertised) |
| `00001623-88ec-688c-644b-3fa706c0bb76` | `00001624` READ+NOTIFY, `00001625` READ+WRITE, `00001626` READ, `00001627` READ, `0000162b` READ+NOTIFY | **FX data service** |

The SMP DFU UUIDs `8D53DC1D-1DB7-4CD3-868B-8A527460AA84` /
`DA2E7828-FBCE-4E01-AE9E-261174997C48` (seen in the APK) are the Adafruit nRF52
bootloader's MCUmgr DFU service — firmware update only, **not** shot data.

## Shot data (the important part)

Subscribe to **`00001624`** (notify). Each shot sends a 9-byte notification:

```
offset   0     1     2     3     4     5     6     7     8
       [0x0B] [0x00] [   freq  u16 BE   ] [    BC   float32 BE ] [0x05]
```

- bytes 0, 1, 8: constant (`0B 00 ... 05`) — opcode / flags / terminator.
- bytes 2-3: **doppler frequency**, uint16 big-endian (Hz).
- bytes 4-7: **ballistic coefficient (BC)**, IEEE754 float32 big-endian.

### Velocity conversion

```
v_m/s = freq / 160.647 = freq * 0.0062248
```

Equivalent to a ~24.097 GHz K-band doppler: `v = c * fd / (2 * f0)`.

Measured across 7 shots (screen velocity vs freq), stable to ±0.01:

| freq  | screen m/s | BC    |
|-------|-----------|-------|
| 51200 | 318.7     | 0.1197|
| 51651 | 321.5     | 0.1200|
| 51536 | 320.8     | 0.1336|
| 52290 | 325.5     | 0.1403|
| 50974 | 317.3     | 0.1349|
| 51418 | 320.1     | 0.1345|
| 52338 | 325.8     | 0.1410|

Store the **raw freq** (not the screen value) — the screen rounds to 0.1 m/s;
the freq gives ~0.006 m/s resolution.

## Secondary shot notify: `0000162b`

Sends an ASCII JSON string per shot, e.g. `"10454-00000-00000"`.
Opaque and non-monotonic — likely a hash / serial. Ignore for data logging.

## Other characteristics

- `00001625` (READ+WRITE): reads `00 00`. Config-write path (distance, drag table,
  units) — **not yet reversed**; the radar's own screen covers it for now.
- `00001626` (READ): `00 00 00 00`.
- `00001627` (READ): 1 byte (observed `0x32`, `0x19`) — status, meaning TBD.
- `0000162b` (READ): returns the last ASCII serial.

## Capture recipe (ESP32 / NimBLE)

```cpp
NimBLEDevice::init("...");
NimBLEAddress addr(std::string("d0:3a:c4:71:d4:ad"), BLE_ADDR_RANDOM);
NimBLEClient* c = NimBLEDevice::createClient(addr);
c->connect();
auto services = c->getServices(true);
for (auto* svc : services)
  for (auto* ch : svc->getCharacteristics(true))
    if (ch->getUUID().toString() == "00001624-88ec-688c-644b-3fa706c0bb76")
      ch->subscribe(true, onNotify);
```

On each notification:

```cpp
uint16_t freq = (data[2] << 8) | data[3];
float bc;
memcpy(&bc, data + 4, 4);   // big-endian float — byteswap if little-endian CPU
float v_mps = freq / 160.647f;
```

No pairing required for shot capture (link works unpaired; the app's 6-digit PIN
"linked" mode is only for 0.1 fps readings).

## Re-obtaining the APK (if ever needed)

```
adb shell pm path com.fxairguns.fxchrony
adb pull <each path>
jadx base.apk            # DEX -> Java
```

Package: `com.fxairguns.fxchrony` (Flutter app; logic in `libapp.so` Dart AOT).
