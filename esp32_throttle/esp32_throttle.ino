#include <EEPROM.h>
#include "setup_functions.h"
#include "system_state.h"
#include "system_command.h"
#include "delay_timer.h"
#include "system_functions.h"
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>


// Constants
#define MIN_CLICK_INTERVAL 300  // Min time between clicks (in ms)
#define MAX_CLICK_INTERVAL 350  // Max time between clicks (in ms)
#define MIN_CLICK_COUNT 4       // Min time between clicks (in ms)
#define MAX_CLICK_COUNT 6       // Max time between clicks (in ms)
#define POWER_THRESHOLD_MAX 40  // Maximum power value to consider throttle clicked
#define POWER_THRESHOLD_MIN 20  // Minimum power value to consider throttle clicked
#define POWER_OFFSET 15         // Offset power value to consider throttle clicked

#define THROTTLE_VALUE_MAX_DEFAULT 3500  // default max throttle value
#define THROTTLE_VALUE_MIN_DEFAULT 1500  // default min throttle value
#define WHEEL_SIZE_DEFAULT 29            // 29" wheel size in inches
#define RUNNING_MODE_DEFAULT 0           // 0 - speedometer, 1 - PAS, 2 - throttle
#define TASK_TIMEOUT 10000               // 10 seconds in milliseconds

#define THROTTLE_PIN 4      // GPIO4 - throttle input (analog)
#define THROTTLE_OUT_PIN 2  // GPIO2 - throttle/power output (analog)
#define SPEEDOMETER_PIN 10  // GPIO10 - speedometer input (digital, interrupt)
#define LED_PIN 8           // GPIO8 - build in LED, HIGH -> off LOW -> on

#define EEPROM_SIZE 10
#define EEPROM_ADDR_THROTTLE_MIN 0      // 0-1 uint16_t
#define EEPROM_ADDR_THROTTLE_MAX 2      // 2-3 uint16_t
#define EEPROM_ADDR_THROTTLE_STATUS 4   // 4-4 uint8_t
#define EEPROM_ADDR_WHEEL_SIZE 5        // 5-6 uint16_t
#define EEPROM_ADDR_RUNNING_MODE_INT 7  // 7-8 uint16_t
#define EEPROM_ADDR_BIKE_STATUS 9       // 9-9 uint8_t

#define SERVICE_UUID "cbed4dc2-b095-4226-8806-81693f5a33b2"
#define CHARACTERISTIC_UUID "b8c8be62-5acb-4e39-b245-242418e5fe8b"

// Variables
bool deviceConnected = false;            // BLE device connection status
char bleCommand = '\0';                  // Character received from BLE
uint16_t throttleValue = 0;              // 0–4095 throttle value scale
uint8_t powerValue = 0;                  // 0–100% power scale
float speedometerSpeed = 0;              // m/s speed scale
bool speedometerValue = false;           // 0-1 speedometer value scale "click"
unsigned long speedometerTime = 0;       // speedometer time in milliseconds
unsigned long speedometerTimeDelta = 0;  // speedometer time delta in milliseconds

uint16_t throttleValueMax = THROTTLE_VALUE_MAX_DEFAULT;  // storage implemented
uint16_t throttleValueMin = THROTTLE_VALUE_MIN_DEFAULT;  // storage implemented
uint16_t wheelSize = WHEEL_SIZE_DEFAULT;                 // storage implemented
uint16_t runningModeInt = RUNNING_MODE_DEFAULT;          // storage implemented

BLECharacteristic* pCharacteristic = nullptr;

StorageErrorState* systemErrorState = nullptr;

SystemState* systemState = nullptr;

RunningMode runningMode = runningModeFromInt(runningModeInt);

SystemCommand* systemCommand = new SystemCommand();

DelayTimer* delayTimer = new DelayTimer();

// === Classes ===

class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    deviceConnected = true;
  }

  void onDisconnect(BLEServer* pServer) {
    deviceConnected = false;
  }
};

class CommandCallback : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* pCharacteristic) {
    String value = pCharacteristic->getValue();
    if (value.length() > 0) {
      char cmd = value[0];
      systemCommand->readBluetoothData(cmd);
    }
  }
};


// === Setup logic ===

