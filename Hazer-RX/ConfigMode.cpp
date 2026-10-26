// ConfigMode.cpp  (Hazer-RX)
// Config mode: the receiver starts its own WiFi network and serves a settings page at
// http://192.168.4.1 . Outputs stay at zero except the bench-test sliders, which have
// a dead-man watchdog (they zero themselves if the browser stops pinging).
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include "Config.h"
#include "HazerProtocol.h"
#include "Settings.h"
#include "DmxOut.h"
#include "Link.h"
#include "ConfigMode.h"

static WebServer server(80);
static bool     active = false;
static uint32_t lastActivity = 0;
static uint32_t benchMs = 0;          // time of last bench ping (0 = bench idle)

bool configModeActive() { return active; }

static void touchActivity() { lastActivity = millis(); }

static String macToStr(const uint8_t* m) {
  char b[18];
  snprintf(b, sizeof(b), "%02X:%02X:%02X:%02X:%02X:%02X", m[0], m[1], m[2], m[3], m[4], m[5]);
  return String(b);
}

// ---------------- page ----------------
static String buildPage(const String& notice, bool isError) {
  String s;
  s.reserve(5000);
  s += F("<!doctype html><html><head><meta charset='utf-8'>"
         "<meta name='viewport' content='width=device-width,initial-scale=1'>"
         "<title>Hazer-RX Setup</title><style>"
         "body{font-family:sans-serif;background:#10151c;color:#e8edf2;margin:0;padding:16px;max-width:520px}"
         "h2{margin:0 0 4px}h3{margin:22px 0 6px;border-top:1px solid #2a3340;padding-top:14px}"
         ".meta{color:#8fa0b3;font-size:13px;line-height:1.6}"
         "label{display:block;margin:12px 0 4px;font-size:14px}"
         "input[type=number],input[type=text]{width:100%;box-sizing:border-box;padding:9px;"
         "background:#1b2430;color:#e8edf2;border:1px solid #33404f;border-radius:6px;font-size:16px}"
         "input[type=range]{width:100%}"
         "button{margin-top:14px;padding:11px 18px;border:0;border-radius:6px;font-size:16px;"
         "background:#2f7de1;color:#fff}"
         "button.warn{background:#a8432f}button.grey{background:#46515f}"
         ".note{padding:10px;border-radius:6px;margin:12px 0;background:#1f5a34}"
         ".err{background:#7a2b22}.small{font-size:12px;color:#8fa0b3}"
         "</style></head><body>");

  s += F("<h2>Hazer-RX Setup</h2><div class='meta'>");
  s += "Firmware v" + String(FW_VERSION_MAJOR) + "." + String(FW_VERSION_MINOR);
  s += " &middot; MAC " + WiFi.macAddress();
  s += "<br>Uptime " + String(millis() / 1000) + " s";
  uint32_t age = linkLastPacketAgeMs();
  s += "<br>Last packet from remote: ";
  s += (age == 0xFFFFFFFFUL) ? String("none yet") : (String(age) + " ms ago");
  s += F("</div>");

  if (notice.length()) {
    s += isError ? F("<div class='note err'>") : F("<div class='note'>");
    s += notice;
    s += F("</div>");
  }

  s += F("<h3>Settings</h3><form method='POST' action='/save'>");
  s += F("<label>DMX base address (1-510). Emit = base, fan = base+1, volume = base+2</label>"
         "<input type='number' name='base' min='1' max='510' value='");
  s += String(cfg.base);
  s += F("'><label>Fail-safe timeout (seconds, 0.5-60)</label>"
         "<input type='number' name='fs' min='0.5' max='60' step='0.5' value='");
  s += String(cfg.failsafeMs / 1000.0f, 1);
  s += F("'><label>Radio channel (1-13) &mdash; must match the remote</label>"
         "<input type='number' name='ch' min='1' max='13' value='");
  s += String(cfg.channel);
  s += F("'><label>Allowed remote MAC (blank = accept any)</label>"
         "<input type='text' name='mac' placeholder='AA:BB:CC:DD:EE:FF' value='");
  s += settingsMacIsSet() ? macToStr(cfg.txMac) : String("");
  s += F("'><button type='submit'>Save settings</button>"
         "<div class='small'>Radio channel changes apply after you exit config mode.</div></form>");

  s += F("<h3>Bench test</h3>"
         "<div class='small'>Drives the hazer directly from this page. Stops by itself if you close the page.</div>"
         "<label><input type='checkbox' id='e'> Emit haze</label>"
         "<label>Fan <span id='fv'>50</span>%</label>"
         "<input type='range' id='f' min='0' max='100' value='50' oninput=\"fv.textContent=this.value\">"
         "<label>Volume <span id='vv'>50</span>%</label>"
         "<input type='range' id='v' min='0' max='100' value='50' oninput=\"vv.textContent=this.value\">"
         "<script>"
         "function send(){fetch('/test?e='+(document.getElementById('e').checked?1:0)"
         "+'&f='+document.getElementById('f').value+'&v='+document.getElementById('v').value)"
         ".catch(function(){});}"
         "setInterval(send,1000);"
         "</script>");

  s += F("<h3>Maintenance</h3>"
         "<form method='POST' action='/defaults' onsubmit=\"return confirm('Restore all settings to defaults?')\">"
         "<button class='warn' type='submit'>Restore defaults</button></form>"
         "<form method='POST' action='/exit'>"
         "<button class='grey' type='submit'>Exit config mode (restart)</button></form>"
         "<div class='small' style='margin-top:14px'>Config mode also ends by itself after 10 minutes without activity.</div>"
         "</body></html>");
  return s;
}

