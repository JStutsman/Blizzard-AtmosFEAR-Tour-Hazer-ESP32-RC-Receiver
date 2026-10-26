# Hazer Remote - Receiver

The receiver for the **Hazer Remote**, a wireless touchscreen remote for the **Blizzard AtmosFEAR Tour HZ** haze machine. It plugs into the hazer's DMX input, takes commands from the remote over ESP-NOW, and outputs continuous DMX512.

This repo is the **receiver**. The touchscreen remote is in
[Blizzard-AtmosFEAR-Tour-Hazer-ESP32-RC](https://github.com/JStutsman/Blizzard-AtmosFEAR-Tour-Hazer-ESP32-RC).

**📖 [User Guide (PDF)](docs/Hazer-Remote-User-Guide.pdf)** covers setup, wiring, LED patterns, setup mode and troubleshooting for the whole system.

Firmware **v1.1**. Built and running on hardware.

## Features

- **DMX output:** continuous DMX512 on three channels per hazer: emit at the base address, fan at base+1, volume at base+2. Default base is 510.
- **DMX address:** set from the remote (or the setup page) and saved to flash.
- **Fail-safe:** outputs go to zero after 3 s without data (adjustable), for the first 2 s after power-up, and in setup mode.
- **One controlling remote at a time:** other remotes get a BUSY reply and show *In use*. Control passes on after 3 s of silence, on SAFE, or on TAKE_CONTROL.
- **Status replies** at 4 Hz: firmware version, RSSI, output echo, DMX address and flags.
- **Setup mode:** a WiFi access point with a web page for settings and a bench test.
- **Status LED** blink patterns.

## Hardware and wiring

| ESP32-C3 pin | Connects to | Purpose |
|---|---|---|
| GPIO10 | RS485 module DI | DMX data out |
| GPIO7 | RS485 module DE + RE (tied together) | Held high = transmit |
| GPIO8 | On-board LED (C3 SuperMini, active low) | Status LED |
| RS485 A / B | DMX pin 3 (+) / pin 2 (-), GND to pin 1 | To the hazer |

Set the hazer's own DMX address ("DMX512 Add") to match the receiver's.

## Status LED

| Pattern | Meaning |
|---|---|
| One short blink | Linked, receiving data |
| Double blink | Fail-safe / no data |
| Fast flicker | Setup mode |
| Slow blink | Identify |
| N quick blinks at power-up | Counting quick power cycles toward setup mode |

## Setup mode

There are two ways in:

- **From the remote:** hold its blue display for 2 s, enter PIN **4817**, then confirm.
- **At the receiver:** power-cycle it 5 times quickly, keeping each power-on under 2 s.

Then join WiFi **`Hazer-RX-Setup-<MAC>`** (password **`48174817`**) and open **http://192.168.4.1**. The page has:

- DMX base address
- Fail-safe timeout
- Radio channel
- Allowed remote MAC (blank = any)
- Bench test sliders
- Restore defaults and Exit

Setup mode ends after 10 minutes without activity.

## Building and flashing

Use VS Code with PlatformIO, env `esp32c3`. Clone both repos side by side:

```
git clone https://github.com/JStutsman/Blizzard-AtmosFEAR-Tour-Hazer-ESP32-RC.git remote-controller
git clone https://github.com/JStutsman/Blizzard-AtmosFEAR-Tour-Hazer-ESP32-RC-Receiver.git receiver
pio run -d receiver -t upload --upload-port COMxx
```

Serial output runs over the C3's native USB at 115200 baud. At boot the receiver prints its MAC, which goes into the remote's `Config.h` (`RX_MAC_RECEIVER_1` / `_2`). For the Arduino IDE, use board *ESP32C3 Dev Module* with *USB CDC On Boot* enabled, on ESP32 core 3.x.

### Configuration (`Hazer-RX/Config.h`)

| Setting | Default | Notes |
|---|---|---|
| `DMX_DI_PIN`, `DMX_EN_PIN` | 10, 7 | RS485 wiring |
| `STATUS_LED_PIN`, `STATUS_LED_ACTIVE_LOW` | 8, true | C3 SuperMini LED |
| `DEFAULT_FAILSAFE_MS` | 3000 | Also adjustable on the setup page |
| `OWNER_TIMEOUT_MS` | 3000 | When another remote may take control |
| `SETUP_PASSWORD` | 48174817 | Setup WiFi password |
| `ALLOWED_TX_MAC` | all zeros | Lock to one remote (zeros = accept any) |

## Code layout

```
Hazer-RX/
  Hazer-RX.ino     main loop, LED state
  Config.h         pins and defaults
  HazerProtocol.h  wire protocol - must stay identical to the remote's copy
  Link.*           ESP-NOW receive, ownership, fail-safe, status replies
  DmxOut.*         continuous DMX512 output
  Settings.*       settings in flash
  ConfigMode.*     setup WiFi and web page
  BootCounter.*    5-power-cycle setup entry
  StatusLed.*      LED patterns
docs/
  Hazer-Remote-User-Guide.pdf
```
