// Settings.h  (Hazer-RX) - settings stored in flash
#pragma once
#include <Arduino.h>

struct RxSettings {
  uint16_t base;         // DMX base address (1..510)
  uint32_t failsafeMs;   // fail-safe timeout
  uint8_t  channel;      // ESP-NOW / WiFi channel (1..13)
  uint8_t  txMac[6];     // allowed transmitter MAC (all zeros = any)
};

extern RxSettings cfg;

void settingsBegin();            // load from flash (or defaults)
void settingsSave();             // write current cfg to flash
void settingsRestoreDefaults();  // reset cfg to defaults and save
bool settingsMacIsSet();         // true if an allowed transmitter MAC is configured