void setup() {
  Serial.begin(115200);

  BLEDevice::init("Wykurwisty Rower Radka");
  BLEServer* pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService* pService = pServer->createService(SERVICE_UUID);
  pCharacteristic = pService->createCharacteristic(
    CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY);

  pCharacteristic->addDescriptor(new BLE2902());
  pCharacteristic->setCallbacks(new CommandCallback());

  pCharacteristic->setValue("Waiting for connection...");
  pService->start();

  BLEAdvertising* pAdvertising = pServer->getAdvertising();
  pAdvertising->start();

  Serial.println("BLE Started, waiting for client connection...");

  pinMode(THROTTLE_OUT_PIN, OUTPUT);
  pinMode(SPEEDOMETER_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);

  digitalWrite(LED_PIN, HIGH);

  // TODO(radek): Implement reading Assist -> Levels Points and AssistCurve

  InitializeBikeStorageParameters initBikeParams = InitializeBikeStorageParameters(
    EEPROM_ADDR_BIKE_STATUS,
    EEPROM_ADDR_WHEEL_SIZE,
    EEPROM_ADDR_RUNNING_MODE_INT,
    WHEEL_SIZE_DEFAULT,
    RUNNING_MODE_DEFAULT);

  InitializeThrottleStorageParameters initThrottleParams = InitializeThrottleStorageParameters(
    EEPROM_ADDR_THROTTLE_STATUS,
    EEPROM_ADDR_THROTTLE_MIN,
    EEPROM_ADDR_THROTTLE_MAX,
    THROTTLE_VALUE_MIN_DEFAULT,
    THROTTLE_VALUE_MAX_DEFAULT);

  InitializeStorageParameters initParams = InitializeStorageParameters(
    EEPROM_SIZE,
    initThrottleParams,
    initBikeParams);

  InitializeStorageResult initResult = initializeStorage(initParams);

  InitializeBikeStorageResult initBikeResult = initResult.bikeStorageResult;
  if (!initBikeResult.storageError) {
    if (initBikeResult.isCalibrated) {
      wheelSize = initBikeResult.wheelSize;
      runningMode = runningModeFromInt(initBikeResult.runningModeint);
      sendCommand("Loaded bike calibration values.");
      sendCommand("wheel size: " + String(initBikeResult.wheelSize));
      sendCommand("running mode: " + String(initBikeResult.runningModeint));
    } else {
      sendCommand("Bike calibration is not set - used default values.");
    }
  } else {
    sendCommand("Invalid bike calibration values received from storage - used default values.");
  }

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
      new StorageErrorState(
        initResult.throttleStorageResult.storageError,
        initResult.bikeStorageResult.storageError);

    systemErrorState = storageErrorState;
    setSystemState(storageErrorState);
  } else {
    setSystemState(new RunningState());
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
    setSystemState(new RunningState());
    updatedSystemState = true;
  } else if (cmd == 'm' && isSystemState(systemState, IDLE)) {
    sendCommand("Starting running mode calibration...");
    setSystemState(new RunningModeCalibrationState(runningMode));
    updatedSystemState = true;
  } else if (cmd == 'w' && isSystemState(systemState, IDLE)) {
    sendCommand("Starting wheel size calibration...");
    setSystemState(new WheelSizeCalibrationState(wheelSize));
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
    updatedSystemState = true;
  } else if (cmd == 'b' && isSystemState(systemState, IDLE)) {
    sendCommand("Clearing bike storage...");
    clearBikeStorage();
    updatedSystemState = true;
  } else if (cmd == 'a' && isSystemState(systemState, IDLE)) {
    sendCommand("Clearing all storage...");
    clearAllStorage();
    updatedSystemState = true;
  } else if (cmd == 'x' && isSystemState(systemState, IDLE)) {
    sendCommand("Checking init exception");
    initException();
    updatedSystemState = true;
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
    setSystemState(new RunningState());
    updatedSystemState = true;
  }
  // STORAGE_ERROR
  else if (cmd == 'e' && isSystemState(systemState, STORAGE_ERROR)) {
    sendCommand("Exiting storage error.");
    setSystemState(new RunningState());
    updatedSystemState = true;
  }
  // WHEEL_SIZE_CALIBRATION
  else if (cmd == 'e' && isSystemState(systemState, WHEEL_SIZE_CALIBRATION)) {
    sendCommand("Exiting wheel size calibration - without saving.");
    setSystemState(new IdleState());
    updatedSystemState = true;
  } else if (cmd == 's' && isSystemState(systemState, WHEEL_SIZE_CALIBRATION)) {
    saveWheelSizeCalibration();
    setSystemState(new IdleState());
    updatedSystemState = true;
  } else if (isSystemState(systemState, WHEEL_SIZE_CALIBRATION)) {
    calibrateWheelSize(cmd);
  }
  // RUNNING_MODE_CALIBRATION
  else if (cmd == 'e' && isSystemState(systemState, RUNNING_MODE_CALIBRATION)) {
    sendCommand("Exiting running mode calibration - without saving.");
    setSystemState(new IdleState());
    updatedSystemState = true;
  } else if (cmd == 's' && isSystemState(systemState, RUNNING_MODE_CALIBRATION)) {
    sendCommand("Exiting running mode calibration - saving.");
    saveRunningModeCalibration();
    setSystemState(new IdleState());
    updatedSystemState = true;
  } else if (isSystemState(systemState, RUNNING_MODE_CALIBRATION)) {
    calibrateRunningMode(cmd);
  } else if (cmd == '0') {
    runningMode = SPEEDOMETER;
  } else if (cmd == '1') {
    runningMode = PAS;
  } else if (cmd == '2') {
    runningMode = THROTTLE;
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
  } else if (isSystemState(systemState, RUNNING_MODE_CALIBRATION)) {
    calibrateRunningMode(-1);
  } else if (isSystemState(systemState, WHEEL_SIZE_CALIBRATION)) {
    calibrateWheelSize(-1);
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
    sendCommand("\n\n\nCommands: \n 'e' - exit idle state\n 'r' - reboot\n 'c' - calibrate throttle\n 'm' - setup running mode\n 'w' - setup wheel size\n 'x' - check init exception\n 't' - clear throttle storage\n 'b' - clear bike storage\n 'a' - clear all storages\n\nexit in (" + String(timeLeft) + ")");
    sendCommandBLE("idle");
  }
}

