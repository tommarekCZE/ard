#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

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


void renderTemp(int temp, int hum){
  oledDisplayCenter("Temperature:", 17);
  display.setTextSize(2);
  oledDisplayCenter(String(temp) + " C*", 30);
  display.setTextSize(0);
  oledDisplayCenter("Humidity: " + String(hum) + "%", 52);
}

void setup() {
  Serial.begin(115200);

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3D for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  delay(2000);
  display.clearDisplay();

  display.setTextSize(1);
  display.setTextColor(WHITE);
  //display.setCursor(0, 16);
  // Display static text
  oledDisplayCenter("Today is Friday", 2);
  renderTemp(41,20);
}

void loop() {
  
}