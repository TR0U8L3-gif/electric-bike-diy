#include <EEPROM.h>
#include "setup_functions.h"
#include "system_state.h"
#include "system_command.h"
#include "delay_timer.h"

// Constants
#define CAL_TO_METER 0.0254  // 1 inch = 0.0254 meters

#define THROTTLE_VALUE_MAX_DEFAULT 3500  // default max throttle value
#define THROTTLE_VALUE_MIN_DEFAULT 1500  // default min throttle value

#define WHEEL_SIZE_DEFAULT 29  // 29" wheel size in inches

#define RUNNING_MODE_DEFAULT 0  // 0 - speedometer, 1 - PAS, 2 - throttle

#define SCALE 100  // 0–100% power scale

#define TASK_TIMEOUT 10000  // 10 seconds in milliseconds

#define THROTTLE_PIN 2      // GPIO2 - throttle input (analog)
#define THROTTLE_OUT_PIN 3  // GPIO3 - throttle/power output (analog)
#define SPEEDOMETER_PIN 10  // GPIO10 - speedometer input (digital, interrupt)
#define LED_PIN 8           // GPIO8 - build in LED, HIGH -> off LOW -> on

#define EEPROM_SIZE 7
#define EEPROM_ADDR_THROTTLE_MIN 0     // uint16_t
#define EEPROM_ADDR_THROTTLE_MAX 2     // uint16_t
#define EEPROM_ADDR_THROTTLE_STATUS 4  // uint8_t
#define EEPROM_ADDR_WHEEL_SIZE 5       // uint8_t
#define EEPROM_ADDR_RUNNING_MODE 6     // uint8_t

// Variables
uint16_t throttleValue = 0;    // 0–4095 throttle value scale
uint8_t speedometerClick = 0;  // 0-1 speedometer click scale
uint8_t powerValue = 0;        // 0–100% power scale
uint8_t speedValue = 0;        // km/h speed scale

uint16_t throttleValueMax = THROTTLE_VALUE_MAX_DEFAULT;
uint16_t throttleValueMin = THROTTLE_VALUE_MIN_DEFAULT;
uint8_t wheelSize = WHEEL_SIZE_DEFAULT;

StorageErrorState* systemErrorState = nullptr;

SystemState* systemState = nullptr;

RunningMode runningMode = runningModeFromInt(RUNNING_MODE_DEFAULT);

SystemCommand* systemCommand = new SystemCommand();

DelayTimer* delayTimer = new DelayTimer();

// === Setup logic ===

void setup() {
  Serial.begin(115200);

  pinMode(THROTTLE_OUT_PIN, OUTPUT);
  pinMode(SPEEDOMETER_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);

  digitalWrite(LED_PIN, HIGH);

  // TODO(radek): Implement reading wheelsize and mode
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
    StorageErrorState* storageErrorState =
      new StorageErrorState(initResult.throttleStorageResult.storageError);
    systemErrorState = storageErrorState;
    setSystemState(storageErrorState);
  } else {
    setSystemState(new RunningState(runningMode));
  }

  delayTimer->reset();
}

// === Main loop input ===

bool handleEvent() {
  systemCommand->readSerialData();
  char cmd = systemCommand->getCommand();
  bool updatedSystemState = false;
  // IDLE
  if (cmd == 'e' && isSystemState(systemState, IDLE)) {
    sendCommand("Exiting idle.");
    setSystemState(new RunningState(runningMode));
    updatedSystemState = true;
  } else if (cmd == 's' && isSystemState(systemState, IDLE)) {
    sendCommand("Entering running state.");
    setSystemState(new RunningState(runningMode));
    updatedSystemState = true;
  } else if (cmd == 'r' && isSystemState(systemState, IDLE)) {
    resetESP();
  } else if (cmd == 'c' && isSystemState(systemState, IDLE)) {
    sendCommand("Starting throttle calibration...");
    setSystemState(new ThrottleCalibrationState());
    updatedSystemState = true;
  } else if (cmd == 't' && isSystemState(systemState, IDLE)) {
    sendCommand("Clearing throttle storage...");
    clearThrottleStorage();
  } else if (cmd == 'a' && isSystemState(systemState, IDLE)) {
    sendCommand("Clearing all storage...");
    clearAllStorage();
  } else if (cmd == 'x' && isSystemState(systemState, IDLE)) {
    sendCommand("Checking init exception");
    initException();
  }
  // RUNNING
  else if (cmd == 'e' && isSystemState(systemState, RUNNING)) {
    sendCommand("Stopping program.");
    setSystemState(new IdleState());
    updatedSystemState = true;
  }
  // THROTTLE_CALIBRATION
  else if (cmd == 'e' && isSystemState(systemState, THROTTLE_CALIBRATION)) {
    sendCommand("Exiting throttle calibration - without saving.");
    setSystemState(new IdleState());
    updatedSystemState = true;
  } else if (cmd == 's' && isSystemState(systemState, THROTTLE_CALIBRATION)) {
    sendCommand("Exiting throttle calibration - saving.");
    saveThrottleCalibration();
    setSystemState(new RunningState(runningMode));
    updatedSystemState = true;
  }
  // STORAGE_ERROR
  else if (cmd == 'e' && isSystemState(systemState, STORAGE_ERROR)) {
    sendCommand("Exiting storage error.");
    setSystemState(new RunningState(runningMode));
    updatedSystemState = true;
  } 

  return updatedSystemState;
}

// === Main loop ===

