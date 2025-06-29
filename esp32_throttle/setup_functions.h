#ifndef SETUP_FUNCTIONS_H
#define SETUP_FUNCTIONS_H
#include <EEPROM.h>

class InitializeBikeStorageParameters {
public:
  uint16_t eepromAddrStatus;
  uint16_t eepromAddrWheelSize;
  uint16_t eepromAddrRunningModeInt;
  uint16_t wheelSizeDefault;
  uint16_t runningModeIntDefault;

  InitializeBikeStorageParameters(
    const uint16_t eepromAddrStatus_val,
    const uint16_t eepromAddrWheelSize_val,
    const uint16_t eepromAddrRunningModeInt_val,
    const uint16_t wheelSizeDefault_val,
    const uint16_t runningModeIntDefault_val)
    : eepromAddrStatus(eepromAddrStatus_val),
      eepromAddrWheelSize(eepromAddrWheelSize_val),
      eepromAddrRunningModeInt(eepromAddrRunningModeInt_val),
      wheelSizeDefault(wheelSizeDefault_val),
      runningModeIntDefault(runningModeIntDefault_val) {}
};

class InitializeThrottleStorageParameters {
public:
  uint16_t eepromAddrStatus;
  uint16_t eepromAddrMin;
  uint16_t eepromAddrMax;
  uint16_t defaultMin;
  uint16_t defaultMax;

  InitializeThrottleStorageParameters(
    const uint16_t eepromAddrStatus_val,
    const uint16_t eepromAddrMin_val,
    const uint16_t eepromAddrMax_val,
    const uint16_t defaultMin_val,
    const uint16_t defaultMax_val)
    : eepromAddrStatus(eepromAddrStatus_val),
      eepromAddrMin(eepromAddrMin_val),
      eepromAddrMax(eepromAddrMax_val),
      defaultMin(defaultMin_val),
      defaultMax(defaultMax_val) {}
};

class InitializeStorageParameters {
public:
  size_t eepromSize;
  InitializeThrottleStorageParameters throttleStorageParams;
  InitializeBikeStorageParameters bikeStorageParams;
  InitializeStorageParameters(
    const size_t eepromSize_val,
    const InitializeThrottleStorageParameters& throttleStorageParams_val,
    const InitializeBikeStorageParameters& bikeStorageParams_val)
    : eepromSize(eepromSize_val),
      throttleStorageParams(throttleStorageParams_val),
      bikeStorageParams(bikeStorageParams_val) {}
};

class InitializeBikeStorageResult {
public:
  bool storageError;
  bool isCalibrated;
  uint16_t wheelSize;
  uint16_t runningModeint;

  InitializeBikeStorageResult(
    const bool storageError_val,
    const bool isCalibrated_val,
    const uint16_t wheelSize_val,
    const uint16_t runningModeint_val)
    : storageError(storageError_val),
      isCalibrated(isCalibrated_val),
      wheelSize(wheelSize_val),
      runningModeint(runningModeint_val) {}
};

class InitializeThrottleStorageResult {
public:
  bool storageError;
  bool isCalibrated;
  uint16_t valueMin;
  uint16_t valueMax;

  InitializeThrottleStorageResult(
    const bool storageError_val,
    const bool isCalibrated_val,
    const uint16_t valueMin_val,
    const uint16_t valueMax_val)
    : storageError(storageError_val),
      isCalibrated(isCalibrated_val),
      valueMin(valueMin_val),
      valueMax(valueMax_val) {}
};

class InitializeStorageResult {
public:
  InitializeThrottleStorageResult throttleStorageResult;
  InitializeBikeStorageResult bikeStorageResult;

  InitializeStorageResult(
    const InitializeThrottleStorageResult& throttleStorageResult_val,
    const InitializeBikeStorageResult& bikeStorageResult_val)
    : throttleStorageResult(throttleStorageResult_val),
      bikeStorageResult(bikeStorageResult_val) {}

  bool storageError() {
    return throttleStorageResult.storageError || bikeStorageResult.storageError;
  }
};

InitializeStorageResult initializeStorage(InitializeStorageParameters params);

#endif