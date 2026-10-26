// StatusLed.cpp  (Hazer-RX)
#include <Arduino.h>
#include "Config.h"
#include "StatusLed.h"

static uint16_t pattern = 0;

static void ledWrite(bool on) {
  if (STATUS_LED_PIN < 0) return;
  digitalWrite(STATUS_LED_PIN, (on != STATUS_LED_ACTIVE_LOW) ? HIGH : LOW);
}

void ledBegin() {
  if (STATUS_LED_PIN >= 0) pinMode(STATUS_LED_PIN, OUTPUT);
  ledWrite(false);
}

void ledSetPattern(uint16_t mask) { pattern = mask; }

void ledUpdate(uint32_t now) {
  uint8_t step = (now / 100) % 16;
  ledWrite((pattern >> step) & 1);
}

uint16_t ledBootPattern(uint8_t n) {
  uint16_t m = 0;
  for (uint8_t i = 0; i < n && i < 8; i++) m |= (1u << (2 * i));
  return m;
}