void loop() {
  bool updatedState = handleEvent();
  if (updatedState) {
    delayTimer->reset();
  } else {
    delayTimer->updateTimer(millis());
  }
  if (isSystemState(systemState, RUNNING)) {
    runSystem();
  } else if (isSystemState(systemState, THROTTLE_CALIBRATION)) {
    calibrateThrottle();
  } else if (isSystemState(systemState, STORAGE_ERROR)) {
    storageError();
  } else if (isSystemState(systemState, IDLE)) {
    idle();
  } else {
    // Do nothing
  }
}

// == System state function ==

void idle() {
  IdleState* currentState = static_cast<IdleState*>(systemState);
  analogWrite(THROTTLE_OUT_PIN, 0);
  digitalWrite(LED_PIN, LOW);

  unsigned long currentMillis = millis();
  bool isTimerSet = currentState->isTimerSet();
  if (!isTimerSet) {
    currentState->setTimer(currentMillis);
  } else {
    bool timeout = currentState->isTimeElapsed(currentMillis, TASK_TIMEOUT);
    if (timeout) {
      digitalWrite(LED_PIN, HIGH);
      systemCommand->readCommand('e');
      return;
    }
  }
  if (delayTimer->elapsed1000ms()) {
    int timeLeft = (int)floor((TASK_TIMEOUT - currentState->getTimeElapsed(currentMillis)) / 1000);
    sendCommand("\n\n\nCommands: \n 'e' - exit idle state\n 'r' - reboot\n 'c' - calibrate\n\nexit in (" + String(timeLeft) + ")");
  }
}

void calibrateThrottle() {
  analogWrite(THROTTLE_OUT_PIN, 0);
  
  ThrottleCalibrationState* currentState = static_cast<ThrottleCalibrationState*>(systemState);
  
  if (delayTimer->elapsed200ms()) {
    uint16_t val = analogRead(THROTTLE_PIN);
    currentState->updateCalibrationValues(val);
    sendCommand("Reading: " + String(val) + " | MAX: " + String(currentState->maxValue) + " | MIN: " + String(currentState->minValue));
  }
}

void storageError() {
  analogWrite(THROTTLE_OUT_PIN, 0);

  StorageErrorState* currentState = static_cast<StorageErrorState*>(systemState);

  unsigned long currentMillis = millis();
  bool isTimerSet = currentState->isTimerSet();
  if (!isTimerSet) {
    currentState->setTimer(currentMillis);
  } else {
    bool timeout = currentState->isTimeElapsed(currentMillis, TASK_TIMEOUT);
    if (timeout) {
      digitalWrite(LED_PIN, HIGH);
      systemCommand->readCommand('e');
      return;
    }
  }

  if (delayTimer->elapsed400ms()) {
    bool blink = currentState->swithLedStatus();
    if (blink) {
      digitalWrite(LED_PIN, LOW);
    } else {
      digitalWrite(LED_PIN, HIGH);
    }
  }
  if (delayTimer->elapsed800ms()) {
    sendCommand(currentState->getErrorMessage());
  }
}

void runSystem() {
  switch (runningMode) {
    case THROTTLE:
      return runSystemThrottle();
    case SPEEDOMETER:
      return runSystemSpeedometer();
    case PAS:
      return runSystemPas();
    default:
      return runSystemSpeedometer();
  }
}

void runSystemThrottle() {
  throttleValue = analogRead(THROTTLE_PIN);
  powerValue = throttleToPower(throttleValue);
  analogWrite(THROTTLE_OUT_PIN, powerValue * 2.55);  // scale 0–100% to 0–255

  if (delayTimer->elapsed100ms()) {
    sendCommand("Power: " + String(powerValue) + " Throttle: " + String(throttleValue));
  }
}

void runSystemSpeedometer() {
  throttleValue = analogRead(THROTTLE_PIN);
  speedometerClick = digitalRead(SPEEDOMETER_PIN);
  powerValue = 100 - throttleToPower(throttleValue);
  analogWrite(THROTTLE_OUT_PIN, powerValue * 2.55);  // scale 0–100% to 0–255

  if(delayTimer->elapsed100ms()) {
    sendCommand("Power: " + String(powerValue) + " Throttle: " + String(throttleValue) + " Click: " + String(speedometerClick));
  }
}

void runSystemPas() {
  if (delayTimer->elapsed100ms()) {
    sendCommand("Power: 0 (placeholder for PAS mode)");
  }
}

// == System event values ==

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

void initException() {
  if (systemErrorState == nullptr) {
    sendCommand("Initialization was successful");
  } else {
    sendCommand("Initialization ended in failure");
  }
}

void clearThrottleStorage() {
  EEPROM.writeUChar(EEPROM_ADDR_THROTTLE_STATUS, 255);
  EEPROM.commit();
}

void clearAllStorage() {
  for (size_t i = 0; i < EEPROM_SIZE; i++) {
    EEPROM.write(i, 255);
  }
  EEPROM.commit();
}

// == Other functions ==

// Sets the system state to a new state
void setSystemState(SystemState* newState) {
  if (systemState != nullptr) {
    delete systemState;
  }
  systemState = newState;
}

// TODO(radek): Implement Bluetooth communication
void sendCommand(String message) {
  Serial.println(message);
}

// Converts analog throttle value to power percentage (0–100%)
uint16_t throttleToPower(uint16_t value) {
  uint16_t result = value;
  if (result > throttleValueMax) result = throttleValueMax;
  if (result < throttleValueMin) result = throttleValueMin;
  return map_uint16_t(result, throttleValueMin, throttleValueMax, 0, SCALE);
}

// Maps a value from one range to another
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