// ---------------- handlers ----------------
static void handleRoot() {
  touchActivity();
  server.send(200, "text/html", buildPage("", false));
}

static void handleSave() {
  touchActivity();
  int   base = server.arg("base").toInt();
  float fs   = server.arg("fs").toFloat();
  int   ch   = server.arg("ch").toInt();
  String mac = server.arg("mac");
  mac.trim();

  String err;
  if (base < 1 || base > 510)  err += "Base address must be 1-510. ";
  if (fs < 0.5f || fs > 60.0f) err += "Fail-safe must be 0.5-60 seconds. ";
  if (ch < 1 || ch > 13)       err += "Radio channel must be 1-13. ";

  uint8_t m[6] = {0, 0, 0, 0, 0, 0};
  if (mac.length()) {
    unsigned u[6];
    if (sscanf(mac.c_str(), "%x:%x:%x:%x:%x:%x", &u[0], &u[1], &u[2], &u[3], &u[4], &u[5]) != 6) {
      err += "MAC must look like AA:BB:CC:DD:EE:FF. ";
    } else {
      for (int i = 0; i < 6; i++) m[i] = (uint8_t)u[i];
    }
  }

  if (err.length()) {
    server.send(400, "text/html", buildPage(err, true));
    return;
  }

  cfg.base       = (uint16_t)base;
  cfg.failsafeMs = (uint32_t)(fs * 1000.0f);
  cfg.channel    = (uint8_t)ch;
  memcpy(cfg.txMac, m, 6);
  settingsSave();
  dmxZeroAll();
  benchMs = 0;
  server.send(200, "text/html", buildPage("Saved.", false));
}

static void handleDefaults() {
  touchActivity();
  settingsRestoreDefaults();
  dmxZeroAll();
  benchMs = 0;
  server.send(200, "text/html", buildPage("Defaults restored.", false));
}

static void handleExit() {
  server.send(200, "text/html",
              "<html><body style='font-family:sans-serif'><h3>Restarting...</h3>"
              "<p>The setup WiFi will disappear. You can close this page.</p></body></html>");
  delay(300);
  ESP.restart();
}

static void handleTest() {
  touchActivity();
  bool    e = server.arg("e").toInt() != 0;
  int     f = constrain(server.arg("f").toInt(), 0, 100);
  int     v = constrain(server.arg("v").toInt(), 0, 100);
  dmxSetChannel(cfg.base + HZV_EMIT, e ? 255 : 0);
  dmxSetChannel(cfg.base + HZV_FAN,  (uint8_t)((f * 255) / 100));
  dmxSetChannel(cfg.base + HZV_VOL,  (uint8_t)((v * 255) / 100));
  benchMs = millis();
  server.send(200, "text/plain", "ok");
}

// ---------------- public ----------------
void configModeBegin() {
  if (active) return;
  dmxZeroAll();
  active = true;
  lastActivity = millis();
  benchMs = 0;

  String mac = WiFi.macAddress();
  mac.replace(":", "");
  String ssid = String(SETUP_SSID_PREFIX) + mac.substring(mac.length() - 6);

  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(ssid.c_str(), SETUP_PASSWORD, cfg.channel);

  server.on("/", HTTP_GET, handleRoot);
  server.on("/save", HTTP_POST, handleSave);
  server.on("/defaults", HTTP_POST, handleDefaults);
  server.on("/exit", HTTP_POST, handleExit);
  server.on("/test", HTTP_GET, handleTest);
  server.onNotFound([]() {
    server.sendHeader("Location", "/");
    server.send(302, "text/plain", "");
  });
  server.begin();

  Serial.printf("[CONFIG] setup WiFi \"%s\"  http://%s\n", ssid.c_str(), WiFi.softAPIP().toString().c_str());
}

void configModeLoop(uint32_t now) {
  if (!active) return;
  server.handleClient();

  // Dead-man watchdog for the bench-test sliders.
  if (benchMs != 0 && (now - benchMs) > 3000) {
    dmxZeroAll();
    benchMs = 0;
  }

  if ((now - lastActivity) > CONFIG_TIMEOUT_MS) {
    Serial.println("[CONFIG] timeout - restarting");
    ESP.restart();
  }
}