void calibrateRunningMode(char value) {
  RunningModeCalibrationState* currentState = static_cast<RunningModeCalibrationState*>(systemState);

  if (value >= 0) {
    currentState->updateCalibration(value);
  }

  if (delayTimer->elapsed1000ms()) {
    sendCommand("Press '0' for Speedometer, '1' for PAS, '2' for Throttle\nCurrent running mode: " + String(currentState->mode) + "\nPress 's' to save the value.\n\n");
    sendCommandBLE("c-rm," + String(currentState->mode));
  }
}

void calibrateWheelSize(char value) {
  WheelSizeCalibrationState* currentState = static_cast<WheelSizeCalibrationState*>(systemState);

  if (value >= 0) {
    currentState->updateCalibration(value);
  }

  if (delayTimer->elapsed1000ms()) {
    sendCommand("Enter number from 0 to 9 to change wheel size, wheel will be calculated by formula 20\" + (entered_number)\".\nExample for 9 -> 29\" wheel size.\nCurrent wheel size: " + String(currentState->wheelSize) + "\"\nPress 's' to save the value.\n\n");
    sendCommandBLE("c-ws," + String(currentState->wheelSize));
  }
}

void calibrateThrottle() {
  analogWrite(THROTTLE_OUT_PIN, 0);

  ThrottleCalibrationState* currentState = static_cast<ThrottleCalibrationState*>(systemState);

  if (delayTimer->elapsed200ms()) {
    uint16_t val = analogRead(THROTTLE_PIN);
    currentState->updateCalibrationValues(val);
    sendCommand("Reading: " + String(val) + " | MAX: " + String(currentState->maxValue) + " | MIN: " + String(currentState->minValue));
    sendCommandBLE("c-th," + String(currentState->maxValue) + "," + String(currentState->minValue));
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
    sendCommandBLE(currentState->getErrorMessageShort());
  }
}

void runSystem() {
  RunningState* currentState = static_cast<RunningState*>(systemState);

  uint8_t power = throttleToPower(throttleValue, throttleValueMin, throttleValueMax);
  bool isThrottleHigh = power >= (POWER_THRESHOLD_MAX + POWER_OFFSET);
  bool isThrottleLow = power <= (POWER_THRESHOLD_MAX - POWER_OFFSET);

  if (isThrottleHigh && !currentState->isThrottleClicked) {
    unsigned long currentTime = millis();
    if (currentState->clickCount > clickCount()) {
      currentState->lastClickTime = currentTime;
    } else if (currentState->clickCount == 0 || (currentTime - currentState->lastClickTime <= clickInterval())) {
      currentState->clickCount++;
      currentState->lastClickTime = currentTime;
    } else {
      currentState->clickCount = 0;
      currentState->lastClickTime = currentTime;
    }
    currentState->isThrottleClicked = true;
  }
  if (isThrottleLow && currentState->isThrottleClicked) {
    currentState->isThrottleClicked = false;
  }

  if (millis() - currentState->lastClickTime > clickInterval()) {
    if (currentState->clickCount == clickCount() && runningMode == SPEEDOMETER) {
      sendCommand(String(clickCount()) + " throttle clicks detected!");
      currentState->clickCount = 0;
      changeModeClick();
      return;
    }
    if (currentState->clickCount > 0) {
      currentState->clickCount = 0;
    }
  }

  if (currentState->clickCount >= clickCount() && runningMode != SPEEDOMETER) {
    sendCommand(String(clickCount()) + " throttle clicks detected!");
    currentState->clickCount = 0;
    changeModeClick();
    return;
  }

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
  RunningState* currentState = static_cast<RunningState*>(systemState);

  throttleValue = analogRead(THROTTLE_PIN);
  powerValue = throttleToPower(throttleValue, throttleValueMin, throttleValueMax);
  analogWrite(THROTTLE_OUT_PIN, map(powerValue, 0, 100, 0, 255));  // scale 0–100% to 0–255

  if (delayTimer->elapsed100ms()) {
    sendCommand("Power: " + String(powerValue) + " Throttle: " + String(throttleValue) + " [" + String(currentState->clickCount) + "/" + String(clickCount()) + "] " + String(currentState->isThrottleClicked ? "clicked" : ""));
    sendCommandBLE("th," + String(powerValue) + "," + String(throttleValue));
  }
}

