#include <Arduino.h>
#include "Communication.h"
#include "Thermistor.h"
#include "daqAD5391.h"

// ----------------- Hardware config  -----------------
static const uint8_t NUM_TEMS = 4;   // how many thermistors you actually use (<=16)

// MUX + sense wiring 
static const uint8_t SIG_THC = A0;
static const uint8_t MUX_EN  = 8; //dont actually use this pin
static const uint8_t MUX_S0  = 5;
static const uint8_t MUX_S1  = 4;
static const uint8_t MUX_S2  = 3;
static const uint8_t MUX_S3  = 2;

// Optional system enable line 
static const uint8_t SYSTEM_ENABLE_PIN = 6;

// ----------------- Globals -----------------
Communication   comm;  // new protocol: addCommand(...), handlers get char** inputs
ThermistorArray therms(NUM_TEMS, SIG_THC, MUX_EN, MUX_S0, MUX_S1, MUX_S2, MUX_S3); // your real ctor
daqAD5391       daq;

bool   enabled = false;
double setpoints[16]; // room for 16 TEMs

// Map TEM index (0..N-1) to DAC channel (0..15).
static const int TEM_TO_DAC[] = { 0, 1, 2, 3 };  // for NUM_TEMS=4
static const int TEM_TO_DAC_COUNT = sizeof(TEM_TO_DAC) / sizeof(TEM_TO_DAC[0]);

// ----------------- Helpers -----------------
static inline double readTempC(uint8_t i) {
  if (i >= NUM_TEMS) i = NUM_TEMS - 1;
  return therms.readCelsiusAvg(i);  // steady reading from your array
}

static void setEnabled(bool on) {
  enabled = on;
  pinMode(SYSTEM_ENABLE_PIN, OUTPUT);
  digitalWrite(SYSTEM_ENABLE_PIN, on ? HIGH : LOW);
  Serial.println(on ? F("OK,e,ENABLED") : F("OK,d,DISABLED"));
}

// --- DAC helpers using driver ---
static const double DAC_VSAFE = 0.0; // safe level at startup
static const double DAC_VMIN  = 0.0;
static const double DAC_VMAX  = 5.0;

static inline double clampV(double v) {
  if (v < DAC_VMIN) return DAC_VMIN;
  if (v > DAC_VMAX) return DAC_VMAX;
  return v;
}

bool dacInit() {
  if (!daq.begin()) {                 // real begin() from your driver
    Serial.println(F("ERR,DAC_INIT_FAIL"));
    return false;
  }
  daq.setCLRValue(DAC_VSAFE);         // define clear value
  daq.performSoftCLR();               // clear to safe at boot
  return true;
}

void dacWrite(int ch, double volts) {
  if (ch < 0 || ch > 15) return;
  daq.setVoltage((uint8_t)ch, clampV(volts));   // real setVoltage()
}

void dacWriteAll(double volts) {
  volts = clampV(volts);
  for (int ch = 0; ch < 16; ++ch) daq.setVoltage((uint8_t)ch, volts);
}

// ----------------- Command Handlers (new protocol: args start at inputs[0]) -----------------

// <e>
void HandleEnable(char** inputs) { (void)inputs; setEnabled(true); }
// <d>
void HandleDisable(char** inputs) { (void)inputs; setEnabled(false); }

// <r>  (match your old "DAQ RESET")
void HandleReset(char** inputs) {
  (void)inputs;
  daq.setCLRValue(1.25);   // same as your old code path
  daq.performSoftCLR();
  Serial.println(F("OK,r,DAQ_RESET"));
}

