/**
 * NodeMCUINA226.cpp - v1.01
 * Created by danel32, 2025
 * MIT License
 */

#include "NodeMCUINA226.h"
#include <math.h>

// Conversion wait times in microseconds
const uint16_t NodeMCUINA226::conversionWaitTimes[8] = {
    140,   // 140us
    204,   // 204us
    332,   // 332us
    588,   // 588us
    1100,  // 1.1ms
    2116,  // 2.1ms
    4156,  // 4.1ms
    8244   // 8.2ms
};

// Sample averages
const uint16_t NodeMCUINA226::sampleAverages[8] = {
    1,    // 1 sample
    4,    // 4 samples
    16,   // 16 samples
    64,   // 64 samples
    128,  // 128 samples
    256,  // 256 samples
    512,  // 512 samples
    1024  // 1024 samples
};

NodeMCUINA226::NodeMCUINA226() {
    inaAddress = 0;
    currentLSB = 0.0;
    powerLSB = 0.0;
    rShunt = 0.1;
    vShuntMax = 0.08192;
    vBusMax = 36.0;
    lastError = INA226_ERROR_NONE;
    alertType = INA226_ALERT_TYPE_NONE;
    alertLimit = 0.0;
}

bool NodeMCUINA226::begin(uint8_t address, int sda, int scl) {
    inaAddress = address;
    
    if (sda >= 0 && scl >= 0) {
        if (!Wire.begin(sda, scl)) {
            setError(INA226_ERROR_I2C_BEGIN);
            return false;
        }
    } else {
        Wire.begin();
    }
    
    Wire.setClock(400000); // Set I2C clock to 400kHz for faster readings
    
    Wire.beginTransmission(inaAddress);
    if (Wire.endTransmission() != 0) {
        setError(INA226_ERROR_I2C_BEGIN);
        return false;
    }
    
    if (!reset()) {
        return false;
    }
    
    delay(1); // Wait for reset to complete
    
    return configure();
}

bool NodeMCUINA226::isConnected() {
    Wire.beginTransmission(inaAddress);
    bool connected = (Wire.endTransmission() == 0);
    
    if (!connected) {
        setError(INA226_ERROR_I2C_BEGIN);
    }
    
    return connected;
}

bool NodeMCUINA226::configure(ina226_averages_t avg, ina226_busConvTime_t busConvTime,
                            ina226_shuntConvTime_t shuntConvTime, ina226_mode_t mode) {
    configAvg = avg;
    configBusConvTime = busConvTime;
    configShuntConvTime = shuntConvTime;
    configMode = mode;
    
    uint16_t config = (avg | busConvTime | shuntConvTime | mode);
    
    vBusMax = 36.0;
    vShuntMax = 0.08192;
    
    bool success = writeRegister16(INA226_REG_CONFIG, config);
    if (!success) {
        setError(INA226_ERROR_CONFIG);
    }
    
    return success;
}

bool NodeMCUINA226::calibrate(float rShuntValue, float iMaxExpected) {
    rShunt = rShuntValue;
    
    float iMaxPossible = vShuntMax / rShunt;
    float minimumLSB = iMaxExpected / 32767;
    
    currentLSB = ceil(minimumLSB * 100000000) / 100000000;
    powerLSB = currentLSB * 25;
    
    uint16_t calibrationValue = (uint16_t)((0.00512) / (currentLSB * rShunt));
    
    bool success = writeRegister16(INA226_REG_CALIBRATION, calibrationValue);
    if (!success) {
        setError(INA226_ERROR_I2C_WRITE);
    }
    
    return success;
}

bool NodeMCUINA226::calibrateForMilliAmps(uint16_t maxMilliAmps, float rShuntOhms) {
    return calibrate(rShuntOhms, maxMilliAmps / 1000.0);
}

bool NodeMCUINA226::powerUp() {
    bool result = configure(configAvg, configBusConvTime, configShuntConvTime, configMode);
    delayMicroseconds(40);
    return result;
}

