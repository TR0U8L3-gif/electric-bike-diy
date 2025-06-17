#include <EEPROM.h>
#include "setup_functions.h"

// EEPROM configuration
#define EEPROM_SIZE 9  // 4 bytes for min + 4 bytes for max + 1 byte for status
#define EEPROM_ADDR_THROTTLE_MIN 0
#define EEPROM_ADDR_THROTTLE_MAX 4
#define EEPROM_ADDR_THROTTLE_STATUS 8

// Pins
const unsigned int THROTTLE_PIN = 2;      // GPIO2 - throttle input (analog)
const unsigned int THROTTLE_OUT_PIN = 3;  // GPIO3 - throttle/power output (analog)

// Default throttle values
const unsigned int THROTTLE_VALUE_MAX_DEFAULT = 3500;
const unsigned int THROTTLE_VALUE_MIN_DEFAULT = 1450;

// Power scale from 0 to 100
const unsigned int SCALE = 100;

// 10 seconds timeout for any tasks
const unsigned long TASK_TIMEOUT = 10000;

// Variables
unsigned int throttleValueMax = THROTTLE_VALUE_MAX_DEFAULT;
unsigned int throttleValueMin = THROTTLE_VALUE_MIN_DEFAULT;
unsigned int throttleValue = 0;
unsigned int powerValue = 0;

// System State
enum SystemState {
  IDLE,
  RUNNING,
  THROTTLE_CALIBRATION,
  STORAGE_ERROR,
};

SystemState systemState = RUNNING;

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
    THROTTLE_VALUE_MAX_DEFAULT
  );

  InitializeStorageParameters initParams = InitializeStorageParameters(
    EEPROM_SIZE, 
    initThrottleParams
  );

  InitializeStorageResult initResult = initializeStorage(initParams);
  
  InitializeThrottleStorageResult initThrottleResult = initResult.throttleStorageResult; 
  if(!initThrottleResult.storageError){
    if(initThrottleResult.isCalibrated){
      throttleValueMax = initThrottleResult.valueMax;
      throttleValueMin = initThrottleResult.valueMin;
      sendCommand("Loaded throttle calibration values.");
      sendCommand("Min: " + String(initThrottleResult.valueMax));
      sendCommand("Max: " + String(initThrottleResult.valueMin));
    } else {
      sendCommand("Throttle calibation is not set - used default values.");
    }
  } else {
    sendCommand("Invalid throttle calibration values received from storage - used default values.");
  }

  if(initResult.storageError()){
    systemState = STORAGE_ERROR;
  } 
}

// === Main loop helper functions ===

void handleInput() {
  char cmd = readCommand();
  if ( cmd == 'e' && systemState == IDLE){
    sendCommand("Exiting idle.");
    systemState = RUNNING;
  } else if( cmd == 'e' && systemState == RUNNING){
    sendCommand("Stopping program.");
    systemState = IDLE;
  }
  else if (cmd == 'c' && systemState != THROTTLE_CALIBRATION) {
    sendCommand("Starting throttle calibration...");
    systemState = THROTTLE_CALIBRATION;
  } else if (cmd == 's' && systemState == THROTTLE_CALIBRATION) {
    sendCommand("Exiting throttle calibration.");
    systemState = RUNNING;
  } else if (cmd == 'C' && systemState == STORAGE_ERROR){
    sendCommand("Clearing storage...");
    clearStorage();
    systemState = RUNNING;
  } else if (cmd == 'T' && systemState == STORAGE_ERROR){
    sendCommand("Recalibrate throttle...");
    systemState = THROTTLE_CALIBRATION;
  } else if (cmd == 'e' && systemState == STORAGE_ERROR){
    sendCommand("Exiting storage error.");
    systemState = RUNNING;
  } else if (cmd != 0) {
    sendCommand("Unknown command: " + String(cmd));
  }
}

// === Main loop ===

void loop() {
  handleInput();

    switch (systemState) {
    case RUNNING:
      runSystem();
      break;
    case THROTTLE_CALIBRATION:
      calibrateThrottle();
      break;
    case STORAGE_ERROR:
      storageError();
      break;  
    case IDLE:
    default:
      // Do nothing
      break;
  }
}

// == System state function ==

void runSystem() {
  throttleValue = analogRead(THROTTLE_PIN);
  powerValue = throttleToPower(throttleValue);
  analogWrite(THROTTLE_OUT_PIN, powerValue * 2.55);  // scale 0–100% to 0–255

  sendCommand("Power: " + String(powerValue));
  delay(100);
}

void calibrateThrottle() {
  // read current throttle values
  int val = analogRead(THROTTLE_PIN);
  // disable output during calibration
  analogWrite(THROTTLE_OUT_PIN, 0);  
    // if (val > maxVal) maxVal = val;
    // if (val < minVal) minVal = val;

  sendCommand("Reading: " + String(val));
  delay(100);
}

void storageError(){
  sendCommand("Init Storage Error");
  delay(100);
}

// == Other functions ==

// Converts analog throttle value to power percentage (0–100%)
int throttleToPower(int value) {
  int result = value;
  if (result > throttleValueMax) result = throttleValueMax;
  if (result < throttleValueMin) result = throttleValueMin;
  return map(result, throttleValueMin, throttleValueMax, 0, SCALE);
}

void clearStorage(){
  for (size_t i = 0; i < EEPROM_SIZE; i++)
  {
    EEPROM.write(i, 255);
  }
  EEPROM.commit();
}