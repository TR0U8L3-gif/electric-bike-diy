#ifndef SYSTEM_STATE_H
#define SYSTEM_STATE_H
#include <cstring>

// System States
constexpr const char* IDLE = "IDLE";
constexpr const char* RUNNING = "RUNNING";
constexpr const char* THROTTLE_CALIBRATION = "THROTTLE_CALIBRATION";
constexpr const char* STORAGE_ERROR = "STORAGE_ERROR";

class SystemState {
public:
  virtual ~SystemState() = default;
  virtual const char* type() const = 0;
};

class IdleState : public SystemState {
public:
  const char* type() const override {
    return IDLE;
  }
};

class RunningState : public SystemState {
public:
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
  const char* type() const override {
    return STORAGE_ERROR;
  }
};

bool isSystemState(SystemState* state, const char* type);

#endif