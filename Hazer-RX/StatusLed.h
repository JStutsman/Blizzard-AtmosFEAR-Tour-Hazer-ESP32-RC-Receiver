// StatusLed.h  (Hazer-RX) - single status LED, blink patterns
#pragma once
#include <Arduino.h>

// 16 steps of 100 ms each (1.6 s period). Bit n set = LED on during step n.
#define LED_PAT_LINKED  0x0003   // one short blink: linked, all good
#define LED_PAT_NOLINK  0x0033   // double blink: fail-safe / no data
#define LED_PAT_CONFIG  0x5555   // fast flicker: config mode
#define LED_PAT_IDENT   0x00FF   // slow blink: identify

void     ledBegin();
void     ledSetPattern(uint16_t mask);
void     ledUpdate(uint32_t now);
uint16_t ledBootPattern(uint8_t n);   // n quick blinks (boot counter feedback)
