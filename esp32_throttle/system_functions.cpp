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
  float wheel_circumference_m = wheel_size * CAL_TO_METER * PI;
  float time_sec = wheel_circumference_m / KMH_TO_MS;
  return static_cast<unsigned long>(time_sec * 1000);
}

uint16_t throttleToPower(uint16_t value, uint16_t throttleValueMin, uint16_t throttleValueMax) {
  uint16_t result = value;
  if (result > throttleValueMax) result = throttleValueMax;
  if (result < throttleValueMin) result = throttleValueMin;
  return map_uint16_t(result, throttleValueMin, throttleValueMax, 0, SCALE);
}

uint16_t interpolatePower(float s, float s1, float s2, uint16_t p1, uint16_t p2) {
  if (s2 == s1) return p1;
  float t = (s - s1) / (s2 - s1);
  return uint16_t(p1 + t * (p2 - p1));
}

uint16_t assistToPower(float speed_ms, AssistCurve& curve) {
  if (speed_ms < curve.level.speed_ms_min || speed_ms > curve.level.speed_ms_max) {
    return 0;
  }

  AssistPoint& p1 = curve.p1;
  AssistPoint& p2 = curve.p2;
  AssistPoint& p3 = curve.p3;
  AssistPoint& p4 = curve.p4;

  if (speed_ms <= p2.speed_ms) {
    return interpolatePower(speed_ms, p1.speed_ms, p2.speed_ms, p1.power, p2.power);
  } else if (speed_ms <= p3.speed_ms) {
    return interpolatePower(speed_ms, p2.speed_ms, p3.speed_ms, p2.power, p3.power);
  } else {
    return interpolatePower(speed_ms, p3.speed_ms, p4.speed_ms, p3.power, p4.power);
  }
}

float computeSpeed(uint8_t wheelSizeInInches, unsigned long timeDeltaMs) {
  if (timeDeltaMs == 0) return 0.0f;
  return (wheelSizeInInches * CAL_TO_METER * PI) / ((float)timeDeltaMs / 1000.0f);
}

unsigned long computeDeltaTime(uint8_t wheel_size, float speed_ms) {
  if (speed_ms <= 0.0f) return 0.0f;
  float wheel_circumference_m = wheel_size * CAL_TO_METER * PI;
  return static_cast<unsigned long>((wheel_circumference_m / speed_ms) * 1000);
}

bool speedDeltaTimeThreshold(unsigned long time, uint8_t wheel_size, float speed_ms) {
  if (time == 0) return false;
  if (speed_ms <= 0) return false;
  return time > (computeDeltaTime(wheel_size, speed_ms) * 2);
}
