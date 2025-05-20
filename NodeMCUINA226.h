/**
 * NodeMCUINA226.h - v1.01
 * Created by danel32, 2025
 * MIT License
 */

#ifndef NODEMCU_INA226_H
#define NODEMCU_INA226_H

#include <Arduino.h>
#include <Wire.h>

// INA226 register addresses
#define INA226_REG_CONFIG           0x00
#define INA226_REG_SHUNTVOLTAGE     0x01
#define INA226_REG_BUSVOLTAGE       0x02
#define INA226_REG_POWER            0x03
#define INA226_REG_CURRENT          0x04
#define INA226_REG_CALIBRATION      0x05
#define INA226_REG_MASKENABLE       0x06
#define INA226_REG_ALERTLIMIT       0x07
#define INA226_REG_MANUFACTURER     0xFE
#define INA226_REG_DIE_ID           0xFF

// Configuration register bits
#define INA226_CONFIG_RESET         0x8000
#define INA226_CONFIG_AVG_MASK      0x0E00
#define INA226_CONFIG_VBUSCT_MASK   0x01C0
#define INA226_CONFIG_VSHCT_MASK    0x0038
#define INA226_CONFIG_MODE_MASK     0x0007

// Alert configuration bits
#define INA226_ALERT_SOL            0x8000
#define INA226_ALERT_SUL            0x4000
#define INA226_ALERT_BOL            0x2000
#define INA226_ALERT_BUL            0x1000
#define INA226_ALERT_POL            0x0800
#define INA226_ALERT_CNVR           0x0400
#define INA226_ALERT_AFF            0x0010
#define INA226_ALERT_CVRF           0x0008
#define INA226_ALERT_OVF            0x0004
#define INA226_ALERT_APOL           0x0002
#define INA226_ALERT_LEN            0x0001

// Error codes
#define INA226_ERROR_NONE           0
#define INA226_ERROR_I2C_BEGIN      1
#define INA226_ERROR_I2C_WRITE      2
#define INA226_ERROR_I2C_READ       3
#define INA226_ERROR_CONFIG         4
#define INA226_ERROR_TIMEOUT        5
#define INA226_ERROR_PARAMETER      6

// Averaging modes (samples)
typedef enum {
    INA226_AVG_1    = 0x0000,
    INA226_AVG_4    = 0x0200,
    INA226_AVG_16   = 0x0400,
    INA226_AVG_64   = 0x0600,
    INA226_AVG_128  = 0x0800,
    INA226_AVG_256  = 0x0A00,
    INA226_AVG_512  = 0x0C00,
    INA226_AVG_1024 = 0x0E00
} ina226_averages_t;

// Bus conversion times
typedef enum {
    INA226_BUS_CONV_140US   = 0x0000,
    INA226_BUS_CONV_204US   = 0x0040,
    INA226_BUS_CONV_332US   = 0x0080,
    INA226_BUS_CONV_588US   = 0x00C0,
    INA226_BUS_CONV_1100US  = 0x0100,
    INA226_BUS_CONV_2116US  = 0x0140,
    INA226_BUS_CONV_4156US  = 0x0180,
    INA226_BUS_CONV_8244US  = 0x01C0
} ina226_busConvTime_t;

// Shunt conversion times
typedef enum {
    INA226_SHUNT_CONV_140US   = 0x0000,
    INA226_SHUNT_CONV_204US   = 0x0008,
    INA226_SHUNT_CONV_332US   = 0x0010,
    INA226_SHUNT_CONV_588US   = 0x0018,
    INA226_SHUNT_CONV_1100US  = 0x0020,
    INA226_SHUNT_CONV_2116US  = 0x0028,
    INA226_SHUNT_CONV_4156US  = 0x0030,
    INA226_SHUNT_CONV_8244US  = 0x0038
} ina226_shuntConvTime_t;

// Operating modes
typedef enum {
    INA226_MODE_POWER_DOWN      = 0x0000,
    INA226_MODE_SHUNT_TRIG      = 0x0001,
    INA226_MODE_BUS_TRIG        = 0x0002,
    INA226_MODE_SHUNT_BUS_TRIG  = 0x0003,
    INA226_MODE_ADC_OFF         = 0x0004,
    INA226_MODE_SHUNT_CONT      = 0x0005,
    INA226_MODE_BUS_CONT        = 0x0006,
    INA226_MODE_SHUNT_BUS_CONT  = 0x0007
} ina226_mode_t;

