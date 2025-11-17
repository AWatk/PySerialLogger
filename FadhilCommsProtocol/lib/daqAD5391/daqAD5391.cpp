#include "daqAD5391.h"
#include <Arduino.h>



daqAD5391::daqAD5391() : i2c_dev(AD5391_I2CADDR_DEFAULT) {
}

void daqAD5391::setCLRValue(double voltage) {
    uint16_t v = voltageToUint16t(voltage);
    
    uint8_t msb = v / 64;
    uint8_t lsb = (v % 64) << 2;

    msb &= AD5391_SETSFR;

    uint8_t packet[3];
    packet[0] = SFR_WRITE_CLR;
    packet[1] = msb;
    packet[2] = lsb;
    i2c_dev.write(packet,3);
}

void daqAD5391::performSoftCLR(){
    uint8_t packet[3];
    packet[0] = SFR_SOFT_CLR;
    packet[1] &= AD5391_SETSFR;
    //packet[2] don't care

    i2c_dev.write(packet,3);
}

void daqAD5391::performSoftReset(){
    uint8_t packet[3];
    packet[0] = SFR_SOFT_RESET;
    packet[1] &= AD5391_SETSFR;
    //packet[2] don't care

    i2c_dev.write(packet,3);
}

void daqAD5391::enableMonitor(){
    ctrl_val |= CTRL_MON_ENABLE;
    writeControlRegister();
}

void daqAD5391::writeControlRegister(){
    uint8_t msb = ctrl_val / 64;
    uint8_t lsb = (ctrl_val % 64) << 2;

    msb &= AD5391_SETSFR;

    uint8_t packet[3];
    packet[0] = SFR_CTRL_WRITE;
    packet[1] = msb;
    packet[2] = lsb;

    i2c_dev.write(packet,3);
}

void daqAD5391::setMonitorOut(uint8_t channel){
    channel = _max(_min(channel,15),0);
    uint8_t packet[3];
    packet[0] = SFR_MONITOR_CHANNEL;
    packet[1] = channel;
    //packet[2] don't care

    i2c_dev.write(packet,3);
}

void daqAD5391::setVoltage(uint8_t channel, double voltage){
    channel = _max(_min(channel, 15),0);
    uint16_t v = voltageToUint16t(voltage);

    uint8_t msdb = v / 64;
    uint8_t lsdb = (v % 64) << 2;

    msdb |= AD5391_SETVOLTAGE;

    uint8_t packet[3];
    packet[0] = channel;
    packet[1] = msdb;
    packet[2] = lsdb;
    i2c_dev.write(packet,3);
}

uint16_t daqAD5391::voltageToUint16t(double voltage){
    uint16_t v = round(voltage / 5.0 * 4096);
    return _max(_min(v,4095),0);
}

bool daqAD5391::begin(){
    return i2c_dev.begin();
}