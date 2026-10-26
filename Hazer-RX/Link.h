// Link.h  (Hazer-RX) - ESP-NOW link to the remote
#pragma once
#include <Arduino.h>

// Set by main loop: true only when outputs may be driven (boot settled, not in config mode).
extern volatile bool g_outputsAllowed;

void     linkBegin();                   // WiFi STA + ESP-NOW on the configured channel
void     linkLoop(uint32_t now);        // fail-safe handling + heartbeat/status replies
bool     linkConfigRequested();         // remote asked us to enter config mode
void     linkClearConfigRequest();
bool     linkFailsafe();                // true while outputs are being held at zero due to no data
bool     linkIdentifyActive();
uint32_t linkLastPacketAgeMs();         // ms since last valid packet (0xFFFFFFFF = never)
void     linkLog(uint32_t now);         // prints one status line per second to Serial
