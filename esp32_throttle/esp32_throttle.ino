#include <EEPROM.h>
#include "setup_functions.h"
#include "system_state.h"

// EEPROM configuration
#define EEPROM_SIZE 5                  // 2 bytes for min + 2 bytes for max + 1 byte for status
#define EEPROM_ADDR_THROTTLE_MIN 0     // uint16_t
#define EEPROM_ADDR_THROTTLE_MAX 2     // uint16_t
#define EEPROM_ADDR_THROTTLE_STATUS 4  // uint8_t

// Pins
const uint16_t THROTTLE_PIN = 2;      // GPIO2 - throttle input (analog)
const uint16_t THROTTLE_OUT_PIN = 3;  // GPIO3 - throttle/power output (analog)

// Default throttle values
const uint16_t THROTTLE_VALUE_MAX_DEFAULT = 3500;
const uint16_t THROTTLE_VALUE_MIN_DEFAULT = 1500;

// Power scale from 0 to 100
const uint16_t SCALE = 100;

// 10 seconds timeout for any tasks
const uint16_t TASK_TIMEOUT = 10000;  // max value: 65535 ~ 65sec ~ 1min

// Variables
uint16_t throttleValueMax = THROTTLE_VALUE_MAX_DEFAULT;
uint16_t throttleValueMin = THROTTLE_VALUE_MIN_DEFAULT;
uint16_t throttleValue = 0;
uint16_t powerValue = 0;

// System state
SystemState* systemState = nullptr;

void setSystemState(SystemState* newState) {
  if (systemState != nullptr) {
    delete systemState;
  }
  systemState = newState;
}

// === Communication abstraction ===

// Reads a command character from available source (Serial / Bluetooth in future)
char readCommand() {
  if (Serial.available()) {
    return Serial.read();
    // In future: SerialBT.read();
  }
  return 0;  // No command
}

// Sends a status message to the active output
void sendCommand(String message) {
  Serial.println(message);
  // In future: SerialBT.println(message);
}

// === Setup logic ===

void setup() {
  Serial.begin(115200);
  pinMode(THROTTLE_OUT_PIN, OUTPUT);

  InitializeThrottleStorageParameters initThrottleParams = InitializeThrottleStorageParameters(
    EEPROM_ADDR_THROTTLE_STATUS,
    EEPROM_ADDR_THROTTLE_MIN,
    EEPROM_ADDR_THROTTLE_MAX,
    THROTTLE_VALUE_MIN_DEFAULT,
    THROTTLE_VALUE_MAX_DEFAULT);

  InitializeStorageParameters initParams = InitializeStorageParameters(
    EEPROM_SIZE,
    initThrottleParams);

  InitializeStorageResult initResult = initializeStorage(initParams);

  InitializeThrottleStorageResult initThrottleResult = initResult.throttleStorageResult;
  if (!initThrottleResult.storageError) {
    if (initThrottleResult.isCalibrated) {
      throttleValueMax = initThrottleResult.valueMax;
      throttleValueMin = initThrottleResult.valueMin;
      sendCommand("Loaded throttle calibration values.");
      sendCommand("Max: " + String(initThrottleResult.valueMax));
      sendCommand("Min: " + String(initThrottleResult.valueMin));
    } else {
      sendCommand("Throttle calibration is not set - used default values.");
    }
  } else {
    sendCommand("Invalid throttle calibration values received from storage - used default values.");
  }

  if (initResult.storageError()) {
    setSystemState(new StorageErrorState());
  } else {
    setSystemState(new RunningState());
  }
}

// === Main loop input ===

void handleInput() {
  char cmd = readCommand();
  // IDLE
  if (cmd == 'e' && isSystemState(systemState, IDLE)) {
    sendCommand("Exiting idle.");
    setSystemState(new RunningState());
  } else if (cmd == 'r' && isSystemState(systemState, IDLE)) {
    resetESP();
  } else if (cmd == 'c' && isSystemState(systemState, IDLE)) {
    sendCommand("Starting throttle calibration...");
    setSystemState(new ThrottleCalibrationState());
  }
  // RUNNING
  else if (cmd == 'e' && isSystemState(systemState, RUNNING)) {
    sendCommand("Stopping program.");
    setSystemState(new IdleState());
    sendCommand("\nCommands: \n 'e' - exit idle state\n 'r' - reboot\n 'c' - calibrate\n");
  }
  // THROTTLE_CALIBRATION
  else if (cmd == 'e' && isSystemState(systemState, THROTTLE_CALIBRATION)) {
    sendCommand("Exiting throttle calibration - without saving.");
    setSystemState(new RunningState());
  } else if (cmd == 's' && isSystemState(systemState, THROTTLE_CALIBRATION)) {
    sendCommand("Exiting throttle calibration - saving.");
    saveThrottleCalibration();
    setSystemState(new RunningState());
  }
  // STORAGE_ERROR
  else if (cmd == 'e' && isSystemState(systemState, STORAGE_ERROR)) {
    sendCommand("Exiting storage error.");
    setSystemState(new RunningState());
  } else if (cmd == 'C' && isSystemState(systemState, STORAGE_ERROR)) {
    sendCommand("Clearing storage...");
    clearStorage();
    setSystemState(new RunningState());
  } else if (cmd == 'T' && isSystemState(systemState, STORAGE_ERROR)) {
    sendCommand("Recalibrate throttle...");
    setSystemState(new ThrottleCalibrationState());
  }
}

