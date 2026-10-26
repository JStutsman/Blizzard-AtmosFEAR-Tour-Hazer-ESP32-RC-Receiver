// Link.cpp  (Hazer-RX)
// Receives HzPacket frames from the remote, writes values into the DMX frame using
// RELATIVE addressing (DMX channel = cfg.base + value index), enforces the fail-safe,
// and replies with HzStatus (heartbeat, version, RSSI, value echo, flags).
#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include "Config.h"
#include "HazerProtocol.h"
#include "Settings.h"
#include "DmxOut.h"
#include "ConfigMode.h"
#include "Link.h"

volatile bool g_outputsAllowed = false;

static uint8_t           txMac[6];
static volatile bool     haveTx = false;
static volatile bool     cfgReq = false;
static volatile uint16_t pendingBase = 0;
static volatile uint32_t lastDataMs = 0;
static volatile uint32_t lastAnyMs = 0;
static volatile uint32_t identifyUntil = 0;
static volatile bool     everData = false;
static volatile bool     everAny = false;
static volatile uint8_t  lastSeq = 0;
static volatile int8_t   lastRssi = 0;
static bool              failsafe = true;
static bool              espnowOk = false;

// ---------------- receive callback (runs in the WiFi task) ----------------
static void onRecv(const esp_now_recv_info_t* info, const uint8_t* data, int len) {
  if (len != (int)sizeof(HzPacket)) return;
  const HzPacket* p = (const HzPacket*)data;
  if (p->magic != HZ_MAGIC || p->version != HZ_PROTOCOL_VERSION) return;
  if (settingsMacIsSet() && memcmp(info->src_addr, cfg.txMac, 6) != 0) return;

  memcpy(txMac, info->src_addr, 6);
  haveTx   = true;
  lastRssi = info->rx_ctrl->rssi;
  lastSeq  = p->seq;
  lastAnyMs = millis();
  everAny   = true;

  switch (p->cmd) {
    case HZ_CMD_DATA: {
      if (!g_outputsAllowed) break;
      uint8_t n = p->count > HZ_MAX_VALUES ? HZ_MAX_VALUES : p->count;
      for (uint8_t i = 0; i < n; i++) {
        uint16_t ch = cfg.base + i;
        if (ch <= 512) dmxSetChannel(ch, p->values[i]);
      }
      lastDataMs = millis();
      everData = true;
      break;
    }
    case HZ_CMD_SAFE:
      dmxZeroAll();
      everData = false;           // back to fail-safe until new data arrives
      break;
    case HZ_CMD_ENTER_CONFIG:
      cfgReq = true;
      break;
    case HZ_CMD_SET_BASE:
      if (p->arg >= 1 && p->arg <= 510) pendingBase = p->arg;
      break;
    case HZ_CMD_IDENTIFY:
      identifyUntil = millis() + 3000;
      break;
    default:
      break;
  }
}

// ---------------- status reply ----------------
static void ensurePeer(const uint8_t* mac) {
  if (esp_now_is_peer_exist(mac)) return;
  esp_now_peer_info_t pi = {};
  memcpy(pi.peer_addr, mac, 6);
  pi.channel = 0;                 // current channel
  pi.encrypt = false;
  esp_now_add_peer(&pi);
}

static void sendStatus() {
  HzStatus s = {};
  s.magic    = HZ_MAGIC;
  s.version  = HZ_PROTOCOL_VERSION;
  s.cmd      = HZ_CMD_STATUS;
  s.seq      = lastSeq;
  s.fwMajor  = FW_VERSION_MAJOR;
  s.fwMinor  = FW_VERSION_MINOR;
  s.flags    = 0;
  if (failsafe)               s.flags |= HZ_FLAG_FAILSAFE;
  if (configModeActive())     s.flags |= HZ_FLAG_CONFIG;
  if (linkIdentifyActive())   s.flags |= HZ_FLAG_IDENTIFY;
  s.count    = HZ_HAZER_VALUES;
  for (uint8_t i = 0; i < HZ_HAZER_VALUES; i++) s.echo[i] = dmxGetChannel(cfg.base + i);
  s.baseAddr = cfg.base;
  s.rxRssi   = lastRssi;

  uint8_t mac[6];
  memcpy(mac, txMac, 6);
  ensurePeer(mac);
  esp_now_send(mac, (const uint8_t*)&s, sizeof(s));
}

// ---------------- public ----------------
void linkBegin() {
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  esp_wifi_set_channel(cfg.channel, WIFI_SECOND_CHAN_NONE);

  Serial.printf("[LINK] MAC: %s  channel: %u\n", WiFi.macAddress().c_str(), cfg.channel);

  if (esp_now_init() != ESP_OK) {
    Serial.println("[LINK] ESP-NOW init FAILED");
    return;
  }
  esp_now_register_recv_cb(onRecv);
  espnowOk = true;
  Serial.printf("[LINK] ready. base DMX address %u (channels %u-%u)\n",
                cfg.base, cfg.base, cfg.base + HZ_HAZER_VALUES - 1);
}

void linkLoop(uint32_t now) {
  if (pendingBase) {
    uint16_t b = pendingBase;
    pendingBase = 0;
    if (b != cfg.base) {            // the remote repeats SET_BASE until confirmed; save once
      cfg.base = b;
      settingsSave();
      dmxZeroAll();
      Serial.printf("[LINK] new base DMX address %u (channels %u-%u)\n",
                    cfg.base, cfg.base, cfg.base + HZ_HAZER_VALUES - 1);
    }
  }

  failsafe = !everData || (now - lastDataMs) > cfg.failsafeMs;

  // Hold outputs at zero whenever data is stale or outputs are not allowed.
  static bool zeroed = false;
  bool hold = failsafe || !g_outputsAllowed;
  if (hold) {
    if (!zeroed) { dmxZeroAll(); zeroed = true; }
  } else {
    zeroed = false;
  }

  static uint32_t lastStatus = 0;
  if (espnowOk && haveTx && (now - lastStatus) >= STATUS_INTERVAL_MS) {
    lastStatus = now;
    sendStatus();
  }
}

// One line per second: state, haze on/off, fan %, volume % (the values actually being output).
void linkLog(uint32_t now) {
  static uint32_t lastLog = 0;
  if ((now - lastLog) < 1000) return;
  lastLog = now;

  const char* state;
  if (configModeActive())      state = "CONFIG";
  else if (!g_outputsAllowed)  state = "BOOT-HOLD";
  else if (failsafe)           state = "FAILSAFE";
  else                         state = "RUN";

  uint8_t e = dmxGetChannel(cfg.base + HZV_EMIT);
  uint8_t f = dmxGetChannel(cfg.base + HZV_FAN);
  uint8_t v = dmxGetChannel(cfg.base + HZV_VOL);
  Serial.printf("[RX] %lus state=%s haze=%s fan=%d%% vol=%d%% (ch %u-%u = %u/%u/%u)\n",
                (unsigned long)(now / 1000), state, e >= 128 ? "ON" : "OFF",
                (f * 100 + 127) / 255, (v * 100 + 127) / 255,
                cfg.base, cfg.base + HZ_HAZER_VALUES - 1, e, f, v);
}

bool linkConfigRequested()     { return cfgReq; }
void linkClearConfigRequest()  { cfgReq = false; }
bool linkFailsafe()            { return failsafe; }
bool linkIdentifyActive()      { return (int32_t)(identifyUntil - millis()) > 0 && identifyUntil != 0; }
uint32_t linkLastPacketAgeMs() { return everAny ? (millis() - lastAnyMs) : 0xFFFFFFFFUL; }
