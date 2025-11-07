#include "Communication.h"

Communication::Communication(int numTEMS) : numTEMS(numTEMS), numData(0), newData(false)
{
}

void Communication::commandEnable()
{
    digitalWrite(enablePin, HIGH);
    isEnabled = true;
    Serial.println("System ENABLED");
}

void Communication::commandDisable()
{
    digitalWrite(enablePin, LOW);
    isEnabled = false;
    Serial.println("System DISABLED");
}

void Communication::commandSetpoint()
{
    if (numData == 2){
        if (inputData[1].charAt(0) == '+') {
            for (uint32_t i(0); i < numTEMS; ++i){
                setpoint[i] = therm->readCelsiusAvg(i) + inputData[1].substring(1).toDouble();
            }
        } else if (inputData[1].charAt(0) == '-') {
            for (uint32_t i(0); i < numTEMS; ++i){
                setpoint[i] = therm->readCelsiusAvg(i) - inputData[1].substring(1).toDouble();
            }
        } else {
            for (uint32_t i(0); i < numTEMS; ++i) {
                setpoint[i] = inputData[1].toDouble();
            }
        }
        Serial.print("All SETPOINTS changed to ");
        Serial.println(setpoint[0]);
    } else if (numData == 3) { 
        setpoint[inputData[1].toInt() - 1] = inputData[2].toDouble();
        Serial.print("SETPOINT of ");
        Serial.print(inputData[1]);
        Serial.println(" changed");
    } else if (numData == numTEMS + 1) {
        for (uint32_t i(0); i < numTEMS; ++i) {
            setpoint[i] = inputData[i+1].toDouble();
        }
        Serial.println("Each Individual SETPOINT changed");
    } else {
        Serial.println("ERROR: NO SETPOINTS changed");
    }
}

void Communication::commandReset()
{
    daq->setCLRValue(1.25);
    daq->performSoftCLR();
    Serial.println("DAQ RESET");
}

void Communication::recvWithStartEndMarkers()
{
    static boolean recvInProgress = false;
    static byte ndx = 0;
    char startMarker = '<';
    char endMarker = '>';
    char rc;
 
    while (Serial.available() > 0 && newData == false) {
        rc = Serial.read();

        if (recvInProgress == true) {
            if (rc != endMarker) {
                receivedChars[ndx] = rc;
                ndx++;
                if (ndx >= numChars) {
                    ndx = numChars - 1;
                }
            }
            else {
                receivedChars[ndx] = '\0'; // terminate the string
                recvInProgress = false;
                ndx = 0;
                newData = true;
            }
        }

        else if (rc == startMarker) {
            recvInProgress = true;
        }
    }
}

void Communication::parseData()
{
    numData = 0;
    String userInput = receivedChars;
    userInput.toLowerCase();
    int commaIndex = userInput.indexOf(',');
    int startIndex = 0;
    if (userInput != "")
    {
        while (commaIndex != -1)
        {
            inputData[numData] = userInput.substring(startIndex, commaIndex);
            inputData[numData].replace(" ", "");
            ++numData;
            startIndex = commaIndex + 1;
            commaIndex = userInput.indexOf(',', startIndex);
        }
        inputData[numData] = userInput.substring(startIndex);
        inputData[numData].replace(" ", "");
        ++numData;
    }
}

void Communication::executeCommand()
{
    switch (inputData[0].charAt(0))
    {
    case 'e':
        commandEnable();
        break;
    
    case 'd':
        commandDisable();
        break;

    case 's':
        commandSetpoint();
        break;

    case 'r':
        commandReset();
        break;
        
    default:
        Serial.println("Unknown command...Nothing Done");
        break;
    }
}

void Communication::initializeTherm(ThermistorArray* thermistors)
{
    therm = thermistors;
}

void Communication::initializeDaq(daqAD5391* parameterDaq)
{
    daq = parameterDaq;
}