bool NodeMCUINA226::powerDown() {
    return writeRegister16(INA226_REG_CONFIG, INA226_MODE_POWER_DOWN);
}

bool NodeMCUINA226::reset() {
    bool success = writeRegister16(INA226_REG_CONFIG, INA226_CONFIG_RESET);
    if (!success) {
        setError(INA226_ERROR_CONFIG);
    }
    return success;
}

bool NodeMCUINA226::triggerAndWait(ina226_mode_t mode) {
    configMode = mode;
    uint16_t config = configAvg | configBusConvTime | configShuntConvTime | mode;
    
    if (!writeRegister16(INA226_REG_CONFIG, config)) {
        setError(INA226_ERROR_CONFIG);
        return false;
    }
    
    delayMicroseconds(40);
    
    uint8_t avgIndex = getAveragesIndex(configAvg);
    uint8_t busIndex = getBusConvTimeIndex(configBusConvTime);
    uint8_t shuntIndex = getShuntConvTimeIndex(configShuntConvTime);
    
    uint32_t convTime = conversionWaitTimes[busIndex] + conversionWaitTimes[shuntIndex];
    convTime *= sampleAverages[avgIndex];
    convTime += 1000; // Safety margin
    
    delayMicroseconds(convTime);
    
    unsigned long timeout = millis() + 1000;
    while (millis() < timeout) {
        uint16_t maskEnable;
        if (readRegister16(INA226_REG_MASKENABLE, &maskEnable)) {
            if (maskEnable & INA226_ALERT_CVRF) {
                return true;
            }
        }
        delay(1);
    }
    
    setError(INA226_ERROR_TIMEOUT);
    return false;
}

float NodeMCUINA226::readShuntVoltage() {
    int16_t value;
    if (!readRegister16Signed(INA226_REG_SHUNTVOLTAGE, &value)) {
        return NAN;
    }
    return value * 0.0000025; // 2.5 µV per LSB
}

float NodeMCUINA226::readBusVoltage() {
    uint16_t value;
    if (!readRegister16(INA226_REG_BUSVOLTAGE, &value)) {
        return NAN;
    }
    return value * 0.00125; // 1.25 mV per LSB
}

float NodeMCUINA226::readShuntCurrent() {
    int16_t value;
    if (!readRegister16Signed(INA226_REG_CURRENT, &value)) {
        return NAN;
    }
    return value * currentLSB;
}

float NodeMCUINA226::readBusPower() {
    uint16_t value;
    if (!readRegister16(INA226_REG_POWER, &value)) {
        return NAN;
    }
    return value * powerLSB;
}

float NodeMCUINA226::readLoadVoltage() {
    float busVoltage = readBusVoltage();
    float shuntVoltage = readShuntVoltage();
    
    if (isnan(busVoltage) || isnan(shuntVoltage)) {
        return NAN;
    }
    
    return busVoltage + shuntVoltage;
}

INA226_Measurements NodeMCUINA226::readAll() {
    INA226_Measurements measurements;
    
    measurements.shuntVoltage = readShuntVoltage();
    measurements.busVoltage = readBusVoltage();
    measurements.current = readShuntCurrent();
    measurements.power = readBusPower();
    measurements.loadVoltage = measurements.busVoltage + measurements.shuntVoltage;
    
    return measurements;
}

float NodeMCUINA226::getMaxPossibleCurrent() {
    return vShuntMax / rShunt;
}

float NodeMCUINA226::getMaxCurrent() {
    float maxCurrent = currentLSB * 32767;
    float maxPossible = getMaxPossibleCurrent();
    return min(maxCurrent, maxPossible);
}

float NodeMCUINA226::getMaxShuntVoltage() {
    float maxVoltage = getMaxCurrent() * rShunt;
    return min(maxVoltage, vShuntMax);
}

float NodeMCUINA226::getMaxPower() {
    return getMaxCurrent() * vBusMax;
}

