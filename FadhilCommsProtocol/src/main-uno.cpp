#include <Arduino.h>
#include "Communication.h"


// === CONFIG ===
static const int LED_PIN = 2;  // onboard LED on ESP32-S

// === LED STATE ===
static int ledMode = 0; // 0=off, 1=on, 2=blinking, 3=fading
static bool ledDigitalLevel = LOW;

static unsigned long blinkIntervalMs = 500;
static unsigned long lastToggleMs = 0;

// Fade variables
static unsigned long fadePeriodMs = 2000; // default full up+down cycle 2s
static int fadeValue = 0;                 // current duty (0..PWM_MAX)
static int fadeDir = 1;                   // 1 = increasing, -1 = decreasing
static unsigned long lastFadeStepMs = 0;
static const int PWM_FREQ_HZ  = 1000;
static const int PWM_RES_BITS = 13;
static const int PWM_MAX_DUTY = (1 << PWM_RES_BITS) - 1;
static const int PWM_CHANNEL  = 0;  // channel 0 for LED_PIN

// === Communication ===
Communication comm;

// -------------------- HANDLERS --------------------

// <l,1> or <l,0>
void HandleLed(char** inputs) {
  if (inputs[0] == nullptr) { Serial.println(F("ERR,led,ARGS")); return; }
  int state = atoi(inputs[0]);
  ledMode = (state != 0) ? 1 : 0;
  ledDigitalLevel = (state != 0) ? HIGH : LOW;
  digitalWrite(LED_PIN, ledDigitalLevel);
  Serial.printf("OK,led,state=%d\n", ledDigitalLevel ? 1 : 0);
}

// <blink>
void HandleBlink(char** inputs) {
  ledMode = 2;
  blinkIntervalMs = 500;
  lastToggleMs = millis();
  Serial.printf("OK,blink,periodMs=%lu\n", blinkIntervalMs);
}

// <blinkp,seconds>
void HandleBlinkPeriod(char** inputs) {
  if (inputs[0] == nullptr) { Serial.println(F("ERR,blinkp,ARGS")); return; }
  float seconds = atof(inputs[0]);
  if (seconds < 0.01f) seconds = 0.01f;
  blinkIntervalMs = (unsigned long)(seconds * 1000.0f);
  ledMode = 2;
  lastToggleMs = millis();
  Serial.printf("OK,blinkp,periodMs=%lu\n", blinkIntervalMs);
}

// <fade>
void HandleFadeDefault(char** inputs) {
  ledMode = 3;
  fadePeriodMs = 2000; // 2s up-down cycle
  fadeDir = 1;
  fadeValue = 0;
  lastFadeStepMs = millis();
  Serial.printf("OK,fade,periodMs=%lu\n", fadePeriodMs);
}

// <fadep,seconds>
void HandleFadePeriod(char** inputs) {
  if (inputs[0] == nullptr) { Serial.println(F("ERR,fadep,ARGS")); return; }
  float seconds = atof(inputs[0]);
  if (seconds < 0.1f) seconds = 0.1f;
  fadePeriodMs = (unsigned long)(seconds * 1000.0f);
  ledMode = 3;
  fadeDir = 1;
  fadeValue = 0;
  lastFadeStepMs = millis();
  Serial.printf("OK,fadep,periodMs=%lu\n", fadePeriodMs);
}

// <r>
void HandleReport(char** inputs) {
  Serial.printf("OK,status,mode=%d,blink=%lu,fadePeriod=%lu,fadeVal=%d\n",
                ledMode, blinkIntervalMs, fadePeriodMs, fadeValue);
}

// -------------------- SETUP / LOOP --------------------
void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // PWM setup for fading
  ledcSetup(PWM_CHANNEL, PWM_FREQ_HZ, PWM_RES_BITS);
  ledcAttachPin(LED_PIN, PWM_CHANNEL);
  ledcWrite(PWM_CHANNEL, 0);

  comm.begin(115200);
  comm.addCommand("l",    &HandleLed);
  comm.addCommand("b",  &HandleBlink);
  comm.addCommand("bp", &HandleBlinkPeriod);
  comm.addCommand("f",   &HandleFadeDefault);
  comm.addCommand("fp",  &HandleFadePeriod);
  comm.addCommand("r",      &HandleReport);

  Serial.println(F("READY,ESP32S-LED"));
}

void loop() {
  comm.processSerial();

  unsigned long now = millis();

  // --- blinking ---
  if (ledMode == 2) {
    if (now - lastToggleMs >= blinkIntervalMs) {
      ledDigitalLevel = !ledDigitalLevel;
      digitalWrite(LED_PIN, ledDigitalLevel);
      lastToggleMs = now;
    }
  }
  // --- fading ---
  else if (ledMode == 3) {
    // how fast should we change brightness? about PWM_MAX steps per (fadePeriodMs/2)
    unsigned long stepDelay = (fadePeriodMs / 2) / PWM_MAX_DUTY;
    if (stepDelay < 1) stepDelay = 1;

    if (now - lastFadeStepMs >= stepDelay) {
      fadeValue += fadeDir;
      if (fadeValue >= PWM_MAX_DUTY) { fadeValue = PWM_MAX_DUTY; fadeDir = -1; }
      if (fadeValue <= 0)             { fadeValue = 0;            fadeDir =  1; }
      ledcWrite(PWM_CHANNEL, fadeValue);
      lastFadeStepMs = now;
    }
  }
  // --- steady OFF/ON ---
  else {
    digitalWrite(LED_PIN, (ledMode == 1) ? HIGH : LOW);
  }
}
