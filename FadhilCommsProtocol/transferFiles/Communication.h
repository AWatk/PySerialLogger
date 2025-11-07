#ifndef COMMUNICATION_H
#define COMMUNICATION_H

#include "Arduino.h"
#include "Thermistor.h"
#include "daqAD5391.h"

const byte numChars = 32;

class Communication {
public:
Communication(int numTEMS);
void commandEnable();
void commandDisable();
void commandSetpoint();
void commandReset(); // Add functionality
void recvWithStartEndMarkers();
void parseData();
void executeCommand();
bool newData;
double setpoint[16];
void initializeTherm(ThermistorArray* thermistors);
void initializeDaq(daqAD5391* parameterDaq);
bool isEnabled;

private:
int numTEMS;
char receivedChars[numChars];
String inputData[17];
int numData;
ThermistorArray* therm;
daqAD5391* daq;

int enablePin = 7;

};

#endif // COMMUNICATION_H