ina226_averages_t NodeMCUINA226::getAverages() {
    uint16_t value;
    if (!readRegister16(INA226_REG_CONFIG, &value)) {
        setError(INA226_ERROR_I2C_READ);
        return INA226_AVG_1;
    }
    return (ina226_averages_t)(value & 0x0E00);
}

ina226_busConvTime_t NodeMCUINA226::getBusConversionTime() {
    uint16_t value;
    if (!readRegister16(INA226_REG_CONFIG, &value)) {
        setError(INA226_ERROR_I2C_READ);
        return INA226_BUS_CONV_1100US;
    }
    return (ina226_busConvTime_t)(value & 0x01C0);
}

ina226_shuntConvTime_t NodeMCUINA226::getShuntConversionTime() {
    uint16_t value;
    if (!readRegister16(INA226_REG_CONFIG, &value)) {
        setError(INA226_ERROR_I2C_READ);
        return INA226_SHUNT_CONV_1100US;
    }
    return (ina226_shuntConvTime_t)(value & 0x0038);
}

ina226_mode_t NodeMCUINA226::getMode() {
    uint16_t value;
    if (!readRegister16(INA226_REG_CONFIG, &value)) {
        setError(INA226_ERROR_I2C_READ);
        return INA226_MODE_POWER_DOWN;
    }
    return (ina226_mode_t)(value & 0x0007);
}

bool NodeMCUINA226::setMaskEnable(uint16_t mask) {
    return writeRegister16(INA226_REG_MASKENABLE, mask);
}

uint16_t NodeMCUINA226::getMaskEnable() {
    uint16_t value;
    if (!readRegister16(INA226_REG_MASKENABLE, &value)) {
        setError(INA226_ERROR_I2C_READ);
        return 0xFFFF;
    }
    return value;
}

bool NodeMCUINA226::enableShuntOverLimitAlert() {
    uint16_t mask = getMaskEnable();
    if (mask == 0xFFFF) return false;
    
    mask |= INA226_ALERT_SOL;
    alertType = INA226_ALERT_TYPE_SHUNT_OVER;
    return setMaskEnable(mask);
}

bool NodeMCUINA226::enableShuntUnderLimitAlert() {
    uint16_t mask = getMaskEnable();
    if (mask == 0xFFFF) return false;
    
    mask |= INA226_ALERT_SUL;
    alertType = INA226_ALERT_TYPE_SHUNT_UNDER;
    return setMaskEnable(mask);
}

bool NodeMCUINA226::enableBusOverLimitAlert() {
    uint16_t mask = getMaskEnable();
    if (mask == 0xFFFF) return false;
    
    mask |= INA226_ALERT_BOL;
    alertType = INA226_ALERT_TYPE_BUS_OVER;
    return setMaskEnable(mask);
}

bool NodeMCUINA226::enableBusUnderLimitAlert() {
    uint16_t mask = getMaskEnable();
    if (mask == 0xFFFF) return false;
    
    mask |= INA226_ALERT_BUL;
    alertType = INA226_ALERT_TYPE_BUS_UNDER;
    return setMaskEnable(mask);
}

bool NodeMCUINA226::enableOverPowerLimitAlert() {
    uint16_t mask = getMaskEnable();
    if (mask == 0xFFFF) return false;
    
    mask |= INA226_ALERT_POL;
    alertType = INA226_ALERT_TYPE_POWER_OVER;
    return setMaskEnable(mask);
}

bool NodeMCUINA226::enableConversionReadyAlert() {
    uint16_t mask = getMaskEnable();
    if (mask == 0xFFFF) return false;
    
    mask |= INA226_ALERT_CNVR;
    alertType = INA226_ALERT_TYPE_CONVERSION_READY;
    return setMaskEnable(mask);
}

bool NodeMCUINA226::setBusVoltageLimit(float voltage) {
    uint16_t value = voltage / 0.00125;
    alertLimit = voltage;
    alertType = INA226_ALERT_TYPE_BUS_OVER;
    return writeRegister16(INA226_REG_ALERTLIMIT, value);
}

