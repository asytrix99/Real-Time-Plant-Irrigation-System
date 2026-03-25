// Power the HW-038 water sensor with 3.3V
// With current code, low water level is around 100, can set a separate threshold of 150
// Servo motor requires 5 V
// Power the KY-018 photoresistor or KY-022 IR receiver with 5V


#include <ESP32Servo.h>

Servo valve;

int WATER_PIN = 1;
int LDR_PIN = 3;
int SERVO_PIN = 4;
int rawWater;
int rawLight;
int waterLevel;
int lightLevel;
static int angle = 0;

void setup() {
  // Set baud rate
  Serial.begin(9600);
  pinMode(WATER_PIN, INPUT);
  pinMode(LDR_PIN, INPUT);
  pinMode(SERVO_PIN, OUTPUT);
  valve.attach(SERVO_PIN, 500, 2400);
  valve.write(0);
}

void loop() {
  // Updates reading of sensors
  rawWater = analogRead(WATER_PIN);
  rawLight = analogRead(LDR_PIN);
  waterLevel = (int)(((float)rawWater / 8191.0) * 255.0);
  lightLevel = (int)(((float)rawLight / 8191.0) * 255.0);

  // Prints values
  Serial.print("Raw reading: ");
  Serial.print(waterLevel);
  Serial.print(", ");
  Serial.println(rawLight);

  if (angle == 0) {
    angle = 90;
  } else {
    angle = 0;
  }
  valve.write(angle);

  delay(2000);
}
