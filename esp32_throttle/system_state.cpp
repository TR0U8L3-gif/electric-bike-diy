#include "system_state.h"

bool isSystemState(SystemState* state, const char* type){
  return strcmp(state->type(), type) == 0;
}