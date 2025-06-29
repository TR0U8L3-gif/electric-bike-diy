#ifndef SYSTEM_FUNCTIONS_H
#define SYSTEM_FUNCTIONS_H
#include <cstdint>

#define CAL_TO_METER 0.0254  // 1 inch = 0.0254 meters
#define KMH_TO_MS 0.27778    // 1 km/h = ~0.27778 m/s
#define MS_TO_KMS 3.6        // 1 m/s = 3.6 km/h
#define SCALE 100            // 0–100% power scale
#define PI 3.14159           // Pi constant

struct AssistLevel {
  float speed_ms_min;  // min speed in m/s
  float speed_ms_max;  //  max speed in m/s
};

struct AssistPoint {
  float speed_ms;  // speed in m/s
  uint16_t power;  // 0-100%
};

struct AssistCurve {
  AssistPoint p1;     // first point in the curve
  AssistPoint p2;     // second point in the curve
  AssistPoint p3;     // third point in the curve
  AssistPoint p4;     // fourth point in the curve
  AssistLevel level;  // level of assistance
};

uint16_t map_uint16_t(uint16_t x, uint16_t in_min, uint16_t in_max, uint16_t out_min, uint16_t out_max);

unsigned long timeFor1Kmh(uint8_t wheel_size);

uint16_t throttleToPower(uint16_t value, uint16_t throttleValueMin, uint16_t throttleValueMax);

uint16_t assistToPower(float speed_ms, AssistCurve& curve);

float computeSpeed(uint8_t wheelSizeInInches, unsigned long timeDeltaMs);

unsigned long computeDeltaTime(uint8_t wheel_size, float speed_ms);

bool speedDeltaTimeThreshold(unsigned long time, uint8_t wheel_size, float speed_ms);

#endif
