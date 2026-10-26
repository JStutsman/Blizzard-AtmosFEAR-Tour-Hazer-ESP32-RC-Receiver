# Hazer Remote

Wireless remote for a Blizzard AtmosFEAR Tour HZ haze machine.

- **Hazer-TX** - ESP32-2432S028R ("CYD") touchscreen. The screen mimics the machine's front panel.
- **Hazer-RX** - ESP32-C3 + RS485 module wired to the hazer's DMX input. Outputs continuous DMX512.
- **Link** - ESP-NOW (no router, no pairing step), fixed WiFi channel 6.

## Repositories

The project is split into two repos:

- Remote (Hazer-TX): https://github.com/JStutsman/Blizzard-AtmosFEAR-Tour-Hazer-ESP32-RC
- Receiver (Hazer-RX): https://github.com/JStutsman/Blizzard-AtmosFEAR-Tour-Hazer-ESP32-RC-Receiver

Paths below assume both are cloned side by side:

```
git clone https://github.com/JStutsman/Blizzard-AtmosFEAR-Tour-Hazer-ESP32-RC.git remote-controller
git clone https://github.com/JStutsman/Blizzard-AtmosFEAR-Tour-Hazer-ESP32-RC-Receiver.git receiver
```

Status: written but NOT compiled or tested on hardware. Expect to fix small things on first build.

## Folders

```
Hazer-TX/                     (open Hazer-TX.ino)
  Hazer-TX.ino   main loop
  Config.h       defines: RX MAC, PIN, radio channel, display options
  FixtureTable.h device list (hazer only for now - groundwork for the hub idea)
  Display.*      LovyanGFX display/touch setup
  Radio.*        ESP-NOW send + status receive
  HazerState.*   panel logic: menus, timer, saved settings
  UiPanel.*      panel screen (blue LCD + 6 buttons)
  UiConfig.*     PIN entry -> confirm -> send "enter config mode"
  HazerProtocol.h

Hazer-RX/                     (open Hazer-RX.ino)
  Hazer-RX.ino   main loop
  Config.h       defines: RS485 pins, LED pin, config timing, setup WiFi password
  Settings.*     settings in flash (base address, fail-safe, channel, allowed TX MAC)
  BootCounter.*  5-power-cycle config entry
  DmxOut.*       continuous DMX512 output
  Link.*         ESP-NOW receive, fail-safe, status replies
  ConfigMode.*   setup WiFi + web page
  StatusLed.*    LED blink patterns
  HazerProtocol.h
```

`HazerProtocol.h` exists in both folders and must stay identical.

## Setup (VS Code + PlatformIO)

Open `Hazer-Remote.code-workspace`. It has two PlatformIO projects:

- `remote-controller/` - env `cyd` (ESP32 Dev Module, pulls LovyanGFX automatically)
- `receiver/` - env `esp32c3` (ESP32-C3, serial over native USB)

Each `platformio.ini` points `src_dir` at the sketch folder, so the sketches
remain unchanged and still open in the Arduino IDE. Build, upload, or monitor from
the PlatformIO toolbar or sidebar, or run `pio run -d receiver -t upload`.

## Setup (Visual Micro / Arduino)

1. Board: **ESP32 Dev Module** for the CYD, **ESP32C3 Dev Module** for the receiver (turn on *USB CDC On Boot* for the C3 if you want the serial monitor over its USB port).
2. Install the **LovyanGFX** library (TX only). Everything else ships with the ESP32 core.
3. Open each `.ino`; the other files in the folder are picked up automatically.

## Fill in before use

Hazer-RX/Config.h
- `DMX_DI_PIN` (ESP TX -> RS485 DI), `DMX_EN_PIN` (DE+RE tied together, held HIGH), `DMX_RO_PIN` (unused, leave -1)
- `STATUS_LED_PIN` / `STATUS_LED_ACTIVE_LOW` (defaults are for a C3 SuperMini: GPIO8, active low)
- optional: `ALLOWED_TX_MAC` (can also be set on the config page)

Hazer-TX/Config.h
- `RX_MAC_BYTES` - the receiver's MAC (it prints on the receiver's serial monitor at boot and shows on its config page)
- `HZ_CONFIG_PIN` is **4817**

Set the hazer's own DMX address to **510** (DMX512 Add in its menu). The receiver defaults to base address 510 (emit 510, fan 511, volume 512).

## Suggested test order

1. **Receiver alone:** flash Hazer-RX, wire the RS485 module, power-cycle it 5 times quickly (each power-on shorter than 2 s). The LED flickers fast = config mode. Join the WiFi `Hazer-RX-Setup-<MAC>` (password `48174817`), open `http://192.168.4.1`, use the **Bench test** sliders to confirm the hazer responds. No remote needed.
2. **Link:** put the receiver's MAC in the remote's Config.h, flash the remote. The blue LCD should change from "No Link" to "Ready Haze" and show `RX v1.0`, dBm and the echoed values.
3. **Panel:** try START/STOP, MANUAL (hold), TIMER, and the FUNCTION menus (the hazer's own pages). The gear icon opens Settings: receivers, DMX addresses, display mode and backlight.
4. **Config via remote:** hold the blue LCD area for 2 seconds -> PIN -> confirm.

## How it behaves

- Remote sends 3 values (emit, fan, volume) at 20 Hz. The receiver adds its base address (relative addressing).
- Receiver zeroes all outputs if no data arrives for 3 s, and holds zero for the first 2 s after power-up and during config mode.
- Receiver replies about 4x/s with: firmware version, its RSSI, the values it is outputting, and flags (fail-safe, config mode, identify).
- Config mode ends by itself after 10 minutes without activity, or with the page's Exit button.
- PIN: 4 digits, 3 wrong tries locks entry for 5 s. PIN 1234 opens a debug/info screen.
- One remote controls a receiver at a time. Other remotes are ignored and show "In use" until the controlling remote is silent for 3 s or deselects it; tapping the info line on the remote offers to take control right away.
- LED patterns (receiver): N blinks while counting power cycles; one short blink = linked; double blink = fail-safe/no data; fast flicker = config mode; slow blink = identify.

## Packet commands (HazerProtocol.h)

DATA, SAFE, ENTER_CONFIG, SET_BASE, IDENTIFY, TAKE_CONTROL, and STATUS (receiver -> remote). SET_BASE is sent from the remote's Settings screen (gear icon, top-left of the blue LCD; the receiver saves it to flash). The remote remembers the address set for each receiver and resends it whenever that receiver reports a different one, so a receiver that was offline catches up when it is back. IDENTIFY is handled by the receiver but the remote has no UI for it yet.

## Known assumptions to check on first run

- Core 3.x ESP-NOW callback signature (`esp_now_recv_info_t`) is used on both sides.
- If the CYD display is blank, change `Panel_ILI9341` to `Panel_ST7789` in Display.h.
- Receiver config mode switches WiFi to AP+STA while ESP-NOW keeps running; verify the remote still sees status replies in config mode.
- The DMX break uses the UART driver's trailing break (24 bit-times = 96 us).