bool NodeMCUINA226::setShuntVoltageLimit(float voltage) {
    uint16_t value = voltage / 0.0000025;
    alertLimit = voltage;
    alertType = INA226_ALERT_TYPE_SHUNT_OVER;
    return writeRegister16(INA226_REG_ALERTLIMIT, value);
}

bool NodeMCUINA226::setPowerLimit(float watts) {
    uint16_t value = watts / powerLSB;
    alertLimit = watts;
    alertType = INA226_ALERT_TYPE_POWER_OVER;
    return writeRegister16(INA226_REG_ALERTLIMIT, value);
}

bool NodeMCUINA226::setAlertInvertedPolarity(bool inverted) {
    uint16_t mask = getMaskEnable();
    if (mask == 0xFFFF) return false;
    
    if (inverted) {
        mask |= INA226_ALERT_APOL;
    } else {
        mask &= ~INA226_ALERT_APOL;
    }
    
    return setMaskEnable(mask);
}

bool NodeMCUINA226::setAlertLatch(bool latch) {
    uint16_t mask = getMaskEnable();
    if (mask == 0xFFFF) return false;
    
    if (latch) {
        mask |= INA226_ALERT_LEN;
    } else {
        mask &= ~INA226_ALERT_LEN;
    }
    
    return setMaskEnable(mask);
}

bool NodeMCUINA226::isMathOverflow() {
    uint16_t mask = getMaskEnable();
    if (mask == 0xFFFF) return false;
    
    return (mask & INA226_ALERT_OVF) != 0;
}

bool NodeMCUINA226::isAlert() {
    uint16_t mask = getMaskEnable();
    if (mask == 0xFFFF) return false;
    
    return (mask & INA226_ALERT_AFF) != 0;
}

uint8_t NodeMCUINA226::getLastError() const {
    return lastError;
}

const char* NodeMCUINA226::getLastErrorMessage() const {
    return errorToString(lastError);
}

ina226_alert_type_t NodeMCUINA226::getAlertType() const {
    return alertType;
}

float NodeMCUINA226::getAlertLimit() const {
    return alertLimit;
}

void NodeMCUINA226::printDebug() {
    Serial.println(F("===== INA226 Debug Info ====="));
    Serial.print(F("I2C Address: 0x"));
    Serial.println(inaAddress, HEX);
    
    Serial.print(F("Last Error: "));
    Serial.print(lastError);
    Serial.print(F(" ("));
    Serial.print(getLastErrorMessage());
    Serial.println(F(")"));
    
    Serial.print(F("Configuration: "));
    Serial.println(describeConfig());
    
    INA226_Measurements measurements = readAll();
    
    Serial.print(F("Bus Voltage: "));
    Serial.print(measurements.busVoltage, 3);
    Serial.println(F(" V"));
    
    Serial.print(F("Shunt Voltage: "));
    Serial.print(measurements.shuntVoltage * 1000, 3);
    Serial.println(F(" mV"));
    
    Serial.print(F("Load Voltage: "));
    Serial.print(measurements.loadVoltage, 3);
    Serial.println(F(" V"));
    
    Serial.print(F("Current: "));
    Serial.print(measurements.current * 1000, 3);
    Serial.println(F(" mA"));
    
    Serial.print(F("Power: "));
    Serial.print(measurements.power * 1000, 3);
    Serial.println(F(" mW"));
    
    Serial.print(F("Alert Type: "));
    switch (alertType) {
        case INA226_ALERT_TYPE_NONE: Serial.println(F("None")); break;
        case INA226_ALERT_TYPE_SHUNT_OVER: Serial.println(F("Shunt Over-Voltage")); break;
        case INA226_ALERT_TYPE_SHUNT_UNDER: Serial.println(F("Shunt Under-Voltage")); break;
        case INA226_ALERT_TYPE_BUS_OVER: Serial.println(F("Bus Over-Voltage")); break;
        case INA226_ALERT_TYPE_BUS_UNDER: Serial.println(F("Bus Under-Voltage")); break;
        case INA226_ALERT_TYPE_POWER_OVER: Serial.println(F("Power Over-Limit")); break;
        case INA226_ALERT_TYPE_CONVERSION_READY: Serial.println(F("Conversion Ready")); break;
        default: Serial.println(F("Unknown")); break;
    }
    
    Serial.print(F("Alert Limit: "));
    Serial.println(alertLimit, 6);
    
    Serial.println(F("============================"));
}

