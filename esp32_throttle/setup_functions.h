#ifndef SETUP_FUNCTIONS_H
#define SETUP_FUNCTIONS_H
#include <EEPROM.h>

class InitializeThrottleStorageParameters {
public:
  int eepromAddrStatus;
  int eepromAddrMin;
  int eepromAddrMax;
  int defaultMin;
  int defaultMax;

  InitializeThrottleStorageParameters(
    const int eepromAddrStatus_val,
    const int eepromAddrMin_val,
    const int eepromAddrMax_val,
    const int defaultMin_val,
    const int defaultMax_val)
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
  int valueMin;
  int valueMax;

  InitializeThrottleStorageResult(
    const bool storageError_val,
    const bool isCalibrated_val,
    const int valueMin_val,
    const int valueMax_val)
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