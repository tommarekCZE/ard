#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

#include <EEPROM.h>

#include <ArduinoJson.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

String apiKey = "a7b8a69f226946c8825152014240704";

bool setuped = false;

JsonDocument wdata;

String publicIP;

unsigned long lastTime = 0;
const long timerDelay = 10000;

const char* serverName = "http://api.weatherapi.com/v1/current.json";

String weekDays[7]={"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};

String currentDay = "NaN";

String reciveArr[2];

String command;

String weekday;

String jsonString;

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org");

void writeString(char add,String data)
{
  int _size = data.length();
  int i;
  for(i=0;i<_size;i++)
  {
    EEPROM.write(add+i,data[i]);
  }
  EEPROM.write(add+_size,'\0');
  EEPROM.commit();
}


String read_String(char add)
{
  int i;
  char data[100];
  int len=0;
  unsigned char k;
  k=EEPROM.read(add);
  while(k != '\0' && len<500)
  {    
    k=EEPROM.read(add+len);
    data[len]=k;
    len++;
  }
  data[len]='\0';
  return String(data);
}

String ssid;
String password;

int connectionAttempts = 0;
unsigned long previousMillis = 0;
const long interval = 1000;

void oledDisplayCenter(String text, int row) {
  int16_t x1;
  int16_t y1;
  uint16_t width;
  uint16_t height;

  display.getTextBounds(text, 0, 0, &x1, &y1, &width, &height);

  display.setCursor((SCREEN_WIDTH - width) / 2, row);
  display.println(text); // text to display
  display.display();
}

void renderLay() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  oledDisplayCenter("Today is " + weekday, 0);
}

String getPublicIP() {
  WiFiClient client;

  if(WiFi.status() != WL_CONNECTED){
    ESP.restart();
  }

  if (!client.connect("api.ipify.org", 80)) {
    Serial.println("Connection failed");
    ESP.restart();
  }

  // Send HTTP request
  client.print("GET / HTTP/1.1\r\n");
  client.print("Host: api.ipify.org\r\n");
  client.print("Connection: close\r\n\r\n");

  // Wait for response
  while (!client.available()) {
    delay(10);
  }

  // Read response
  String line;
  while (client.available()) {
    line = client.readStringUntil('\r');
  }
  
  // Find IP address in the response
  int pos = line.indexOf("\r\n\r\n") + 1;
  String ip = line.substring(pos);
  
  return ip;
}

void fetchWeather() {
  WiFiClient client;
  HTTPClient http;
    
  // Your IP address with path or Domain name with URL path 
  http.begin(client, serverName);
  
  // Send HTTP POST request
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  int httpResponseCode = http.POST("key=" + apiKey + "&q=" + publicIP);
  
  String payload = "{}"; 
  
  if (httpResponseCode>0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    payload = http.getString();
  }
  else {
    Serial.print("Error code: ");
    Serial.println(httpResponseCode);
  }
  // Free resources
  http.end();
  
  Serial.println(payload);

  DeserializationError error = deserializeJson(wdata, payload);

  if (error) {
    Serial.print("Deserialitazion failed: ");
    Serial.println(error.c_str());
    return;
  }
  Serial.print("Deserialitazion succeeded");  
}



void connectToWiFi() {
  Serial.println("Connecting to WiFi...");
  Serial.println("SSID: " + ssid + " / PASS: " + password);
  WiFi.begin(ssid.c_str(), password.c_str());
}

String* splitString(const String &input, char delimiter, int& size) {
    int partIndex = 0; // Index for parts array
    int startIndex = 0; // Start index for substring extraction

    // Count the number of parts
    for (int i = 0; i < input.length(); i++) {
        if (input.charAt(i) == delimiter) {
            size++;
        }
    }
    size++; // Account for the last part

    // Create an array of strings dynamically
    String *parts = new String[size];

    // Split the input string by the delimiter
    for (int i = 0; i < input.length(); i++) {
        if (input.charAt(i) == delimiter) {
            parts[partIndex++] = input.substring(startIndex, i);
            startIndex = i + 1;
        }
    }
    parts[partIndex] = input.substring(startIndex); // Store the last part

    return parts;
}

void handleWiFiConnection() {
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    Serial.print(".");
    connectionAttempts++;
    if (connectionAttempts >= 10) {
      connectionAttempts = 0;
      oledDisplayCenter("Failed to connect!", 30);
      oledDisplayCenter("Setup via USB", 40);
      blinkLED();
    }
  }
}

void blinkLED() {
  static bool ledState = false;
  digitalWrite(LED_BUILTIN, ledState ? HIGH : LOW);
  ledState = !ledState;
}

void handleSerialCommands() {
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil(' ');
    if (command == "!set") {
      String input = Serial.readStringUntil('\n');
      int commaIndex = input.indexOf(',');
      if (commaIndex != -1) {
        int a = 2;
        int& lref = a;
        String *values = splitString(input, ',', a);
        ssid = values[0];
        password = values[1];
        writeString(10, ssid);
        writeString(50, password);
        Serial.println("WiFi credentials set! Restarting ESP8266...");
        delay(1000);
        ESP.restart();
      } else {
        Serial.println("Invalid format. Please enter SSID and password separated by a comma.");
      }
    }
  }
}

void setup() {
  Serial.begin(74880);
  EEPROM.begin(512);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH);
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  display.clearDisplay();
  display.setCursor(0, 0);
  display.display();
  delay(2000);

  display.setTextSize(1);
  display.setTextColor(WHITE);
  oledDisplayCenter("Connecting...", 0);

  Serial.println("SSID: " + read_String(10) + " / PASS: " + read_String(50));
  ssid = read_String(10);
  password = read_String(50);
  //timeClient.begin();
  //timeClient.setTimeOffset(3600);
  Serial.println("Setup Completed!");
  connectToWiFi();
}

void loop() {
  while (setuped==false) {
    handleSerialCommands();
      if (WiFi.status() != WL_CONNECTED) {
          handleWiFiConnection();
      } else {
        setuped=true;
        Serial.println("\nWiFi Connected!");
        display.clearDisplay();
        oledDisplayCenter("Loading...", 0);
        timeClient.begin();
        timeClient.setTimeOffset(3600);
        timeClient.update();
        weekday = weekDays[timeClient.getDay()];
        Serial.println(weekday);
        publicIP = getPublicIP();
        Serial.println("Public IP: " + publicIP);
        //fetchWeather();
        renderLay();
        break;
      }
  }


  if ((millis() - lastTime) > timerDelay) {
    //Check WiFi connection status
    if(WiFi.status()== WL_CONNECTED){
      //timeClient.update();
      //String weekDay = weekDays[timeClient.getDay()];
      //Serial.println(weekDay);
      //jsonString = fetchWeather();
      //Serial.println(jsonString);
    }
    else {
      Serial.println("WiFi Disconnected");
      ESP.restart();
    }
    lastTime = millis();
  }
}
/*
  if ((millis() - lastTime) > timerDelay) {
    //Check WiFi connection status
    if(WiFi.status()== WL_CONNECTED){
              
      //jsonString = fetchWeather();
      //Serial.println(jsonString);
    }
    else {
      Serial.println("WiFi Disconnected");
    }
    lastTime = millis();
  }
  */