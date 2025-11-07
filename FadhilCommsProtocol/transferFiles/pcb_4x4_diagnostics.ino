//#include "Thermistor.h"
#include "PID_v1.h"
#include "Communication.h"
#include "daqAD5391.h"

#define FORWARD 0
#define REVERSE 1

const int numTEMS = 16;

Communication comm(numTEMS);

// pcb setup pins
int enablePin = 7;
int itecPin = A1;
int vtecPin = A2;
int ldacPin = 6;
double vref = 2.5;
int monitorPin = A3;

// thermistor setup pins
int sigTHC = A0;
int en = 8; //dont actually use this pin
int s0 = 5;
int s1 = 4;
int s2 = 3;
int s3 = 2;

double v_itec = 0;
double v_vtec = 0;
double tecCurrVal = 0;
double tecVoltVal = 0;

ThermistorArray therms = ThermistorArray(numTEMS,sigTHC,en,s0,s1,s2,s3);

daqAD5391 daq;

// defines voltage values for the TEM
double inputVoltage = 5;
double maxOutputVolt = 2.03;
double minOutputVolt = 0.47;

// FIX ME - Make pid controller for each TEM
// PID setup
double setpoint[numTEMS];
double input[numTEMS];
double output[numTEMS];
double Kp=0.5, Ki=0, Kd=0;
PID tPid[numTEMS];

void setup() {
  for (uint32_t i(0); i < numTEMS; ++i) {
    
    tPid[i] = PID(&input[i], &output[i], &setpoint[i], Kp, Ki, Kd, REVERSE);
    
  }
  Serial.begin(115200);
  analogReference(EXTERNAL);
  pinMode(enablePin, OUTPUT);
  digitalWrite(enablePin, LOW);
  pinMode(ldacPin, OUTPUT);
  digitalWrite(ldacPin, LOW);

  // Setup daq
  if(!daq.begin())
  {
    Serial.println("Didn't find I2C device!");
  } else {
    daq.performSoftReset(); // Tell daq to power cycle
    delay(1);
    daq.enableMonitor(); // allows us to monitor each output
    daq.setMonitorOut(6);
    daq.setCLRValue(1.25);
    daq.performSoftCLR();
  }

  for (uint32_t i(0); i < numTEMS; ++i) {
    // setup PID and commucation setpoint
    input[i] = therms.readCelsiusAvg(i);
    setpoint[i] = input[i];
    comm.setpoint[i] = input[i];
    tPid[i].SetOutputLimits(-1,1);
    tPid[i].SetMode(AUTOMATIC);
    tPid[i].SetSampleTime(5);
  }
  comm.initializeTherm(&therms);
  comm.initializeDaq(&daq);
}

void loop() {
  String data = "";
  // Serial.println(analogRead(monitorPin) / 1024.0 * 2.511);
  // read user input
  comm.recvWithStartEndMarkers();
  if (comm.newData == true) {
    comm.parseData();
    comm.executeCommand();
    for (uint32_t i(0); i < numTEMS; ++i) {
      if (setpoint[i] != comm.setpoint[i]) {
        setpoint[i] = comm.setpoint[i];
      }
    }
  }
  comm.newData = false;

  for (uint32_t i(0); i < numTEMS; ++i) {
    // compute PID control signal value and write the control value to the DAC board
    input[i] = therms.readCelsius(i);
    if (comm.isEnabled) {
      tPid[i].Compute();
      double controlVoltage = interpolate(-1, 1, maxOutputVolt, minOutputVolt, output[i]);
      daq.setVoltage(i, controlVoltage);
    }
    
    //if (i == 6) {
    double val = therms.readCelsius(i);
    if(i == 5)
    {
      data.concat(i + 1);
      data.concat(". ");
      data.concat(val);
      data.concat(", ");
      data.concat(output[i]);
      data.concat(", ");
      double vtec = (analogRead(vtecPin) * 2.5 / 1024.0 - 1.25) * 4; 
      data.concat(vtec);
      data.concat(", ");
      double itec = (analogRead(itecPin) * 2.5 / 1024.0 - 1.25) / 0.525;
      data.concat(itec);
      data.concat("\n");
    }
    
    // Serial.print(i + 1);
    // Serial.print(". ");
    // Serial.print(therms.readCelsiusAvg(i));
    // Serial.print(",");
    // Serial.println(output[i]);
    //}
    if (i == numTEMS - 1) {
      Serial.println(data);
    }
  }
}

// perform computations for linear interpolation
double interpolate(double x1, double x2, double y1, double y2, double normValue) {
  return y1 + (normValue - x1) * ((y2 - y1) / (x2 - x1));
}
