#ifndef SYSTEM_STATE_H
#define SYSTEM_STATE_H
#include <cstring>

// System States
constexpr const char* IDLE = "IDLE";
constexpr const char* RUNNING = "RUNNING";
constexpr const char* THROTTLE_CALIBRATION = "THROTTLE_CALIBRATION";
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
  THROTTLE,
  SPEEDOMETER,
  PAS
};

class RunningState : public SystemState {
public:
  RunningMode mode;

  RunningState(RunningMode mode_val)
    : mode(mode_val) {}

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

class StorageErrorState : public SystemState {
public:
  bool throttleCalibrationError;
  bool ledStatus;
  StorageErrorState(bool throttleCalibrationError_val = false)
    : throttleCalibrationError(throttleCalibrationError_val),
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
