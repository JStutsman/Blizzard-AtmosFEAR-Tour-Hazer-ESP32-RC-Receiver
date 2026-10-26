// Hazer-RX.ino
// Receiver for the Hazer remote: ESP-NOW in, continuous DMX512 out (RS485) to the
// AtmosFEAR Tour HZ. ESP32-C3 + RS485 module.
//
// Files: Config.h (pins/defines), Settings, BootCounter, DmxOut, Link, ConfigMode,
//        StatusLed, HazerProtocol.h (shared with the remote)

#include "Config.h"
#include "HazerProtocol.h"
#include "Settings.h"
#include "BootCounter.h"
#include "DmxOut.h"
#include "Link.h"
#include "ConfigMode.h"
#include "StatusLed.h"

void setup() {
  Serial.begin(115200);
  delay(100);
  Serial.printf("\n[RX] Hazer-RX v%d.%d\n", FW_VERSION_MAJOR, FW_VERSION_MINOR);

  settingsBegin();
  ledBegin();
  bool configByPower = bootCounterBegin();   // true after BOOT_CYCLES_FOR_CONFIG quick power cycles
  dmxBegin();                                // outputs start at zero
  linkBegin();

  if (configByPower) {
    Serial.println("[RX] power-cycle sequence detected -> config mode");
    configModeBegin();
  }
}

void loop() {
  uint32_t now = millis();

  bootCounterLoop(now);
  g_outputsAllowed = bootSettled() && !configModeActive();

  linkLoop(now);
  linkLog(now);

  if (linkConfigRequested()) {
    linkClearConfigRequest();
    if (!configModeActive()) {
      Serial.println("[RX] config requested by remote");
      configModeBegin();
    }
  }
  if (configModeActive()) configModeLoop(now);

  uint16_t pat;
  if (!bootSettled())              pat = ledBootPattern(bootCount());
  else if (configModeActive())     pat = LED_PAT_CONFIG;
  else if (linkIdentifyActive())   pat = LED_PAT_IDENT;
  else if (linkFailsafe())         pat = LED_PAT_NOLINK;
  else                             pat = LED_PAT_LINKED;
  ledSetPattern(pat);
  ledUpdate(now);

  delay(2);
}
