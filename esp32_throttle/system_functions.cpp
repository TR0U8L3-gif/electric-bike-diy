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