// Alert types
typedef enum {
    INA226_ALERT_TYPE_NONE,
    INA226_ALERT_TYPE_SHUNT_OVER,
    INA226_ALERT_TYPE_SHUNT_UNDER,
    INA226_ALERT_TYPE_BUS_OVER,
    INA226_ALERT_TYPE_BUS_UNDER,
    INA226_ALERT_TYPE_POWER_OVER,
    INA226_ALERT_TYPE_CONVERSION_READY
} ina226_alert_type_t;

// Measurement data structure
struct INA226_Measurements {
    float shuntVoltage;  // Shunt voltage in volts
    float busVoltage;    // Bus voltage in volts
    float current;       // Current in amps
    float power;         // Power in watts
    float loadVoltage;   // Load voltage (bus + shunt) in volts
};

class NodeMCUINA226 {
public:
    NodeMCUINA226();
    
    // Initialization
    bool begin(uint8_t address = 0x40, int sda = -1, int scl = -1);
    bool isConnected();
    bool reset();
    
    // Configuration
    bool configure(ina226_averages_t avg = INA226_AVG_1,
                  ina226_busConvTime_t busConvTime = INA226_BUS_CONV_1100US,
                  ina226_shuntConvTime_t shuntConvTime = INA226_SHUNT_CONV_1100US,
                  ina226_mode_t mode = INA226_MODE_SHUNT_BUS_CONT);
    
    // Calibration
    bool calibrate(float rShuntValue = 0.1, float iMaxExpected = 2.0);
    bool calibrateForMilliAmps(uint16_t maxMilliAmps, float rShuntOhms);
    
    // Power management
    bool powerUp();
    bool powerDown();
    bool triggerAndWait(ina226_mode_t mode);
    
    // Single measurements
    float readShuntVoltage();
    float readBusVoltage();
    float readShuntCurrent();
    float readBusPower();
    float readLoadVoltage();
    
    // All measurements at once
    INA226_Measurements readAll();
    
    // Maximum values
    float getMaxPossibleCurrent();
    float getMaxCurrent();
    float getMaxShuntVoltage();
    float getMaxPower();
    
    // Configuration getters
    ina226_averages_t getAverages();
    ina226_busConvTime_t getBusConversionTime();
    ina226_shuntConvTime_t getShuntConversionTime();
    ina226_mode_t getMode();
    
    // Alert configuration
    bool setMaskEnable(uint16_t mask);
    uint16_t getMaskEnable();
    bool enableShuntOverLimitAlert();
    bool enableShuntUnderLimitAlert();
    bool enableBusOverLimitAlert();
    bool enableBusUnderLimitAlert();
    bool enableOverPowerLimitAlert();
    bool enableConversionReadyAlert();
    bool setBusVoltageLimit(float voltage);
    bool setShuntVoltageLimit(float voltage);
    bool setPowerLimit(float watts);
    bool setAlertInvertedPolarity(bool inverted);
    bool setAlertLatch(bool latch);
    bool isMathOverflow();
    bool isAlert();
    
    // Error handling
    uint8_t getLastError() const;
    const char* getLastErrorMessage() const;
    ina226_alert_type_t getAlertType() const;
    float getAlertLimit() const;
    
    // Debug
    void printDebug();
    String describeConfig();
    
private:
    uint8_t inaAddress;
    float currentLSB;
    float powerLSB;
    float rShunt;
    float vShuntMax;
    float vBusMax;
    uint8_t lastError;
    ina226_alert_type_t alertType;
    float alertLimit;
    
    // Configuration settings
    ina226_averages_t configAvg;
    ina226_busConvTime_t configBusConvTime;
    ina226_shuntConvTime_t configShuntConvTime;
    ina226_mode_t configMode;
    
    // Conversion time tables
    static const uint16_t conversionWaitTimes[8];
    static const uint16_t sampleAverages[8];
    
    // Helper functions
    uint8_t getAveragesIndex(ina226_averages_t avg);
    uint8_t getBusConvTimeIndex(ina226_busConvTime_t busConvTime);
    uint8_t getShuntConvTimeIndex(ina226_shuntConvTime_t shuntConvTime);
    
    // I2C communication
    bool writeRegister16(uint8_t reg, uint16_t val);
    bool readRegister16(uint8_t reg, uint16_t* val);
    bool readRegister16Signed(uint8_t reg, int16_t* val);
    
    // Error handling
    void setError(uint8_t error);
    const char* errorToString(uint8_t error);
};

#endif // NODEMCU_INA226_H