String NodeMCUINA226::describeConfig() {
    String config = "";
    
    switch (configAvg) {
        case INA226_AVG_1: config += F("AVG: 1, "); break;
        case INA226_AVG_4: config += F("AVG: 4, "); break;
        
        case INA226_AVG_16: config += F("AVG: 16, "); break;
        case INA226_AVG_64: config += F("AVG: 64, "); break;
        case INA226_AVG_128: config += F("AVG: 128, "); break;
        case INA226_AVG_256: config += F("AVG: 256, "); break;
        case INA226_AVG_512: config += F("AVG: 512, "); break;
        case INA226_AVG_1024: config += F("AVG: 1024, "); break;
        default: config += F("AVG: Unknown, "); break;
    }
    
    switch (configBusConvTime) {
        case INA226_BUS_CONV_140US: config += F("Bus: 140us, "); break;
        case INA226_BUS_CONV_204US: config += F("Bus: 204us, "); break;
        case INA226_BUS_CONV_332US: config += F("Bus: 332us, "); break;
        case INA226_BUS_CONV_588US: config += F("Bus: 588us, "); break;
        case INA226_BUS_CONV_1100US: config += F("Bus: 1.1ms, "); break;
        case INA226_BUS_CONV_2116US: config += F("Bus: 2.1ms, "); break;
        case INA226_BUS_CONV_4156US: config += F("Bus: 4.1ms, "); break;
        case INA226_BUS_CONV_8244US: config += F("Bus: 8.2ms, "); break;
        default: config += F("Bus: Unknown, "); break;
    }
    
    switch (configShuntConvTime) {
        case INA226_SHUNT_CONV_140US: config += F("Shunt: 140us, "); break;
        case INA226_SHUNT_CONV_204US: config += F("Shunt: 204us, "); break;
        case INA226_SHUNT_CONV_332US: config += F("Shunt: 332us, "); break;
        case INA226_SHUNT_CONV_588US: config += F("Shunt: 588us, "); break;
        case INA226_SHUNT_CONV_1100US: config += F("Shunt: 1.1ms, "); break;
        case INA226_SHUNT_CONV_2116US: config += F("Shunt: 2.1ms, "); break;
        case INA226_SHUNT_CONV_4156US: config += F("Shunt: 4.1ms, "); break;
        case INA226_SHUNT_CONV_8244US: config += F("Shunt: 8.2ms, "); break;
        default: config += F("Shunt: Unknown, "); break;
    }
    
    switch (configMode) {
        case INA226_MODE_POWER_DOWN: config += F("Mode: Power Down"); break;
        case INA226_MODE_SHUNT_TRIG: config += F("Mode: Shunt Triggered"); break;
        case INA226_MODE_BUS_TRIG: config += F("Mode: Bus Triggered"); break;
        case INA226_MODE_SHUNT_BUS_TRIG: config += F("Mode: Shunt+Bus Triggered"); break;
        case INA226_MODE_ADC_OFF: config += F("Mode: ADC Off"); break;
        case INA226_MODE_SHUNT_CONT: config += F("Mode: Shunt Continuous"); break;
        case INA226_MODE_BUS_CONT: config += F("Mode: Bus Continuous"); break;
        case INA226_MODE_SHUNT_BUS_CONT: config += F("Mode: Shunt+Bus Continuous"); break;
        default: config += F("Mode: Unknown"); break;
    }
    
    return config;
}

