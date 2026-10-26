// BootCounter.cpp  (Hazer-RX)
// Counts consecutive quick power-ups. Only true power-on resets are counted, so
// uploads, crashes, watchdog and software restarts never trigger config mode.
#include <Arduino.h>
#include <Preferences.h>
#include <esp_system.h>
#include "Config.h"
#include "BootCounter.h"

static Preferences bp;
static uint8_t  count = 0;
static bool     settled = false;
static uint32_t t0 = 0;

bool bootCounterBegin() {
  bp.begin("hzboot", false);
  t0 = millis();
  bool enterConfig = false;

  if (esp_reset_reason() == ESP_RST_POWERON) {
    count = bp.getUChar("cnt", 0) + 1;
    if (count >= BOOT_CYCLES_FOR_CONFIG) {
      enterConfig = true;
      count = 0;
      bp.putUChar("cnt", 0);
    } else {
      bp.putUChar("cnt", count);
    }
  } else {
    count = 0;
    bp.putUChar("cnt", 0);
  }

  // Not counting (or config triggered) -> no need to hold the outputs back.
  settled = (count == 0);
  return enterConfig;
}

void bootCounterLoop(uint32_t now) {
  if (!settled && (now - t0) >= BOOT_SETTLE_MS) {
    count = 0;
    bp.putUChar("cnt", 0);
    settled = true;
  }
}

bool bootSettled() { return settled; }
uint8_t bootCount() { return count; }
