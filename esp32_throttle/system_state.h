#ifndef SYSTEM_STATE_H
#define SYSTEM_STATE_H
#include <cstring>

// System States
constexpr const char* IDLE = "IDLE";
constexpr const char* RUNNING = "RUNNING";
constexpr const char* THROTTLE_CALIBRATION = "THROTTLE_CALIBRATION";
constexpr const char* WHEEL_SIZE_CALIBRATION = "WHEEL_SIZE_CALIBRATION";
constexpr const char* RUNNING_MODE_CALIBRATION = "RUNNING_MODE_CALIBRATION";
constexpr const char* STORAGE_ERROR = "STORAGE_ERROR";

class SystemState {
protected:
  unsigned long timer;
public:
  SystemState()
    : timer(0) {}
  virtual ~SystemState() = default;

  virtual const char* type() const = 0;

  bool isTimerSet() {
    return timer != 0;
  }

  void setTimer(unsigned long time) {
    timer = time;
  }

  void resetTimer() {
    timer = 0;
  }

  unsigned long getTime() {
    return timer;
  }

  unsigned long getTimeElapsed(unsigned long time) {
    return time - timer;
  }

  bool isTimeElapsed(unsigned long time, unsigned long delay) {
    if (timer == 0) {
      timer = time;
      return false;
    } else {
      return time - timer >= delay;
    }
  }
};

class IdleState : public SystemState {
public:
  const char* type() const override {
    return IDLE;
  }
};

enum RunningMode {
  SPEEDOMETER,
  PAS,
  THROTTLE
};

class RunningState : public SystemState {
public:
  bool isPowerOff;
  float minSpeedCutOff;
  bool isThrottleClicked;
  unsigned int clickCount;
  unsigned long lastClickTime;
  RunningState()
    : isPowerOff(false), minSpeedCutOff(0.0), isThrottleClicked(false), clickCount(0), lastClickTime(0) {}

  const char* type() const override {
    return RUNNING;
  }
};

class ThrottleCalibrationState : public SystemState {
public:
  uint16_t minValue;
  uint16_t maxValue;

  ThrottleCalibrationState()
    : minValue(4095),
      maxValue(0) {}

  const char* type() const override {
    return THROTTLE_CALIBRATION;
  }

  void updateCalibrationValues(uint16_t currentRead) {
    if (currentRead < minValue) {
      minValue = currentRead;
    }
    if (currentRead > maxValue) {
      maxValue = currentRead;
    }
  }

  bool isCalibrationCorrect() {
    return minValue < maxValue;
  }
};

class WheelSizeCalibrationState : public SystemState {
public:
  uint16_t wheelSize;
  WheelSizeCalibrationState(uint16_t wheelSizeDefault_val)
    : wheelSize(wheelSizeDefault_val) {}

  const char* type() const override {
    return WHEEL_SIZE_CALIBRATION;
  }

  void updateCalibration(char value) {
    switch (value) {
      case '9':
        wheelSize = 29;
        break;
      case '8':
        wheelSize = 28;
        break;
      case '7':
        wheelSize = 27;
        break;
      case '6':
        wheelSize = 26;
        break;
      case '5':
        wheelSize = 25;
        break;
      case '4':
        wheelSize = 24;
        break;
      case '3':
        wheelSize = 23;
        break;
      case '2':
        wheelSize = 22;
        break;
      case '1':
        wheelSize = 21;
        break;
      case '0':
        wheelSize = 20;
        break;
    }
  }

  bool isCalibrationCorrect() {
    return wheelSize <= 29 && wheelSize >= 20;
  }
};

class RunningModeCalibrationState : public SystemState {
public:
  RunningMode mode;
  RunningMode modeDefault;
  RunningModeCalibrationState(RunningMode modeDefault_val)
    : mode(modeDefault_val),
      modeDefault(modeDefault_val) {}

  const char* type() const override {
    return RUNNING_MODE_CALIBRATION;
  }

  void updateCalibration(char value) {
    switch (value) {
      case '0':
        mode = SPEEDOMETER;
        break;
      case '1':
        mode = PAS;
        break;
      case '2':
        mode = THROTTLE;
        break;
    }
  }

  bool isCalibrationCorrect() {
    return true;
  }
};

class StorageErrorState : public SystemState {
public:
  bool throttleCalibrationError;
  bool bikeCalibrationError;
  bool ledStatus;
  StorageErrorState(bool throttleCalibrationError_val = false, bool bikeCalibrationError_val = false)
    : throttleCalibrationError(throttleCalibrationError_val),
      bikeCalibrationError(bikeCalibrationError_val),
      ledStatus(true) {}

  const char* type() const override {
    return STORAGE_ERROR;
  }

  bool swithLedStatus() {
    ledStatus = !ledStatus;
    return !ledStatus;
  }

  char* getErrorMessage() {
    if (throttleCalibrationError) {
      return "error: Throttle calibration error. Please calibrate the throttle.";
    } else {
      return "error: Storage error. Please reset the storage.";
    }
  }
};

bool isSystemState(SystemState* state, const char* type);

uint8_t runningModeToInt(RunningMode value);

RunningMode runningModeFromInt(uint8_t value);

#endif