void runSystemSpeedometer() {
  RunningState* currentState = static_cast<RunningState*>(systemState);

  throttleValue = analogRead(THROTTLE_PIN);
  bool speedometerClick = digitalRead(SPEEDOMETER_PIN);

  bool speedometerClickVerified = false;
  unsigned long currentMillis = millis();

  if (speedometerClick == true && speedometerValue == false) {
    speedometerClickVerified = true;
    speedometerValue = true;
    speedometerTimeDelta = currentMillis - speedometerTime;
    speedometerTime = currentMillis;
  }
  if (speedometerClick == false && speedometerValue == true) {
    speedometerValue = false;
  }

  bool isFasterThan1kmh = (currentMillis - speedometerTime) <= timeFor1Kmh(wheelSize);
  bool isSpeedThreshold = speedDeltaTimeThreshold((currentMillis - speedometerTime), wheelSize, speedometerSpeed);
  if (!isFasterThan1kmh || isSpeedThreshold || speedometerTimeDelta == 0) {
    speedometerSpeed = (float)0;
    speedometerTimeDelta = 0;
  } else {
    speedometerSpeed = computeSpeed(wheelSize, speedometerTimeDelta);
  }

  AssistCurve assistCurveLow = {
    // AssistPoint (curve points)
    { 0.0 * KMH_TO_MS, 70 },
    { 6.0 * KMH_TO_MS, 90 },
    { 15.0 * KMH_TO_MS, 70 },
    { 20.0 * KMH_TO_MS, 0 },

    // AssistLevel (active range)
    { 3.0 * KMH_TO_MS, 20.0 * KMH_TO_MS }
  };

  unsigned int throttlePower = throttleToPower(throttleValue, throttleValueMin, throttleValueMax);
  unsigned int assistPower = assistToPower(speedometerSpeed, assistCurveLow);

  if (throttlePower > POWER_THRESHOLD_MIN) {
    currentState->isPowerOff = true;
    currentState->minSpeedCutOff = speedometerSpeed;
  } else {
    if (currentState->isPowerOff) {
      if (speedometerSpeed < currentState->minSpeedCutOff) {
        currentState->minSpeedCutOff = speedometerSpeed;
      }
      if (speedometerSpeed >= currentState->minSpeedCutOff + 1.0) {
        currentState->isPowerOff = false;
      }
    }
  }

  if (currentState->isPowerOff) {
    powerValue = 0;
  } else {
    powerValue = assistPower;
  }

  analogWrite(THROTTLE_OUT_PIN, map(powerValue, 0, 100, 0, 255));  // scale 0–100% to 0–255

  if (delayTimer->elapsed100ms() || speedometerClick) {
    sendCommand("Power: " + String(powerValue) + "[" + String(currentState->isPowerOff ? ("Off - " + String(currentState->minSpeedCutOff * MS_TO_KMS) + "km\\h") : "On") + "] Speed: " + String(speedometerSpeed * MS_TO_KMS) + "km\\h Throttle: " + String(throttleValue) + " Click: " + String(speedometerClick ? 1 : 0) + "/" + String(speedometerValue ? 1 : 0) + "=" + String(speedometerClickVerified ? 1 : 0) + " [" + String(currentState->clickCount) + "/" + String(clickCount()) + "] " + String(currentState->isThrottleClicked ? "clicked" : ""));
    sendCommandBLE("sp," + String(powerValue) + "," + String(speedometerSpeed * MS_TO_KMS) + "," + String(currentState->isPowerOff ? "0" : "1") + "," + String(speedometerClick ? "1" : "0"));
  }
}

