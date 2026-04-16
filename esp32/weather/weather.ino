#include <WiFi.h>
#include <HTTPClient.h>
#include <Arduino_JSON.h>
#include <WiFiClientSecure.h>
#include <time.h>

const char* ssid = "wifi"; 
const char* password = "password"; 
String openWeatherMapApiKey = "weather api"; 
String city = "Singapore";
String countryCode = "SG";
unsigned long lastTime = 0;
unsigned long timerDelay = 10000; // Api call delay : 10 seconds
const int NEW_TX_PIN = 1;
const int NEW_RX_PIN = 2;
String botToken = "telebot token";
String chatId = "ID";
const char* ntp = "pool.ntp.org";
const long offset = 8 * 3600; // 8h * 3600seconds

void setup() {
  Serial.begin(115200); // Serial monitor
  Serial1.begin(9600, SERIAL_8N1, NEW_RX_PIN, NEW_TX_PIN); // UART
  Serial.print("Connecting to SSID: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  
  while(WiFi.status() != WL_CONNECTED) {
    delay(250);
    Serial.print(".");
  }
  
  Serial.println("\nConnected to WiFi!");

  configTime(offset, 0, ntp);
  Serial.println("Time configured via NTP.");
  //sendTelegramMessage("⚠️ ALERT: Water tank is critically low! Please refill.");
}

void loop() {
  if ((millis() - lastTime) > timerDelay) {
    if (WiFi.status() == WL_CONNECTED){
      String serverPath = "http://api.openweathermap.org/data/2.5/weather?q=" + city + "," + countryCode + "&APPID=" + openWeatherMapApiKey;
      String jsonBuffer = httpGETRequest(serverPath.c_str());
      
      JSONVar myObject = JSON.parse(jsonBuffer);
      if(JSON.typeof(myObject) == "undefined") {
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
      
      Serial1.println(packetToSend); 
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

  if (Serial1.available()) {
    String incomingData = Serial1.readStringUntil('\n');
    Serial.print("Received from MCXC444: ");
    Serial.println(incomingData);

    if (incomingData.indexOf("<A,LOW>") >= 0) {
      sendTelegramMessage("ALERT: Water tank is critically low! Please refill.");
    }
    else if (incomingData.indexOf("<A,D>") >= 0) {
      struct tm timeinfo;
      if (!getLocalTime(&timeinfo)) {
        Serial.println("Failed to obtain time");
        sendTelegramMessage("Watering! (Failed to obtain time)");
      } else {
        char timeStr[50];
        strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", &timeinfo);
        String msg = "Watering!\n time: " + String(timeStr);
        sendTelegramMessage(msg);
      }
    }
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

void sendTelegramMessage(String message) {
  if(WiFi.status() == WL_CONNECTED){
    WiFiClientSecure secureClient;
    secureClient.setInsecure();
    HTTPClient http;
    
    String url = "https://api.telegram.org/bot" + botToken + "/sendMessage?chat_id=" + chatId + "&text=" + message;
    
    http.begin(secureClient, url);
    int httpResponseCode = http.GET();
    
    if (httpResponseCode > 0) {
      Serial.println("Telegram Alert Sent Successfully!");
    } else {
      Serial.print("Telegram Error Code: ");
      Serial.println(httpResponseCode);
    }
    http.end();
  }
}
