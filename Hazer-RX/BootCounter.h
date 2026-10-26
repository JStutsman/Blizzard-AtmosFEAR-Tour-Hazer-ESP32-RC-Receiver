// BootCounter.h  (Hazer-RX) - detects the "power cycle N times" config sequence
#pragma once
#include <Arduino.h>

bool    bootCounterBegin();          // call early in setup(); returns true if the config sequence was completed
void    bootCounterLoop(uint32_t now); // call in loop(); clears the counter after BOOT_SETTLE_MS
bool    bootSettled();               // true once the boot window is over (outputs may be enabled)
uint8_t bootCount();                 // current count (for LED blink feedback)
