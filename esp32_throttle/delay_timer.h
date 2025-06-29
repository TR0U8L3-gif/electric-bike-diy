#ifndef DElAY_TIMER_H
#define DElAY_TIMER_H

class DelayTimer {
private:
  unsigned long millis100 = 0;
  unsigned long millis200 = 0;
  unsigned long millis400 = 0;
  unsigned long millis800 = 0;
  unsigned long millis1000 = 0;
public:
  DelayTimer() {
    reset();
  }

  void updateTimer(unsigned long millis) {
    if (millis100 == 0) {
      millis100 = millis + 1;
    }
    if (millis200 == 0) {
      millis200 = millis + 1;
    }
    if (millis400 == 0) {
      millis400 = millis + 1;
    }
    if (millis800 == 0) {
      millis800 = millis + 1;
    }
    if (millis1000 == 0) {
      millis1000 = millis + 1;
    }
    if (millis > millis100 && millis - millis100 >= 100) {
      millis100 = 0;
    }
    if (millis > millis200 && millis - millis200 >= 200) {
      millis200 = 0;
    }
    if (millis > millis400 && millis - millis400 >= 400) {
      millis400 = 0;
    }
    if (millis > millis800 && millis - millis800 >= 800) {
      millis800 = 0;
    }
    if (millis > millis1000 && millis - millis1000 >= 1000) {
      millis1000 = 0;
    }
  }
  bool elapsed100ms() {
    return millis100 == 0;
  }
  bool elapsed200ms() {
    return millis200 == 0;
  }
  bool elapsed400ms() {
    return millis400 == 0;
  }
  bool elapsed800ms() {
    return millis800 == 0;
  }
  bool elapsed1000ms() {
    return millis1000 == 0;
  }
  void reset() {
    millis100 = 0;
    millis200 = 0;
    millis400 = 0;
    millis800 = 0;
    millis1000 = 0;
  }
};

#endif