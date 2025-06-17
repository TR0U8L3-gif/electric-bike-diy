#include "setup_functions.h"

// Initializes throttle storage
InitializeThrottleStorageResult initializeThrottleStorage(InitializeThrottleStorageParameters params) {
  // read throttle calibration values from EEPROM
  bool storageError = false;
  bool isCalibrated = false;
  int throttleValueMinStorage, throttleValueMaxStorage, throttleValueMin, throttleValueMax = 0;
  char throttleCalibrationStatus = EEPROM.read(params.eepromAddrStatus);


  // if the status is not set to 'C', we assume calibration has not been done
  if (throttleCalibrationStatus == 'C') {
    isCalibrated = true;

    EEPROM.get(params.eepromAddrMin, throttleValueMinStorage);
    EEPROM.get(params.eepromAddrMax, throttleValueMaxStorage);

    if (throttleValueMinStorage < 0 || throttleValueMinStorage > 4095) {
      storageError = true;
    }
    if (throttleValueMax < 0 || throttleValueMax > 4095 || throttleValueMax <= throttleValueMin) {
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
    throttleValueMax
  );
}

// Initializes storage
InitializeStorageResult initializeStorage(InitializeStorageParameters params) {
  // Initialize EEPROM
  EEPROM.begin(params.eepromSize);

  InitializeThrottleStorageResult throttleResult = initializeThrottleStorage(params.throttleStorageParams);

  return InitializeStorageResult(
    throttleResult
  );
}