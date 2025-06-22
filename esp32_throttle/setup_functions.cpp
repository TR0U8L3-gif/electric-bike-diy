#include "setup_functions.h"

// Initializes throttle storage
InitializeThrottleStorageResult initializeThrottleStorage(InitializeThrottleStorageParameters params) {
  // read throttle calibration values from EEPROM
  bool storageError = false;
  bool isCalibrated = false;
  uint16_t throttleValueMinStorage = 0;
  uint16_t throttleValueMaxStorage = 0;
  uint16_t throttleValueMin, throttleValueMax = 0;
  uint8_t throttleCalibrationStatus = EEPROM.readUChar(params.eepromAddrStatus);


  // if the status is not set to 'C', we assume calibration has not been done
  if (throttleCalibrationStatus == 'C') {
    isCalibrated = true;

    throttleValueMinStorage = EEPROM.readUShort(params.eepromAddrMin);
    throttleValueMaxStorage = EEPROM.readUShort(params.eepromAddrMax);

    if (throttleValueMinStorage < 0 || throttleValueMinStorage > 4095) {
      storageError = true;
    }
    if (throttleValueMaxStorage < 0 || throttleValueMaxStorage > 4095 || throttleValueMaxStorage <= throttleValueMinStorage) {
      storageError = true;
    }

    if (!storageError) {
      throttleValueMin = throttleValueMinStorage;
      throttleValueMax = throttleValueMaxStorage;
    } else {
      throttleValueMin = params.defaultMin;
      throttleValueMax = params.defaultMax;
    }
  }

  return InitializeThrottleStorageResult(
    storageError,
    isCalibrated,
    throttleValueMin,
    throttleValueMax);
}

// Initializes storage
InitializeStorageResult initializeStorage(InitializeStorageParameters params) {
  // Initialize EEPROM
  EEPROM.begin(params.eepromSize);

  InitializeThrottleStorageResult throttleResult = initializeThrottleStorage(params.throttleStorageParams);

  return InitializeStorageResult(
    throttleResult);
}