// === Main loop ===

void loop() {
  handleInput();

  if (isSystemState(systemState, RUNNING)) {
    runSystem();
  } else if (isSystemState(systemState, THROTTLE_CALIBRATION)) {
    calibrateThrottle();
  } else if (isSystemState(systemState, STORAGE_ERROR)) {
    storageError();
  } else if (isSystemState(systemState, IDLE)) {
    // Do idle
  } else {
    // Do nothing
  }
}

// == System state function ==

void calibrateThrottle() {
  ThrottleCalibrationState* currentState = static_cast<ThrottleCalibrationState*>(systemState);
  // disable output during calibration
  analogWrite(THROTTLE_OUT_PIN, 0);

  // read current throttle values
  uint16_t val = analogRead(THROTTLE_PIN);
  currentState->updateCalibrationValues(val);
  sendCommand("Reading: " + String(val) + " | MAX: " + String(currentState->maxValue) + " | MIN: " + String(currentState->minValue));
  delay(100);
}

void runSystem() {
  throttleValue = analogRead(THROTTLE_PIN);
  powerValue = throttleToPower(throttleValue);
  analogWrite(THROTTLE_OUT_PIN, powerValue * 2.55);  // scale 0–100% to 0–255

  sendCommand("Power: " + String(powerValue));
  delay(100);
}

void storageError() {
  delay(100);
}

// == System input values ==

void resetESP() {
  for (size_t i = 5; i > 0; i--) {
    sendCommand("Restarting ESP in " + String(i));
    delay(1000);
  }
  ESP.restart();
}

void saveThrottleCalibration() {
  ThrottleCalibrationState* currentState = static_cast<ThrottleCalibrationState*>(systemState);
  if (!currentState->isCalibrationCorrect()) {
    sendCommand("Calibration failed: Min value (" + String(currentState->minValue) + ") is greater than Max value (" + String(currentState->maxValue) + ").");
    return;
  }


  EEPROM.writeUShort(EEPROM_ADDR_THROTTLE_MIN, currentState->minValue);
  EEPROM.writeUShort(EEPROM_ADDR_THROTTLE_MAX, currentState->maxValue);
  EEPROM.writeUChar(EEPROM_ADDR_THROTTLE_STATUS, 'C');
  EEPROM.commit();

  throttleValueMin = currentState->minValue;
  throttleValueMax = currentState->maxValue;

  sendCommand("Calibration complete and saved.");
  sendCommand("New Max: " + String(throttleValueMax));
  sendCommand("New Min: " + String(throttleValueMin));
  return;
}

// == Other functions ==

// Converts analog throttle value to power percentage (0–100%)
uint16_t throttleToPower(uint16_t value) {
  uint16_t result = value;
  if (result > throttleValueMax) result = throttleValueMax;
  if (result < throttleValueMin) result = throttleValueMin;
  return map_uint16_t(result, throttleValueMin, throttleValueMax, 0, SCALE);
}

uint16_t map_uint16_t(uint16_t x, uint16_t in_min, uint16_t in_max, uint16_t out_min, uint16_t out_max) {
  const uint16_t run = in_max - in_min;
  if (run == 0) {
    log_e("map(): Invalid input range, min == max");
    return -1;  // AVR returns -1, SAM returns 0
  }
  const uint16_t rise = out_max - out_min;
  const uint16_t delta = x - in_min;
  return (delta * rise) / run + out_min;
}

void clearStorage() {
  for (size_t i = 0; i < EEPROM_SIZE; i++) {
    EEPROM.write(i, 255);
  }
  EEPROM.commit();
}