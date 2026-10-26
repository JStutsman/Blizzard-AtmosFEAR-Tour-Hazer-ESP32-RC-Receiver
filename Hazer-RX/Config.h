// Config.h  (Hazer-RX)
// All compile-time settings for the receiver live here.
#pragma once

#define FW_VERSION_MAJOR 1
#define FW_VERSION_MINOR 1

// ---------------- RS485 pins: FILL THESE IN ----------------
// Use GPIO numbers for your ESP32-C3 board. Leave -1 until wired.
#define DMX_DI_PIN    10    // ESP32 TX  -> RS485 module DI  (data to the bus)
#define DMX_RO_PIN    -1    // RS485 RO  -> ESP32 RX         (unused; leave -1)
#define DMX_EN_PIN    7     // RS485 DE and RE tied together (held HIGH = transmit)
#define DMX_UART_NUM  1     // UART1 (UART0 is used by the USB serial port)

// ---------------- status LED ----------------
#define STATUS_LED_PIN         8      // ESP32-C3 SuperMini onboard LED (GPIO8). Change if needed.
#define STATUS_LED_ACTIVE_LOW  true   // SuperMini LED lights when the pin is LOW

// ---------------- config mode entry (power cycling) ----------------
#define BOOT_CYCLES_FOR_CONFIG  5      // quick power cycles needed
#define BOOT_SETTLE_MS          2000   // power must stay on this long to clear the counter
#define CONFIG_TIMEOUT_MS       (10UL * 60UL * 1000UL)  // leave config mode after this much inactivity

// ---------------- behaviour ----------------
#define DEFAULT_FAILSAFE_MS     3000   // zero outputs if no data packet for this long
#define STATUS_INTERVAL_MS      250    // heartbeat / status reply rate
#define OWNER_TIMEOUT_MS        3000   // another remote may take over after the owner is silent this long

// ---------------- setup WiFi (config mode) ----------------
#define SETUP_SSID_PREFIX   "Hazer-RX-Setup-"   // MAC suffix is appended
#define SETUP_PASSWORD      "48174817"          // PIN x2 (min 8 characters)

// ---------------- transmitter allowlist ----------------
// MAC address of the paired remote. All zeros = accept packets from any sender.
// Can also be changed on the config web page.
#define ALLOWED_TX_MAC  {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}
