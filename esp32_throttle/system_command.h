#ifndef SYSTEM_COMMAND_H
#define SYSTEM_COMMAND_H

class SystemCommand {
private:
  char command;
  char serialCommand;
  char bluetoothCommand;

public:
  SystemCommand() {
    command = 0;
    serialCommand = 0;
    bluetoothCommand = 0;
  }

  ~SystemCommand() {
    clearAll();
  }

  void clearAll() {
    command = 0;
    serialCommand = 0;
    bluetoothCommand = 0;
  }

  void readCommand(char value) {
    if (command != 0) return;
    command = value;
  }

  char readSerialData() {
    if (Serial.available()) {
      char c = Serial.read();
      if (serialCommand == 0) {
        serialCommand = c;
      }
      // clear the rest of the serial data
      while (Serial.available()) {
        Serial.read();
      }
      return c;
    }
    return 0;
  }

  char getCommand() {
    char result = 0;
    if (command != 0) {
      result = command;
    } else if (bluetoothCommand != 0) {
      result = bluetoothCommand;
    } else if (serialCommand != 0) {
      result = serialCommand;
    }
    clearAll();
    return result;
  }
};

#endif
