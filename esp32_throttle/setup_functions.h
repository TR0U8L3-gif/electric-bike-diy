#ifndef SETUP_FUNCTIONS_H
#define SETUP_FUNCTIONS_H
#include <sys/_stdint.h>
#include <EEPROM.h>

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
  InitializeStorageParameters(
    const size_t eepromSize_val,
    const InitializeThrottleStorageParameters& throttleStorageParams_val)
    : eepromSize(eepromSize_val),
      throttleStorageParams(throttleStorageParams_val) {}
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

  InitializeStorageResult(
    const InitializeThrottleStorageResult& throttleStorageResult_val)
    : throttleStorageResult(throttleStorageResult_val) {}

  bool storageError() {
    return throttleStorageResult.storageError;
  }
};


InitializeStorageResult initializeStorage(InitializeStorageParameters params);

#endif