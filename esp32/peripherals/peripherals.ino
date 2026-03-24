// Power the HW-038 water sensor with 3.3V
// Power the KY-018 with 5V
// With current code, low water level is around 100, can set a separate threshold of 150

int WATER_PIN = 1;
int LDR_PIN = 3;
int rawWater;
int rawLight;
int waterLevel;
int lightLevel;

void setup() {
  // Set baud rate
  Serial.begin(9600);
  pinMode(WATER_PIN, INPUT);
  pinMode(LDR_PIN, INPUT);
}

void loop() {
  // Updates reading of sensors
  rawWater = analogRead(WATER_PIN);
  rawLight = analogRead(LDR_PIN);
  waterLevel = (int)(((float)rawWater / 8191.0) * 255.0);
  // int lightLevel = (int)(((float)rawLight / 8191.0) * 255.0);

  // Prints values
  Serial.print("Raw reading: ");
  Serial.print(waterLevel);
  Serial.print(", ");
  Serial.println(rawLight);

  delay(1000);
}