void runSystemPas() {
  if (delayTimer->elapsed100ms()) {
    sendCommand("Power: 0 (placeholder for PAS mode)");
    sendCommandBLE("pas");
  }
}

// == System event values ==

void resetESP() {
  sendCommandBLE("reset");
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
    sendCommandBLE("s-th,f," + String(throttleValueMax) + "," + String(throttleValueMin));
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
  sendCommandBLE("s-th,s," + String(throttleValueMax) + "," + String(throttleValueMin));
  return;
}

void saveWheelSizeCalibration() {
  WheelSizeCalibrationState* currentState = static_cast<WheelSizeCalibrationState*>(systemState);
  if (!currentState->isCalibrationCorrect()) {
    sendCommand("Calibration failed: Invalid wheel size.");
    sendCommandBLE("s-ws,f");
    return;
  }

  EEPROM.writeUShort(EEPROM_ADDR_WHEEL_SIZE, currentState->wheelSize);

  unsigned char bikeStatus = EEPROM.readUChar(EEPROM_ADDR_BIKE_STATUS);

  if (bikeStatus != 'C') {
    EEPROM.writeUShort(EEPROM_ADDR_RUNNING_MODE_INT, runningModeInt);
  }

  EEPROM.writeUChar(EEPROM_ADDR_BIKE_STATUS, 'C');
  EEPROM.commit();

  wheelSize = currentState->wheelSize;

  sendCommand("Calibration complete and saved.");
  sendCommand("New Wheel Size: " + String(wheelSize) + "\"");
  sendCommandBLE("s-ws,s," + String(wheelSize));
  return;
}

void saveRunningModeCalibration() {
  RunningModeCalibrationState* currentState = static_cast<RunningModeCalibrationState*>(systemState);
  if (!currentState->isCalibrationCorrect()) {
    sendCommand("Calibration failed: Invalid running mode.");
    sendCommandBLE("s-rm,f");
    return;
  }

  EEPROM.writeUChar(EEPROM_ADDR_RUNNING_MODE_INT, runningModeToInt(currentState->mode));

  unsigned char bikeStatus = EEPROM.readUChar(EEPROM_ADDR_BIKE_STATUS);

  if (bikeStatus != 'C') {
    EEPROM.writeUShort(EEPROM_ADDR_WHEEL_SIZE, wheelSize);
  }

  EEPROM.writeUChar(EEPROM_ADDR_BIKE_STATUS, 'C');
  EEPROM.commit();

  runningModeInt = runningModeToInt(currentState->mode);
  runningMode = currentState->mode;

  sendCommand("Calibration complete and saved.");
  sendCommand("New Running Mode: " + String(runningMode));
  sendCommandBLE("s-rm,s," + String(runningMode));
  return;
}

void initException() {
  if (systemErrorState == nullptr) {
    sendCommand("Initialization was successful");
    sendCommandBLE("init-e,s");
  } else {
    sendCommand("Initialization ended in failure");
    sendCommandBLE("init-e,f");
  }
}

void clearThrottleStorage() {
  EEPROM.writeUChar(EEPROM_ADDR_THROTTLE_STATUS, 255);
  EEPROM.commit();
}

void clearBikeStorage() {
  EEPROM.writeUChar(EEPROM_ADDR_BIKE_STATUS, 255);
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

void sendCommand(String message) {
  Serial.println(message);
}

void sendCommandBLE(String message) {
  if (deviceConnected) {
    pCharacteristic->setValue(message.c_str());
    pCharacteristic->notify();  // Notify BLE client
  }
}

unsigned int clickCount() {
  switch (runningMode) {
    case THROTTLE:
      return MIN_CLICK_COUNT;
    case SPEEDOMETER:
      return MAX_CLICK_COUNT;
    case PAS:
      return MIN_CLICK_COUNT;
    default:
      return MIN_CLICK_COUNT;
  }
}

unsigned int clickInterval() {
  switch (runningMode) {
    case THROTTLE:
      return MIN_CLICK_INTERVAL;
    case SPEEDOMETER:
      return MAX_CLICK_INTERVAL;
    case PAS:
      return MIN_CLICK_INTERVAL;
    default:
      return MIN_CLICK_INTERVAL;
  }
}

void changeModeClick() {
  switch (runningMode) {
    case THROTTLE:
      runningMode = SPEEDOMETER;
      break;
    case SPEEDOMETER:
      //runningMode = THROTTLE;
      break;
    case PAS:
      runningMode = SPEEDOMETER;
      break;
    default:
      runningMode = SPEEDOMETER;
      break;
  }
}