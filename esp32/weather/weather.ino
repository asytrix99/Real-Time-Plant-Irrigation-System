#include <WiFi.h>
#include <HTTPClient.h>
#include <Arduino_JSON.h>

const char* ssid = "Xiaomi 13 Pro"; 
const char* password = "1234567890"; 
String openWeatherMapApiKey = "11d9fd741957988badffa1134a497315"; 
String city = "Singapore";
String countryCode = "SG";
unsigned long lastTime = 0;
unsigned long timerDelay = 10000; // Api call delay : 10 seconds
const int NEW_TX_PIN = 1;
const int NEW_RX_PIN = 2;

void setup() {
  Serial.begin(115200); // Serial monitor
  Serial1.begin(115200, SERIAL_8N1, NEW_RX_PIN, NEW_TX_PIN); // UART
  Serial.print("Connecting to SSID: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  
  while(WiFi.status() != WL_CONNECTED) {
    delay(250);
    Serial.print(".");
  }
  
  Serial.println("\nConnected to WiFi!");
}

void loop() {
  if ((millis() - lastTime) > timerDelay) {
    if(WiFi.status() == WL_CONNECTED){
      String serverPath = "http://api.openweathermap.org/data/2.5/weather?q=" + city + "," + countryCode + "&APPID=" + openWeatherMapApiKey;
      String jsonBuffer = httpGETRequest(serverPath.c_str());
      
      JSONVar myObject = JSON.parse(jsonBuffer);
      if (JSON.typeof(myObject) == "undefined") {
        Serial.println("Parsing input failed!");
        return;
      }
      
      String weatherCondition = (const char*) myObject["weather"][0]["main"]; // Extract the "main" weather condition
      
      // Packetize data
      String packetToSend = "";
      if (weatherCondition == "Clear") {
        packetToSend = "<W,S>"; // Sunny
      } else if (weatherCondition == "Rain" || weatherCondition == "Thunderstorm" || weatherCondition == "Drizzle") {
        packetToSend = "<W,R>"; // Rainy
      } else {
        packetToSend = "<W,C>"; // Cloudy
      }
      
      Serial1.print(packetToSend); 
      Serial.print("Raw Weather Condition: ");
      Serial.println(weatherCondition);
      Serial.print("Sent to MCXC444: "); 
      Serial.println(packetToSend);
      Serial.println("-------------------------");
    } else {
      Serial.println("WiFi Disconnected");
    }
    lastTime = millis();
  }
}

String httpGETRequest(const char* serverName) {
  WiFiClient client;
  HTTPClient http;
  
  http.begin(client, serverName);
  int httpResponseCode = http.GET();
  
  String payload = "{}";
  if (httpResponseCode > 0) {
    payload = http.getString();
  } else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
  http.end();
  return payload;
}