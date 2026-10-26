// ConfigMode.h  (Hazer-RX) - setup WiFi access point + web page
#pragma once
#include <Arduino.h>

void configModeBegin();            // zero outputs, start AP "Hazer-RX-Setup-<MAC>" and web server
void configModeLoop(uint32_t now); // serve web requests, bench-test watchdog, inactivity timeout
bool configModeActive();
