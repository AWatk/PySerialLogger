#include <Arduino.h>
#include "Communication.h"

// === CONFIG ===
static const int LED_PIN = 2; // onboard LED on many ESP32 dev boards

// === LED STATE ===
static int  ledMode = 0;            // 0=off, 1=on, 2=blinking
static bool ledCurrentLevel = LOW;  // current digital state of LED_PIN
static unsigned long blinkIntervalMs = 500; // default 0.5s between toggles
static unsigned long lastToggleMs = 0;

// === Communication object ===
Communication comm;

// ================= HANDLERS =================

// <led,1>  = LED on solid
// <led,0>  = LED off solid
void HandleLed(char** inputs) {
  // inputs[0] = "led"
  // inputs[1] = "1" or "0"
  if (inputs[1] == nullptr) {
    Serial.println(F("ERR,led,ARGS"));
    return;
  }

  int state = atoi(inputs[1]); // 0 or nonzero
  if (state != 0) {
    ledMode = 1;               // solid on
    ledCurrentLevel = HIGH;
  } else {
    ledMode = 0;               // solid off
    ledCurrentLevel = LOW;
  }

  digitalWrite(LED_PIN, ledCurrentLevel);

  Serial.print(F("OK,led,mode="));
  Serial.print(ledMode);
  Serial.print(F(",level="));
  Serial.println(ledCurrentLevel == HIGH ? 1 : 0);
}

// <blink>
// no args; just start blinking at default speed (0.5s per toggle)
void HandleBlinkDefault(char** inputs) {
  ledMode = 2;                 // blinking mode
  blinkIntervalMs = 500;       // 500ms between toggles
  lastToggleMs = millis();     // reset timer so it starts clean

  Serial.print(F("OK,blink,periodMs="));
  Serial.println(blinkIntervalMs);
}

// <blinkp,seconds>
// e.g. <blinkp,1.0> = toggle every 1.0 seconds
void HandleBlinkPeriod(char** inputs) {
  if (inputs[1] == nullptr) {
    Serial.println(F("ERR,blinkp,ARGS"));
    return;
  }

  float seconds = atof(inputs[1]);      // read seconds as float
  if (seconds < 0.01f) seconds = 0.01f; // clamp: don't allow zero
  if (seconds > 10.0f) seconds = 10.0f; // clamp: don't allow super long

  blinkIntervalMs = (unsigned long)(seconds * 1000.0f);
  ledMode = 2;
  lastToggleMs = millis();

  Serial.print(F("OK,blinkp,periodMs="));
  Serial.println(blinkIntervalMs);
}

// optional: <r> to report status
// tells you mode, interval, etc.
void HandleReport(char** inputs) {
  Serial.print(F("OK,status,mode="));
  Serial.print(ledMode);                // 0=off 1=on 2=blinking
  Serial.print(F(",level="));
  Serial.print(ledCurrentLevel ? 1 : 0);
  Serial.print(F(",intervalMs="));
  Serial.println(blinkIntervalMs);
}

// ================ SETUP / LOOP =================

void setup() {
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  ledCurrentLevel = LOW;
  ledMode = 0;
  blinkIntervalMs = 500;
  lastToggleMs = millis();

  comm.begin(115200);

  // register commands
  comm.addCommand("led",    &HandleLed);
  comm.addCommand("blink",  &HandleBlinkDefault);
  comm.addCommand("blinkp", &HandleBlinkPeriod);
  comm.addCommand("r",      &HandleReport);

  Serial.println(F("READY,ESP32-LED"));
}

void loop() {
  // 1. handle incoming serial commands
  comm.processSerial();

  // 2. handle blinking behavior
  if (ledMode == 2) { // blinking
    unsigned long now = millis();
    if (now - lastToggleMs >= blinkIntervalMs) {
      // time to toggle LED
      ledCurrentLevel = !ledCurrentLevel;
      digitalWrite(LED_PIN, ledCurrentLevel ? HIGH : LOW);
      lastToggleMs = now;
    }
  } else if (ledMode == 0) {
    // force off
    if (ledCurrentLevel != LOW) {
      ledCurrentLevel = LOW;
      digitalWrite(LED_PIN, LOW);
    }
  } else if (ledMode == 1) {
    // force on
    if (ledCurrentLevel != HIGH) {
      ledCurrentLevel = HIGH;
      digitalWrite(LED_PIN, HIGH);
    }
  }

  // no delay() needed; loop stays responsive
}