// <s,...>  
void HandleSetpoint(char** inputs) {
  // count args
  int argc = 0; while (inputs[argc]) ++argc;
  if (argc == 0) { Serial.println(F("ERR,s,NOARGS")); return; }

  // 1) single arg: <s,25> or <s,+5> or <s,-3>
  if (argc == 1) {
    const char* a0 = inputs[0];

    if (a0[0] == '+' || a0[0] == '-') {
      // relative to current temps
      double delta = atof(a0 + 1);
      bool add = (a0[0] == '+');
      for (int i = 0; i < NUM_TEMS; ++i) {
        double base = readTempC(i);
        setpoints[i] = add ? (base + delta) : (base - delta);
      }
      Serial.print(F("OK,s,REL,delta=")); Serial.println(delta, 3);
      return;
    }

    // absolute set for all
    double val = atof(a0);
    for (int i = 0; i < NUM_TEMS; ++i) setpoints[i] = val;
    Serial.print(F("OK,s,ALL=")); Serial.println(val, 3);
    return;
  }

  // 2) two args: <s,idx,val>  -- specific control (0based indexing)
  if (argc == 2) {
    int    which = atoi(inputs[0]);  
    double val   = atof(inputs[1]);
    if (which < 0 || which > (NUM_TEMS-1)) { Serial.println(F("ERR,s,BADINDEX")); return; }
    setpoints[which] = val;
    Serial.print(F("OK,s,IDX=")); Serial.print(which);
    Serial.print(F(",VAL=")); Serial.println(val, 3);
    return;
  }

  // 3) N args == NUM_TEMS: <s,v1,v2,...,vN> (control all using speciific sets)
  if (argc == NUM_TEMS) {
    for (int i = 0; i < NUM_TEMS; ++i) setpoints[i] = atof(inputs[i]);
    Serial.println(F("OK,s,EACH"));
    return;
  }

  Serial.println(F("ERR,s,ARGS"));
}

// <t>  dump temps + setpoints (logging)
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

// ----- DAC commands -----
// <v,idx,volts>  (idx is 0-based; we map directly to DAC channel via TEM_TO_DAC[])
void HandleDacOne(char** inputs) {
  if (!inputs[0] || !inputs[1]) {
    Serial.println(F("ERR,v,ARGS"));
    return;
  }

  int idx = atoi(inputs[0]);       // 0-based TEM index
  double volts = atof(inputs[1]);

  if (idx < 0 || idx >= TEM_TO_DAC_COUNT) {
    Serial.println(F("ERR,v,BADINDEX"));
    return;
  }

  int ch = TEM_TO_DAC[idx];        // map thermistor index to DAC channel
  dacWrite(ch, volts);

  Serial.print(F("OK,v,idx=")); Serial.print(idx);
  Serial.print(F(",ch="));  Serial.print(ch);
  Serial.print(F(",V="));   Serial.println(clampV(volts), 3);
}


// <va,v1,v2,...>  write first N mapped channels
void HandleDacArray(char** inputs) {
  int n = 0; while (inputs[n]) ++n;
  if (n == 0) { Serial.println(F("ERR,va,ARGS")); return; }
  int limit = min(n, TEM_TO_DAC_COUNT);
  for (int i = 0; i < limit; ++i) dacWrite(TEM_TO_DAC[i], atof(inputs[i]));
  Serial.print(F("OK,va,N=")); Serial.println(limit);
}

// <vr,volts>  set all mapped channels to same volts
void HandleDacAllSame(char** inputs) {
  if (!inputs[0]) { Serial.println(F("ERR,vr,ARGS")); return; }
  double v = atof(inputs[0]);
  for (int i = 0; i < TEM_TO_DAC_COUNT; ++i) dacWrite(TEM_TO_DAC[i], v);
  Serial.print(F("OK,vr,N=")); Serial.print(TEM_TO_DAC_COUNT);
  Serial.print(F(",V=")); Serial.println(clampV(v), 3);
}

// ----------------- Arduino lifecycle -----------------
void setup() {
  Serial.begin(115200);

  pinMode(SYSTEM_ENABLE_PIN, OUTPUT);
  digitalWrite(SYSTEM_ENABLE_PIN, LOW);
  enabled = false;

  // ThermistorArray is fully initialized by its constructor (it calls updateArraySettings + initThermistors)
  // (No therms.begin() in your API.)

  // DAC init
  dacInit();

  // default setpoints
  for (int i = 0; i < NUM_TEMS; ++i) setpoints[i] = 25.0;

  // New protocol: register commands (tokens → handlers with args starting at inputs[0])
  comm.begin(115200);
  comm.addCommand("e",  &HandleEnable);
  comm.addCommand("d",  &HandleDisable);
  comm.addCommand("s",  &HandleSetpoint);
  comm.addCommand("r",  &HandleReset);
  comm.addCommand("t",  &HandleTemps);

  // DAC helpers
  comm.addCommand("v",  &HandleDacOne);
  comm.addCommand("va", &HandleDacArray);
  comm.addCommand("vr", &HandleDacAllSame);

  Serial.println(F("READY,THERMAL"));
}

void loop() {
  comm.processSerial();
  // If/when you add control, do it here; e.g. read temps, compare to setpoints, write DAC.
}
