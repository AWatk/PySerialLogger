#include <Arduino.h>
#include <Wire.h>
#include "Communication.h"
#include "Thermistor.h"
#include "daqAD5391.h"
#include "CommandDetails.h"

// ----------------- Hardware config -----------------
static const uint8_t NUM_TEMS = 4;

// Thermistor signals (safe pins)
static const uint8_t SIG_THC = A0;
static const uint8_t MUX_EN  = 16;
static const uint8_t MUX_S0  = 17;
static const uint8_t MUX_S1  = 18;
static const uint8_t MUX_S2  = 19;
static const uint8_t MUX_S3  = 23;

// System enable pin (safe)
static const uint8_t SYSTEM_ENABLE_PIN = 14;

// ----------------- Global objects -----------------
Communication   comm;  
ThermistorArray therms(NUM_TEMS, SIG_THC, MUX_EN, MUX_S0, MUX_S1, MUX_S2, MUX_S3);
daqAD5391       daq;

bool   enabled = false;
double setpoints[16];

// Thermistor index → DAC channel
static const int TEM_TO_DAC[] = { 0, 1, 2, 3 };
static const int TEM_TO_DAC_COUNT = sizeof(TEM_TO_DAC) / sizeof(TEM_TO_DAC[0]);

// ----------------- Helpers -----------------
static inline double readTempC(uint8_t i) {
  if (i >= NUM_TEMS) i = NUM_TEMS - 1;
  return therms.readCelsiusAvg(i);
}

static void setEnabled(bool on) {
  enabled = on;
  digitalWrite(SYSTEM_ENABLE_PIN, on ? HIGH : LOW);
  Serial.println(on ? F("OK,e,ENABLED") : F("OK,d,DISABLED"));
}

// ----------------- DAC helpers -----------------
static const double DAC_VSAFE = 0.0;
static const double DAC_VMIN  = 0.0;
static const double DAC_VMAX  = 5.0;

static inline double clampV(double v) {
  if (v < DAC_VMIN) return DAC_VMIN;
  if (v > DAC_VMAX) return DAC_VMAX;
  return v;
}

bool dacInit() {
  if (!daq.begin()) {
    Serial.println(F("ERR,DAC_INIT_FAIL"));
    return false;
  }
  daq.setCLRValue(DAC_VSAFE);
  daq.performSoftCLR();
  return true;
}

void dacWrite(int ch, double volts) {
  if (ch < 0 || ch > 15) return;
  daq.setVoltage((uint8_t)ch, clampV(volts));
}

void dacWriteAll(double volts) {
  volts = clampV(volts);
  for (int ch = 0; ch < 16; ++ch)
    daq.setVoltage((uint8_t)ch, volts);
}

// ----------------- Command Handlers -----------------

void HandleEnable(char** inputs)  { (void)inputs; setEnabled(true); }
void HandleDisable(char** inputs) { (void)inputs; setEnabled(false); }

void HandleReset(char** inputs) {
  (void)inputs;
  daq.setCLRValue(1.25);
  daq.performSoftCLR();
  Serial.println(F("OK,r,DAQ_RESET"));
}

void HandleSetpoint(char** inputs) {
  int argc = 0; while (inputs[argc]) ++argc;
  if (argc == 0) { Serial.println(F("ERR,s,NOARGS")); return; }

  // <s,25>
  if (argc == 1) {
    const char* a0 = inputs[0];

    if (a0[0] == '+' || a0[0] == '-') {
      double delta = atof(a0 + 1);
      bool add = (a0[0] == '+');
      for (int i = 0; i < NUM_TEMS; ++i) {
        double base = readTempC(i);
        setpoints[i] = add ? (base + delta) : (base - delta);
      }
      Serial.print(F("OK,s,REL,delta=")); Serial.println(delta, 3);
      return;
    }

    double val = atof(a0);
    for (int i = 0; i < NUM_TEMS; ++i)
      setpoints[i] = val;

    Serial.print(F("OK,s,ALL=")); Serial.println(val, 3);
    return;
  }

  // <s,idx,val>
  if (argc == 2) {
    int    which = atoi(inputs[0]);
    double val   = atof(inputs[1]);
    if (which < 0 || which >= NUM_TEMS) {
      Serial.println(F("ERR,s,BADINDEX"));
      return;
    }
    setpoints[which] = val;

    Serial.print(F("OK,s,IDX=")); Serial.print(which);
    Serial.print(F(",VAL=")); Serial.println(val, 3);
    return;
  }

  // <s,v1,v2,v3,v4>
  if (argc == NUM_TEMS) {
    for (int i = 0; i < NUM_TEMS; ++i)
      setpoints[i] = atof(inputs[i]);

    Serial.println(F("OK,s,EACH"));
    return;
  }

  Serial.println(F("ERR,s,ARGS"));
}

