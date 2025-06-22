#include "system_state.h"

bool isSystemState(SystemState* state, const char* type) {
  return strcmp(state->type(), type) == 0;
}

uint8_t runningModeToInt(RunningMode value) {
  switch (value) {
    case THROTTLE:
      return 2;
    case SPEEDOMETER:
      return 0;
    case PAS:
      return 1;
    default:
      return 0;
  }
}

RunningMode runningModeFromInt(uint8_t value) {
  switch (value) {
    case 2:
      return THROTTLE;
    case 0:
      return SPEEDOMETER;
    case 1:
      return PAS;
    default:
      return SPEEDOMETER;
  }
}