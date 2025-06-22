#include <sys/_stdint.h>
#include "system_functions.h"

uint16_t map_uint16_t(uint16_t x, uint16_t in_min, uint16_t in_max, uint16_t out_min, uint16_t out_max) {
  const uint16_t run = in_max - in_min;
  const uint16_t rise = out_max - out_min;
  if (run == 0) {
    return (in_max + in_min) / 2;
  }
  const uint16_t delta = x - in_min;
  return (delta * rise) / run + out_min;
}

unsigned long timeFor1Kmh(uint8_t wheel_size) {
  float wheel_circumference_m = wheel_size * CAL_TO_METER;
  float time_sec = wheel_circumference_m / KMH_TO_MS;
  return static_cast<unsigned long>(time_sec * 1000);
}

uint16_t throttleToPower(uint16_t value, uint16_t throttleValueMin, uint16_t throttleValueMax) {
  uint16_t result = value;
  if (result > throttleValueMax) result = throttleValueMax;
  if (result < throttleValueMin) result = throttleValueMin;
  return map_uint16_t(result, throttleValueMin, throttleValueMax, 0, SCALE);
}

float computeSpeed(uint8_t wheelSizeInInches, unsigned long timeDeltaMs) {
  if (timeDeltaMs == 0) return 0.0f;
  return (wheelSizeInInches * CAL_TO_METER * PI) / ((float)timeDeltaMs / 1000.0f);
}

unsigned long computeDeltaTime(uint8_t wheel_size, float speed_ms) {
  if (speed_ms <= 0.0f) return 0.0f;
  float wheel_circumference_m = PI * wheel_size * CAL_TO_METER;
  return static_cast<unsigned long>((wheel_circumference_m / speed_ms) * 1000);
}

bool speedDeltaTimeThreshold(unsigned long time, uint8_t wheel_size, float speed_ms) {
  if (time == 0) return false;
  if (speed_ms <= 0) return false;
  return time > (computeDeltaTime(wheel_size, speed_ms) * 2);
}