void HandleTemps(char** inputs) {
  (void)inputs;

  Serial.print(F("OK,t,EN=")); Serial.print(enabled ? 1 : 0);

  Serial.print(F(",T="));
  for (int i = 0; i < NUM_TEMS; ++i) {
    Serial.print(readTempC(i), 2);
    if (i < NUM_TEMS - 1) Serial.print(',');
  }

  Serial.print(F(",SP="));
  for (int i = 0; i < NUM_TEMS; ++i) {
    Serial.print(setpoints[i], 2);
    if (i < NUM_TEMS - 1) Serial.print(',');
  }

  Serial.println();
}

void HandleDacOne(char** inputs) {
  if (!inputs[0] || !inputs[1]) {
    Serial.println(F("ERR,v,ARGS"));
    return;
  }

  int idx = atoi(inputs[0]);
  double volts = atof(inputs[1]);

  if (idx < 0 || idx >= TEM_TO_DAC_COUNT) {
    Serial.println(F("ERR,v,BADINDEX"));
    return;
  }

  int ch = TEM_TO_DAC[idx];
  dacWrite(ch, volts);

  Serial.print(F("OK,v,idx=")); Serial.print(idx);
  Serial.print(F(",ch="));  Serial.print(ch);
  Serial.print(F(",V="));   Serial.println(clampV(volts), 3);
}

void HandleDacArray(char** inputs) {
  int n = 0; while (inputs[n]) ++n;
  if (n == 0) { Serial.println(F("ERR,va,ARGS")); return; }

  int limit = min(n, TEM_TO_DAC_COUNT);
  for (int i = 0; i < limit; ++i)
    dacWrite(TEM_TO_DAC[i], atof(inputs[i]));

  Serial.print(F("OK,va,N=")); Serial.println(limit);
}

void HandleDacAllSame(char** inputs) {
  if (!inputs[0]) { Serial.println(F("ERR,vr,ARGS")); return; }

  double v = atof(inputs[0]);
  for (int i = 0; i < TEM_TO_DAC_COUNT; ++i)
    dacWrite(TEM_TO_DAC[i], v);

  Serial.print(F("OK,vr,N=")); Serial.print(TEM_TO_DAC_COUNT);
  Serial.print(F(",V=")); Serial.println(clampV(v), 3);
}

// ----------------- Arduino lifecycle -----------------

void setup() {
  Serial.begin(115200);
  delay(300);

  // Safe I2C pins on ESP32-WROOM
  Wire.begin(21, 22);

  // Initialize thermistor pins & mux
  therms.begin();

  // System enable pin
  pinMode(SYSTEM_ENABLE_PIN, OUTPUT);
  digitalWrite(SYSTEM_ENABLE_PIN, LOW);

  // Initialize DAC
  dacInit();

  // Default setpoints
  for (int i = 0; i < NUM_TEMS; ++i)
    setpoints[i] = 25.0;

  // Register commands
  comm.begin(115200);
  comm.addCommand("e",  &HandleEnable,   DETAILS_ENABLE);
  comm.addCommand("d",  &HandleDisable,  DETAILS_DISABLE);
  comm.addCommand("s",  &HandleSetpoint, DETAILS_SETPOINT);
  comm.addCommand("r",  &HandleReset,    DETAILS_RESET);
  comm.addCommand("t",  &HandleTemps,    DETAILS_TEMPS);
  comm.addCommand("v",  &HandleDacOne,   DETAILS_DAC_ONE);
  comm.addCommand("va", &HandleDacArray, DETAILS_DAC_ARRAY);
  comm.addCommand("vr", &HandleDacAllSame, DETAILS_DAC_ALL);

  Serial.println(F("READY,THERMAL"));
}

void loop() {    
  comm.processSerial();
}
