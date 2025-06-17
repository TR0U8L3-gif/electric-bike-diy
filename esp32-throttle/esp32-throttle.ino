#include <EEPROM.h>

// EEPROM configuration
#define EEPROM_SIZE 9           // 4 bytes for min + 4 bytes for max + 1 byte for status
#define EEPROM_ADDR_THROTTLE_MIN 0
#define EEPROM_ADDR_THROTTLE_MAX 4
#define EEPROM_ADDR_THROTTLE_STATUS 8

// Pins
const unsigned int THROTTLE_PIN = 2;       // GPIO2 - throttle input (analog)
const unsigned int THROTTLE_OUT_PIN = 3;   // GPIO3 - throttle/power output (analog)

// Default throttle values
const unsigned int THROTTLE_VALUE_MAX_DEFAULT = 3500;
const unsigned int THROTTLE_VALUE_MIN_DEFAULT = 1450;

// Power scale from 0 to 100
const unsigned int SCALE = 100;

// Variables
unsigned int throttleValueMax = THROTTLE_VALUE_MAX_DEFAULT;
unsigned int throttleValueMin = THROTTLE_VALUE_MIN_DEFAULT;
unsigned int throttleValue = 0;
unsigned int powerValue = 0;
bool storageError = false;

// === Communication abstraction ===

// Reads a command character from available source (Serial / Bluetooth in future)
char readCommand() {
  if (Serial.available()) {
    return Serial.read();
    // In future: SerialBT.read();
  }
  return 0; // No command
}

// Sends a status message to the active output
void sendCommand(String message) {
  Serial.println(message);
  // In future: SerialBT.println(message);
}

// === Main program logic ===

void setup() {
  Serial.begin(115200);
  pinMode(THROTTLE_OUT_PIN, OUTPUT);

  initializeStorage();
}

void loop() {
  char cmd = readCommand();
  if (cmd == 'c') {
    calibrateThrottle();
  }

  throttleValue = analogRead(THROTTLE_PIN);
  powerValue = throttleToPower(throttleValue);
  analogWrite(THROTTLE_OUT_PIN, powerValue * 2.55);  // scale 0–100% to 0–255

  sendCommand("Power: " + String(powerValue) + "%");

  delay(100);
}

// == Other functions == 

// Initializes EEPROM storage
void initializeStorage(){
  // Initialize EEPROM
  EEPROM.begin(EEPROM_SIZE);

  initializeThrottleStorage();
}

// Initializes throttle storage by reading calibration values from EEPROM
void initializeThrottleStorage() {
  // read throttle calibration values from EEPROM
  char throttleCalibrationStatus = EEPROM.read(EEPROM_ADDR_THROTTLE_STATUS);

  // if the status is not set to 'C', we assume calibration has not been done
  if(throttleCalibrationStatus == 'C'){
    int throttleValueMinStorage, throttleValueMaxStorage = 0; 
    // Load throttle min/max from EEPROM
    EEPROM.get(EEPROM_ADDR_THROTTLE_MIN, throttleValueMinStorage);
    EEPROM.get(EEPROM_ADDR_THROTTLE_MAX, throttleValueMaxStorage);
  
    // If EEPROM contains invalid values, fall back to defaults
    if (throttleValueMinStorage < 0 || throttleValueMinStorage > 4095) {
      storageError = true;
    }
    if (throttleValueMax < 0 || throttleValueMax > 4095 || throttleValueMax <= throttleValueMin) {
      storageError = true;
    }

    if(!storageError){
      sendCommand("Loaded throttle calibration values:");
      sendCommand("Min: " + String(throttleValueMinStorage));
      sendCommand("Max: " + String(throttleValueMaxStorage));
      sendCommand("Type 'c' to start throttle calibration.");
      throttleValueMin = throttleValueMinStorage;
      throttleValueMax = throttleValueMaxStorage;
    } else {
      sendCommand("Invalid throttle calibration values in EEPROM.");
      sendCommand("Using default values: Min = " + String(THROTTLE_VALUE_MIN_DEFAULT) + ", Max = " + String(THROTTLE_VALUE_MAX_DEFAULT));
      throttleValueMin = THROTTLE_VALUE_MIN_DEFAULT;
      throttleValueMax = THROTTLE_VALUE_MAX_DEFAULT;
    }
  }
}

// Converts analog throttle value to power percentage (0–100%)
int throttleToPower(int value) {
  int result = value;
  if(result > throttleValueMax) result = throttleValueMax;
  if(result < throttleValueMin) result = throttleValueMin;
  return map(result, throttleValueMin, throttleValueMax, 0, SCALE);
}

// Throttle calibration function
void calibrateThrottle() {
  sendCommand("== Throttle Calibration Mode ==");
  sendCommand("Move the throttle through its full range.");
  sendCommand("Press 's' to save and exit calibration.");

  int minVal = 4095;
  int maxVal = 0;

  while (true) {
    char endCmd = readCommand();
    if (endCmd == 's') {
      break;
    }

    int val = analogRead(THROTTLE_PIN);
    if (val > maxVal) maxVal = val;
    if (val < minVal) minVal = val;

    analogWrite(THROTTLE_OUT_PIN, 0);  // disable output during calibration

    sendCommand("Reading: " + String(val) + " | Min: " + String(minVal) + " | Max: " + String(maxVal));
    delay(100);
  }

  if (minVal >= maxVal) {
    sendCommand("Calibration failed: Min value is not less than Max value.");
    return;
  }

  // Save new min/max values
  throttleValueMin = minVal;
  throttleValueMax = maxVal;

  EEPROM.put(EEPROM_ADDR_THROTTLE_MIN, throttleValueMin);
  EEPROM.put(EEPROM_ADDR_THROTTLE_MAX, throttleValueMax);
  EEPROM.write(EEPROM_ADDR_THROTTLE_STATUS, 'C'); // Set status to 'C' for calibrated
  EEPROM.commit();

  sendCommand("Calibration complete and saved.");
  sendCommand("New Min: " + String(throttleValueMin));
  sendCommand("New Max: " + String(throttleValueMax));
}
