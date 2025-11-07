#ifndef DAQ_AD5391_H
#define DAQ_AD5391_H

#define AD5391_I2CADDR_DEFAULT (0x54)

#define AD5391_SETVOLTAGE   0b11000000
#define AD5391_SETOFFSET    0b10000000
#define AD5391_SETGAIN      0b01000000
#define AD5391_SETSFR       0b00111111

#define SFR_NOP             0b00000000
#define SFR_WRITE_CLR       0b00000001
#define SFR_SOFT_CLR        0b00000010
#define SFR_SOFT_PDOWN      0b00001000
#define SFR_SOFT_PUP        0b00001001
#define SFR_CTRL_WRITE      0b00001100
#define SFR_CTRL_READ       0b00001100
#define SFR_MONITOR_CHANNEL 0b00001010
#define SFR_SOFT_RESET      0b00001111

#define CTRL_PDOWN_IMP      0b0000100000000000
#define CTRL_PDOWN_RES      0b1111011111111111
#define CTRL_REF_25         0b0000010000000000
#define CTRL_REF_125        0b1111101111111111
#define CTRL_I_BOOST_ON     0b0000001000000000
#define CTRL_I_BOOST_OFF    0b1111110111111111
#define CTRL_REF_INT        0b0000000100000000
#define CTRL_REF_EXT        0b1111111011111111
#define CTRL_MON_ENABLE     0b0000000010000000
#define CTRL_MON_DISABLE    0b1111111101111111
#define CTRL_THERM_ON       0b0000000001000000
#define CTRL_THERM_OFF      0b1111111110111111
#define CTRL_TOG_1_ON       0b0000000000000010
#define CTRL_TOG_1_OFF      0b1111111111111101
#define CTRL_TOG_2_ON       0b0000000000000001
#define CTRL_TOG_2_OFF      0b1111111111111110

#include <Adafruit_I2CDevice.h>

class daqAD5391 {
public:
    daqAD5391();
    bool begin();
    void setCLRValue(double voltage);
    void performSoftCLR();
    void performSoftReset();
    void enableMonitor();
    void writeControlRegister();
    void setMonitorOut(uint8_t channel);
    void setVoltage(uint8_t channel, double voltage);

private:
    Adafruit_I2CDevice i2c_dev;
    uint16_t ctrl_val = CTRL_PDOWN_IMP | CTRL_REF_25 & CTRL_I_BOOST_OFF & CTRL_REF_EXT 
                    & CTRL_MON_DISABLE & CTRL_THERM_OFF & CTRL_TOG_1_OFF & CTRL_TOG_2_OFF;
    int count = 0;
    uint16_t voltageToUint16t(double voltage);
};

#endif // DAQ_AD5391_H