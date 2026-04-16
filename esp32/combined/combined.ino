#include <WiFi.h>
#include <HTTPClient.h>
#include <Arduino_JSON.h>
#include <WiFiClientSecure.h>
#include <time.h>
#include <ESP32Servo.h>

// Wifi connection and password
const char *ssid = "Xiaomi 13 Pro";
const char *password = "1234567890";

// API variables
String openWeatherMapApiKey = "11d9fd741957988badffa1134a497315";
String city = "Singapore";
String countryCode = "SG";
String weatherCondition;

// Variables for controlling API fetch frequency
unsigned long lastTime = 0;
unsigned long timerDelay = 10000; // Api call delay : 10 seconds
unsigned long lastLowWaterAlertTime = 0;
unsigned long lowWaterAlertCooldown = 60000;

// Telegram bot variables
String botToken = "7698137105:AAEttnPiSC-jwm-TEy5URZkxQg0FrFB6JqI";
String chatId = "7404385637";
const char *ntp = "pool.ntp.org";
const long offset = 8 * 3600; // 8h * 3600seconds

// Pin declarations
const int NEW_TX_PIN = 1;
const int NEW_RX_PIN = 2;
int WATER_PIN = 3;
int LDR_PIN = 4;
int SERVO_PIN = 5;
int SENSOR_POWER_PIN = 6;

// Global variables for sensor values
int rawWater;
int rawLight;
unsigned char waterLevel;
unsigned char lightLevel;
unsigned long servoOpenTime = 0;
bool isValveOpen = false;

// Servo variables
Servo valve;
static int angle_closed = 0;
static int angle_open = 90;

void setup()
{
  // Initialize debug UART and MCXC444 UART link
  Serial.begin(115200);                                    // Serial monitor
  Serial1.begin(9600, SERIAL_8N1, NEW_RX_PIN, NEW_TX_PIN); // UART
  Serial.print("Connecting to SSID: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  valve.attach(SERVO_PIN, 500, 2400); // Grants 180 degrees of rotation
  valve.write(0);

  pinMode(SENSOR_POWER_PIN, OUTPUT);
  digitalWrite(SENSOR_POWER_PIN, LOW);

  // Block until WiFi is connected before starting cloud operations
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(250);
    Serial.print(".");
  }

  Serial.println("\nConnected to WiFi!");

  // Sync RTC so watering alerts include local timestamp
  configTime(offset, 0, ntp);
  Serial.println("Time configured via NTP.");
  // sendTelegramMessage("ALERT: Water tank is critically low! Please refill.");
}

void loop()
{

  // Refresh local sensor readings for the next packet
  readSensors();

  // Periodically fetch weather and publish combined status to MCXC444
  if ((millis() - lastTime) > timerDelay)
  {
    if (WiFi.status() == WL_CONNECTED)
    {
      String jsonBuffer = fetchAPIData();
      JSONVar myObject = JSON.parse(jsonBuffer);
      if (JSON.typeof(myObject) == "undefined")
      {
        Serial.println("Parsing input failed!");
        return;
      }

      weatherCondition = (const char *)myObject["weather"][0]["main"]; // Extract the "main" weather condition

      // Packetize weather, water, and light into a UART-safe message
      String packetToSend = createMessageString();

      printTransmittedMessage(packetToSend);
    }
    else
    {
      Serial.println("WiFi Disconnected");
    }
    lastTime = millis();
  }

  if (Serial1.available())
  {
    // Consume one newline-terminated control packet from MCXC444
    String incomingData = Serial1.readStringUntil('\n');
    Serial.print("Received from MCXC444: ");
    Serial.println(incomingData);

    processMessage(incomingData);
  }

  // Auto-close valve after brief watering pulse
  if (isValveOpen && (millis() - servoOpenTime >= 500))
  {
    closeValve();
    isValveOpen = false;
  }
}

// Build and issue OpenWeatherMap request, then return raw JSON payload
String fetchAPIData()
{
  String serverPath = "http://api.openweathermap.org/data/2.5/weather?q=" + city + "," + countryCode + "&APPID=" + openWeatherMapApiKey;
  String jsonBuffer = httpGETRequest(serverPath.c_str());
  return jsonBuffer;
}

// Encode weather condition + sensor levels into the agreed UART protocol
String createMessageString()
{
  String packetToSend = "";
  if (weatherCondition == "Clear")
  {
    packetToSend = "<W,S>"; // Sunny
  }
  else if (weatherCondition == "Rain" || weatherCondition == "Thunderstorm" || weatherCondition == "Drizzle")
  {
    packetToSend = "<W,R>"; // Rainy
  }
  else
  {
    packetToSend = "<W,C>"; // Cloudy
  }
  packetToSend += "<" + String(waterLevel) + ", " + String(lightLevel) + ">";
  return packetToSend;
}

