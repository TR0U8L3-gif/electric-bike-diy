// pins
const int THROTTLE_PIN = 2; // GPIO2  - throttle (analog)
const int THROTTLE_OUT_PIN = 3; // GPIO3 - throttle / power output (analog)

// const variables
const unsigned int SCALE = 100;   // power scale from 0 to 100

// variables
unsigned int throttleValueMax = 3550;
unsigned int throttleValueMin = 1450;

unsigned int throttleValue = 0;
unsigned int powerValue = 0;

void setup() {
  Serial.begin(115200);
  pinMode(THROTTLE_OUT_PIN, OUTPUT); 
}

void loop() {
  throttleValue = analogRead(THROTTLE_PIN);
  powerValue = throttleToPower(throttleValue);
  analogWrite(THROTTLE_OUT_PIN, powerValue * 2.55);

  // Serial.print("Throttle:");
  // Serial.print(throttleValue);
  // Serial.println(""); 

  Serial.print("Power:");
  Serial.print(powerValue);
  Serial.println(""); 
  delay(100);
}

// convert `throttle` analog values (0-4096) to `power` 0-100 %  
int throttleToPower(int value) {
  int result = value;
  if(result > throttleValueMax) result = throttleValueMax;
  if(result < throttleValueMin) result = throttleValueMin;
  return map(result, throttleValueMin, throttleValueMax, 0, SCALE);
}