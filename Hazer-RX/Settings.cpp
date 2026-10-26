// Settings.cpp  (Hazer-RX)
#include <Arduino.h>
#include <Preferences.h>
#include "Config.h"
#include "HazerProtocol.h"
#include "Settings.h"

RxSettings cfg;
static Preferences prefs;
static const uint8_t DEFAULT_TX_MAC[6] = ALLOWED_TX_MAC;

static void setDefaults() {
  cfg.base       = HZ_DEFAULT_BASE;
  cfg.failsafeMs = DEFAULT_FAILSAFE_MS;
  cfg.channel    = HZ_DEFAULT_CHANNEL;
  memcpy(cfg.txMac, DEFAULT_TX_MAC, 6);
}

static void clampSettings() {
  if (cfg.base < 1 || cfg.base > 510)                 cfg.base = HZ_DEFAULT_BASE;
  if (cfg.failsafeMs < 500 || cfg.failsafeMs > 60000) cfg.failsafeMs = DEFAULT_FAILSAFE_MS;
  if (cfg.channel < 1 || cfg.channel > 13)            cfg.channel = HZ_DEFAULT_CHANNEL;
}

void settingsBegin() {
  setDefaults();
  prefs.begin("hzrx", false);
  cfg.base       = prefs.getUShort("base", cfg.base);
  cfg.failsafeMs = prefs.getUInt("failsafe", cfg.failsafeMs);
  cfg.channel    = prefs.getUChar("channel", cfg.channel);
  if (prefs.getBytesLength("txmac") == 6) prefs.getBytes("txmac", cfg.txMac, 6);
  clampSettings();
}

void settingsSave() {
  clampSettings();
  prefs.putUShort("base", cfg.base);
  prefs.putUInt("failsafe", cfg.failsafeMs);
  prefs.putUChar("channel", cfg.channel);
  prefs.putBytes("txmac", cfg.txMac, 6);
}

void settingsRestoreDefaults() {
  setDefaults();
  settingsSave();
}

bool settingsMacIsSet() {
  for (int i = 0; i < 6; i++) if (cfg.txMac[i] != 0) return true;
  return false;
}
