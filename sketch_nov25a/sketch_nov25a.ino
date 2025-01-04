#include <ESP8266WiFi.h>
#include <UniversalTelegramBot.h>
#include <ArduinoJson.h>

// WiFi settings
#define WIFI_SSID ""
#define WIFI_PASSWORD ""
WiFiClientSecure secured_client;

// Telegram settings
#define BOT_TOKEN ""
#define CHAT_ID ""
UniversalTelegramBot bot(BOT_TOKEN, secured_client);

// OpenWeather settings
#define WEATHER_API_KEY "" // your waether api key
#define CITY_NAME "" // your desired city
#define WEATHER_URL "https://api.openweathermap.org/data/2.5/weather?q=" CITY_NAME "&appid=" WEATHER_API_KEY "&units=metric"

// Timing settings
const unsigned long BOT_MTBS = 3000; // mean time between scan messages
unsigned long bot_lasttime;

// Function to fetch weather data from OpenWeather
String getWeather() {
  if (WiFi.status() != WL_CONNECTED) {
    return "WiFi not connected!";
  }

  secured_client.setInsecure(); // Optional for HTTPS, avoid if you can load proper certificates
  if (!secured_client.connect("api.openweathermap.org", 443)) {
    return "Connection to OpenWeather failed!";
  }

  String url = WEATHER_URL;
  secured_client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                       "Host: api.openweathermap.org\r\n" +
                       "Connection: close\r\n\r\n");

  String response = "";
  while (secured_client.connected() || secured_client.available()) {
    response += secured_client.readString();
  }
  
  secured_client.stop();

  // Parse JSON response
  DynamicJsonDocument doc(2048);
  auto jsonStart = response.indexOf('{');
  if (jsonStart == -1) {
    return "Invalid weather data!";
  }

  DeserializationError error = deserializeJson(doc, response.substring(jsonStart));
  if (error) {
    return "Error parsing weather data!";
  }

  // Extract data
  float temp = doc["main"]["temp"];
  String weatherDescription = doc["weather"][0]["description"];
  float windSpeed = doc["wind"]["speed"];
  int windDeg = doc["wind"]["deg"];
  int humidity = doc["main"]["humidity"];

  // Convert wind degree to direction
  String windDirection;
  if (windDeg >= 337.5 || windDeg < 22.5) windDirection = "N";
  else if (windDeg < 67.5) windDirection = "NE";
  else if (windDeg < 112.5) windDirection = "E";
  else if (windDeg < 157.5) windDirection = "SE";
  else if (windDeg < 202.5) windDirection = "S";
  else if (windDeg < 247.5) windDirection = "SW";
  else if (windDeg < 292.5) windDirection = "W";
  else windDirection = "NW";

  // Formatted response
  String result = "Temperature: " + String(temp) + "°C\n";
  result += "Weather conditions: " + weatherDescription + "\n";
  result += "Humidity: " + String(humidity) + "%\n";
  result += "Wind speed and direction: " + String(windSpeed) + " м/c, " + windDirection;
  return result;
}

void setup() {
  Serial.begin(9600);
  configTime(0, 0, "pool.ntp.org");
  secured_client.setInsecure(); 
  
  Serial.print("Connecting to WiFi SSID ");
  Serial.print(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.print("\nWiFi connected. IP address: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  if (millis() - bot_lasttime > BOT_MTBS) {
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    while (numNewMessages) {
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
    bot_lasttime = millis();
  }
}

void handleNewMessages(int numNewMessages) {
  for (int i = 0; i < numNewMessages; i++) {
    if (bot.messages[i].chat_id == CHAT_ID) {
      String text = bot.messages[i].text;

      if (text == "/temp") {
        String weatherInfo = getWeather();
        bot.sendMessage(CHAT_ID, weatherInfo, "");
      }

      if (text == "/start" || text == "/help") {
        String welcome = "Hi, I'm ESP8266.\n";
        welcome += "/temp - The ability to get a current weather forecast, including temperature, humidity, wind speed and direction.\n";
        bot.sendMessage(CHAT_ID, welcome, "");
      }
    }
  }
}