uint8_t NodeMCUINA226::getAveragesIndex(ina226_averages_t avg) {
    switch (avg) {
        case INA226_AVG_1: return 0;
        case INA226_AVG_4: return 1;
        case INA226_AVG_16: return 2;
        case INA226_AVG_64: return 3;
        case INA226_AVG_128: return 4;
        case INA226_AVG_256: return 5;
        case INA226_AVG_512: return 6;
        case INA226_AVG_1024: return 7;
        default: return 0;
    }
}

uint8_t NodeMCUINA226::getBusConvTimeIndex(ina226_busConvTime_t busConvTime) {
    switch (busConvTime) {
        case INA226_BUS_CONV_140US: return 0;
        case INA226_BUS_CONV_204US: return 1;
        case INA226_BUS_CONV_332US: return 2;
        case INA226_BUS_CONV_588US: return 3;
        case INA226_BUS_CONV_1100US: return 4;
        case INA226_BUS_CONV_2116US: return 5;
        case INA226_BUS_CONV_4156US: return 6;
        case INA226_BUS_CONV_8244US: return 7;
        default: return 4;
    }
}

uint8_t NodeMCUINA226::getShuntConvTimeIndex(ina226_shuntConvTime_t shuntConvTime) {
    switch (shuntConvTime) {
        case INA226_SHUNT_CONV_140US: return 0;
        case INA226_SHUNT_CONV_204US: return 1;
        case INA226_SHUNT_CONV_332US: return 2;
        case INA226_SHUNT_CONV_588US: return 3;
        case INA226_SHUNT_CONV_1100US: return 4;
        case INA226_SHUNT_CONV_2116US: return 5;
        case INA226_SHUNT_CONV_4156US: return 6;
        case INA226_SHUNT_CONV_8244US: return 7;
        default: return 4;
    }
}

bool NodeMCUINA226::writeRegister16(uint8_t reg, uint16_t val) {
    Wire.beginTransmission(inaAddress);
    Wire.write(reg);
    Wire.write((val >> 8) & 0xFF);
    Wire.write(val & 0xFF);
    
    if (Wire.endTransmission() != 0) {
        setError(INA226_ERROR_I2C_WRITE);
        return false;
    }
    return true;
}

bool NodeMCUINA226::readRegister16(uint8_t reg, uint16_t* val) {
    if (val == nullptr) {
        setError(INA226_ERROR_PARAMETER);
        return false;
    }
    
    Wire.beginTransmission(inaAddress);
    Wire.write(reg);
    if (Wire.endTransmission() != 0) {
        setError(INA226_ERROR_I2C_WRITE);
        return false;
    }
    
    if (Wire.requestFrom(inaAddress, (uint8_t)2) != 2) {
        setError(INA226_ERROR_I2C_READ);
        return false;
    }
    
    uint8_t vha = Wire.read();
    uint8_t vla = Wire.read();
    *val = ((uint16_t)vha << 8) | vla;
    
    return true;
}

bool NodeMCUINA226::readRegister16Signed(uint8_t reg, int16_t* val) {
    uint16_t unsignedVal;
    bool success = readRegister16(reg, &unsignedVal);
    
    if (success) {
        *val = (int16_t)unsignedVal;
        return true;
    }
    
    return false;
}

void NodeMCUINA226::setError(uint8_t error) {
    lastError = error;
}

const char* NodeMCUINA226::errorToString(uint8_t error) {
    switch (error) {
        case INA226_ERROR_NONE: return "No error";
        case INA226_ERROR_I2C_BEGIN: return "I2C begin failed";
        case INA226_ERROR_I2C_WRITE: return "I2C write failed";
        case INA226_ERROR_I2C_READ: return "I2C read failed";
        case INA226_ERROR_CONFIG: return "Configuration failed";
        case INA226_ERROR_TIMEOUT: return "Operation timed out";
        case INA226_ERROR_PARAMETER: return "Invalid parameter";
        default: return "Unknown error";
    }
}