// Send one telemetry packet and mirror it on serial debug output
void printTransmittedMessage(String packetToSend)
{
  Serial1.println(packetToSend);
  Serial.print("Raw Weather Condition: ");
  Serial.println(weatherCondition);
  Serial.print("Sent to MCXC444: ");
  Serial.println(packetToSend);
  Serial.println("-------------------------");
}

unsigned long lastWaterTime = 0;
unsigned long waterCooldown = 30000; // 10s cooldown for open valve

void processMessage(String incomingData)
{
  // Low-water tank alert path with Telegram rate limiting
  if (incomingData.indexOf("<A,LOW>") >= 0)
  {
    if (millis() - lastLowWaterAlertTime >= lowWaterAlertCooldown)
    {
      lastLowWaterAlertTime = millis();
      sendTelegramMessage("ALERT: Water tank is critically low! Please refill.");
    }
    else
    {
      Serial.println("Low water alert suppressed by cooldown.");
    }
  }
  // Dry-soil alert path: open valve briefly and send timestamped notification
  else if (incomingData.indexOf("<A,D>") >= 0)
  {
    if (millis() - lastWaterTime >= waterCooldown)
    {
      lastWaterTime = millis();
      openValve();
      isValveOpen = true;
      servoOpenTime = millis();

      struct tm timeinfo;
      if (!getLocalTime(&timeinfo))
      {
        Serial.println("Failed to obtain time");
        sendTelegramMessage("Watering! (Failed to obtain time)");
      }
      else
      {
        char timeStr[50];
        strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", &timeinfo);
        String msg = "Watering!\\n Time: " + String(timeStr);
        sendTelegramMessage(msg);
      }
    }
    else
    {
      Serial.println("Watering cooldown active...");
    }
  }
  // Manual override path from hardware button event on MCXC444
  else if (incomingData.indexOf("<V,1>") >= 0)
  {
    Serial.println("Manual Override Received! Watering...");
    openValve();
    isValveOpen = true;
    servoOpenTime = millis();
    sendTelegramMessage("Manual Watering Triggered!");
  }
}

// Execute HTTP GET and return server response body (or empty JSON on error)
String httpGETRequest(const char *serverName)
{
  WiFiClient client;
  HTTPClient http;

  http.begin(client, serverName);
  int httpResponseCode = http.GET();

  String payload = "{}";
  if (httpResponseCode > 0)
  {
    payload = http.getString();
  }
  else
  {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
  http.end();
  return payload;
}

// Push Telegram message via HTTPS Bot API
void sendTelegramMessage(String message)
{
  if (WiFi.status() == WL_CONNECTED)
  {
    WiFiClientSecure secureClient;
    secureClient.setInsecure();
    HTTPClient http;

    String url = "https://api.telegram.org/bot" + botToken + "/sendMessage";
    http.begin(secureClient, url);
    http.addHeader("Content-Type", "application/json"); // Specify content type as JSON

    String payload = "{\"chat_id\":\"" + chatId + "\",\"text\":\"" + message + "\"}";
    int httpResponseCode = http.POST(payload); // Send HTTP POST request

    if (httpResponseCode > 0)
    {
      Serial.println("Telegram Alert Sent Successfully!");
    }
    else
    {
      Serial.print("Telegram Error Code: ");
      Serial.println(httpResponseCode);
    }
    http.end();
  }
}

void readSensors()
{
  // Power sensors shortly before sampling to reduce idle consumption
  digitalWrite(SENSOR_POWER_PIN, HIGH);
  delay(200);

  // Take multiple samples to smooth sensor noise
  int waterSum = 0;
  int lightSum = 0;
  int numSamples = 20;

  for (int i = 0; i < numSamples; i++)
  {
    waterSum += analogRead(WATER_PIN);
    lightSum += analogRead(LDR_PIN);
    delay(50);
  }

  // Convert 13-bit ADC readings into 8-bit payload values
  rawWater = waterSum / numSamples;
  rawLight = lightSum / numSamples;

  lightLevel = (char)(((float)rawLight / 8191.0) * 255.0);
  waterLevel = (char)(((float)rawWater / 8191.0) * 255.0);

  digitalWrite(SENSOR_POWER_PIN, LOW);
}

// Rotate valve to the configured open angle
void openValve()
{
  valve.write(angle_open);
}

// Rotate valve back to the configured closed angle
void closeValve()
{
  valve.write(angle_closed);
}