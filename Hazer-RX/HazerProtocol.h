// HazerProtocol.h
// Shared wire protocol for Hazer-TX (remote) and Hazer-RX (receiver).
// IMPORTANT: Hazer-TX/HazerProtocol.h and Hazer-RX/HazerProtocol.h must stay IDENTICAL.
#pragma once
#include <stdint.h>

#define HZ_MAGIC             0xA5
#define HZ_PROTOCOL_VERSION  1
#define HZ_MAX_VALUES        16
#define HZ_DEFAULT_CHANNEL   6      // WiFi channel used by ESP-NOW
#define HZ_DEFAULT_BASE      510    // receiver's default DMX base address (510..512)

// Value indexes for the hazer fixture (relative addressing: base + index = DMX channel)
#define HZV_EMIT        0           // >=128 = haze on
#define HZV_FAN         1           // 0-255
#define HZV_VOL         2           // 0-255
#define HZ_HAZER_VALUES 3

// Commands
enum HzCmd : uint8_t {
  HZ_CMD_DATA         = 1,   // TX -> RX: set values
  HZ_CMD_SAFE         = 2,   // TX -> RX: zero outputs now
  HZ_CMD_ENTER_CONFIG = 3,   // TX -> RX: stop output and start setup WiFi
  HZ_CMD_SET_BASE     = 4,   // TX -> RX: arg = new base DMX address (saved to flash)
  HZ_CMD_IDENTIFY     = 5,   // TX -> RX: blink the receiver LED for a few seconds
  HZ_CMD_STATUS       = 0x80 // RX -> TX: status reply / heartbeat
};

// Status flag bits (HzStatus.flags)
enum HzStatusFlags : uint8_t {
  HZ_FLAG_FAILSAFE       = 0x01,  // receiver is in fail-safe (outputs zeroed)
  HZ_FLAG_CONFIG         = 0x02,  // receiver is in config mode
  HZ_FLAG_RESERVED_WARMUP = 0x04, // reserved (not used yet)
  HZ_FLAG_IDENTIFY       = 0x08   // identify blink active
};

#pragma pack(push, 1)
// TX -> RX
struct HzPacket {
  uint8_t  magic;
  uint8_t  version;
  uint8_t  cmd;
  uint8_t  seq;
  uint8_t  count;                 // number of valid entries in values[]
  uint16_t arg;                   // command argument (SET_BASE)
  uint8_t  values[HZ_MAX_VALUES];
};

// RX -> TX
struct HzStatus {
  uint8_t  magic;
  uint8_t  version;
  uint8_t  cmd;                   // HZ_CMD_STATUS
  uint8_t  seq;                   // last sequence number received from TX
  uint8_t  fwMajor;
  uint8_t  fwMinor;
  uint8_t  flags;                 // HzStatusFlags
  uint8_t  count;                 // number of valid echo values
  uint8_t  echo[HZ_MAX_VALUES];   // values the receiver is actually outputting
  uint16_t baseAddr;              // receiver's current DMX base address
  int8_t   rxRssi;                // RSSI (dBm) of the last packet as seen by the receiver
};
#pragma pack(pop)
