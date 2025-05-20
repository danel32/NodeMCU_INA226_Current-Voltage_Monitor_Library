# INA226 Node.js Library

A complete Node.js library for interfacing with the INA226 current/voltage monitoring sensor. This library provides a full-featured API for configuring the INA226 and reading voltage, current, and power measurements.

## Features

- Complete implementation of INA226 functionality in Node.js
- Promises-based API for async operations
- TypeScript definitions included
- Comprehensive error handling
- Support for all INA226 operating modes and configuration options
- Alert functionality with various threshold types
- Detailed documentation and examples

## Installation

```bash
npm install ina226-node
```

## Basic Usage

```javascript
const { INA226, OperatingMode } = require('ina226-node');

// Create a new INA226 instance
const ina226 = new INA226({
  busNumber: 1,       // I2C bus number (usually 1 on Raspberry Pi)
  address: 0x40,      // I2C address of INA226 (default 0x40)
  rShunt: 0.1,        // Shunt resistor value in ohms
  maxCurrent: 2.0     // Maximum expected current in amps
});

async function main() {
  try {
    // Initialize the sensor
    await ina226.begin();
    
    // Configure the device for continuous measurements
    await ina226.configure({
      mode: OperatingMode.SHUNT_BUS_CONT
    });
    
    // Calibrate for accurate current and power readings
    await ina226.calibrate(0.1, 2.0); // 0.1 ohm shunt, 2A max
    
    // Read measurements
    const busVoltage = await ina226.readBusVoltage();
    const shuntVoltage = await ina226.readShuntVoltage();
    const current = await ina226.readShuntCurrent();
    const power = await ina226.readBusPower();
    
    console.log(`Bus Voltage: ${busVoltage.toFixed(3)} V`);
    console.log(`Shunt Voltage: ${(shuntVoltage * 1000).toFixed(3)} mV`);
    console.log(`Current: ${(current * 1000).toFixed(3)} mA`);
    console.log(`Power: ${(power * 1000).toFixed(3)} mW`);
    
    // Close the I2C connection when done
    await ina226.close();
  } catch (err) {
    console.error('Error:', err.message);
    await ina226.close();
  }
}

main();
```

## API Documentation

### Constructor

```typescript
new INA226(options?: INA226Options)
```

Creates a new INA226 instance with the specified options:

- `busNumber` - I2C bus number (default: 1)
- `address` - I2C address of INA226 (default: 0x40)
- `rShunt` - Shunt resistor value in ohms (default: 0.1)
- `maxCurrent` - Maximum expected current in amps (default: 2.0)

### Methods

#### Basic Operations

- `begin()` - Initialize the INA226 device
- `close()` - Close the I2C connection
- `isConnected()` - Check if the device is connected
- `reset()` - Reset the device to default settings

#### Configuration

- `configure(options?)` - Configure the device with specified options:
  - `averages` - Number of samples to average
  - `busConvTime` - Bus voltage conversion time
  - `shuntConvTime` - Shunt voltage conversion time
  - `mode` - Operating mode
- `calibrate(rShuntValue, iMaxExpected)` - Calibrate for current/power measurements
- `calibrateForMilliAmps(maxMilliAmps, rShuntOhms)` - Calibrate for milliamp-range sensing

#### Power Management

- `powerUp()` - Power up the device
- `powerDown()` - Power down the device
- `triggerAndWait(mode)` - Trigger a one-shot measurement and wait for completion

#### Measurements

- `readShuntVoltage()` - Read the shunt voltage in volts
- `readBusVoltage()` - Read the bus voltage in volts
- `readShuntCurrent()` - Read the current in amps
- `readBusPower()` - Read the power in watts

#### Information

- `getMaxPossibleCurrent()` - Get the maximum possible current in amps
- `getMaxCurrent()` - Get the maximum current in amps
- `getMaxShuntVoltage()` - Get the maximum shunt voltage in volts
- `getMaxPower()` - Get the maximum power in watts
- `getAverages()` - Get the current averaging mode
- `getBusConversionTime()` - Get the current bus conversion time
- `getShuntConversionTime()` - Get the current shunt conversion time
- `getMode()` - Get the current operating mode
- `getLastError()` - Get the last error code
- `getLastErrorMessage()` - Get the last error as a string
- `getAlertType()` - Get current alert configuration
- `getAlertLimit()` - Get current alert limit value
- `printDebug()` - Print debug information to console
- `describeConfig()` - Get description of current configuration

#### Alerts
- `setMaskEnable(mask)` - Set the mask/enable register
- `getMaskEnable()` - Get the mask/enable register
- `enableShuntOverLimitAlert()` - Enable the shunt over-limit alert
- `enableShuntUnderLimitAlert()` - Enable the shunt under-limit alert
- `enableBusOverLimitAlert()` - Enable the bus over-limit alert
- `enableBusUnderLimitAlert()` - Enable the bus under-limit alert
- `enableOverPowerLimitAlert()` - Enable the power over-limit alert
- `enableConversionReadyAlert()` - Enable the conversion ready alert
- `setBusVoltageLimit(voltage)` - Set the bus voltage limit
- `setShuntVoltageLimit(voltage)` - Set the shunt voltage limit
- `setPowerLimit(watts)` - Set the power limit
- `setAlertInvertedPolarity(inverted)` - Set the alert polarity
- `setAlertLatch(latch)` - Set the alert latch mode
- `isMathOverflow()` - Check if math overflow occurred
- `isAlert()` - Check if alert flag is set

## License
MIT

## Credits
Based on the C++ ; by danel32
