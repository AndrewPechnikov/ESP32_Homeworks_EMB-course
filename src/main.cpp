#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <HTTPClient.h>
#include <Arduino_JSON.h>

WebServer server(80);
Preferences preferences;
String ssid = "";
String password = "";
String city = "";
String countryCode = "";
String openWeatherMapApiKey = "";
unsigned long lastTime = 95000;
unsigned long timerDelay = 100000; 

String getHTML() {
  String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<title>ESP32 Weather Config</title>";
  html += "<style>body{font-family:Arial,sans-serif; margin:20px; background:#f4f4f9;} ";
  html += "input[type='text'], input[type='password']{width:100%; padding:10px; margin:5px 0 15px; border:1px solid #ccc; border-radius:4px; box-sizing:border-box;} ";
  html += "input[type='submit']{background-color:#4CAF50; color:white; padding:12px 20px; border:none; border-radius:4px; cursor:pointer; width:100%;} ";
  html += "div.container{background:white; padding:20px; border-radius:8px; max-width:400px; margin:auto; box-shadow:0 4px 8px rgba(0,0,0,0.1);}</style></head><body>";
  
  html += "<div class='container'><h2>Налаштування ESP32</h2>";
  html += "<form action='/save' method='POST'>";
  
  html += "<label>Wi-Fi SSID:</label>";
  html += "<input type='text' name='ssid' value='" + ssid + "' required>";
  
  html += "<label>Wi-Fi Password:</label>";
  html += "<input type='password' name='password' value='" + password + "'>";
  
  html += "<label>City (Місто):</label>";
  html += "<input type='text' name='city' value='" + city + "' required>";
  
  html += "<label>Country Code (наприклад, PT, UA):</label>";
  html += "<input type='text' name='country' value='" + countryCode + "' required>";
  
  html += "<label>OpenWeather API Key:</label>";
  html += "<input type='text' name='apikey' value='" + openWeatherMapApiKey + "' required>";
  
  html += "<input type='submit' value='Зберегти та Перезавантажити'>";
  html += "</form></div></body></html>";
  
  return html;
}

void handleRoot() {
  server.send(200, "text/html", getHTML());
}

void handleSave() {
  if (server.method() == HTTP_POST) {
    ssid = server.arg("ssid");
    password = server.arg("password");
    city = server.arg("city");
    countryCode = server.arg("country");
    openWeatherMapApiKey = server.arg("apikey");
    preferences.putString("ssid", ssid);
    preferences.putString("password", password);
    preferences.putString("city", city);
    preferences.putString("country", countryCode);
    preferences.putString("apikey", openWeatherMapApiKey);
    String response = "<!DOCTYPE html><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'></head>";
    response += "<body style='font-family:Arial; text-align:center; margin-top:50px;'><h2>Налаштування збережено!</h2><p>ESP32 перезавантажується...</p></body></html>";
    server.send(200, "text/html", response);
    delay(1000);
    ESP.restart();
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

void setup() {
  Serial.begin(115200);
  preferences.begin("config", false);
  ssid = preferences.getString("ssid", "");
  password = preferences.getString("password", "");
  city = preferences.getString("city", "");
  countryCode = preferences.getString("country", "");
  openWeatherMapApiKey = preferences.getString("apikey", "");

  if (ssid == "") {
    Serial.println("SSID порожній. Запуск режиму Access Point (AP)...");
    WiFi.mode(WIFI_AP);
    WiFi.softAP("ESP32-Weather-Setup");
    Serial.print("AP IP Address: ");
    Serial.println(WiFi.softAPIP());
  } else {
    Serial.println("Спроба підключення до Wi-Fi: " + ssid);
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), password.c_str());

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 30) {
      delay(500);
      Serial.print(".");
      attempts++;
    }
    Serial.println("");

    if (WiFi.status() == WL_CONNECTED) {
      Serial.print("Підключено! Local IP: ");
      Serial.println(WiFi.localIP());
    } else {
      Serial.println("Не вдалося підключитися. Запуск режиму Access Point (AP)...");
      WiFi.mode(WIFI_AP);
      WiFi.softAP("ESP32-Weather-Setup");
      Serial.print("AP IP Address: ");
      Serial.println(WiFi.softAPIP());
    }
  }

  server.on("/", handleRoot);
  server.on("/save", handleSave);

  server.begin();
  Serial.println("Web server started!");
}

void loop() {
  server.handleClient();

  if (WiFi.status() == WL_CONNECTED) {
    if ((millis() - lastTime) > timerDelay) {
  
      if (city != "" && openWeatherMapApiKey != "") {
        String serverPath = "http://api.openweathermap.org/data/2.5/weather?q=" + city + "," + countryCode + "&APPID=" + openWeatherMapApiKey;
        
        String jsonBuffer = httpGETRequest(serverPath.c_str());
        Serial.println("--------------------");
        // Serial.println(jsonBuffer);
        
        JSONVar myObject = JSON.parse(jsonBuffer);
    
        if (JSON.typeof(myObject) == "undefined") {
          Serial.println("Parsing input failed!");
        } else if (myObject.hasOwnProperty("cod") && (int)myObject["cod"] != 200) {
           Serial.print("API Error: ");
           Serial.println((const char*)myObject["message"]);
        } else {
          Serial.print("Temperature (Celsius): ");
          Serial.println((double)myObject["main"]["temp"] - 273.15);
          Serial.print("Pressure (hPa): ");
          Serial.println(myObject["main"]["pressure"]);
          Serial.print("Humidity (%): ");
          Serial.println(myObject["main"]["humidity"]);
          Serial.print("Wind Speed (m/s): ");
          Serial.println(myObject["wind"]["speed"]);
        }
      } else {
         Serial.println("API Key або City не налаштовані. Зайдіть на веб-сторінку налаштувань.");
      }
      lastTime = millis();
    }
  }
}