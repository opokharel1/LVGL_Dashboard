#include <Arduino.h>
#include <TFT_eSPI.h>   // Include the graphics library
// #include <../TFT_eSPI/User_Setup.h>

TFT_eSPI tft = TFT_eSPI();  // Invoke library

void setup() {
  Serial.begin(115200);
  Serial.println("TFT Test Starting...");

  tft.init();
  tft.setRotation(1);   // 0-3 depending on display orientation

  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.setTextSize(2);

  tft.setCursor(40, 60);
  tft.println("Hello TFT!");
  delay(2000);
}

void loop() {
  // Simple color cycle demo
  uint16_t colors[] = {TFT_RED, TFT_GREEN, TFT_BLUE, TFT_YELLOW, TFT_CYAN, TFT_MAGENTA};
  for (int i = 0; i < 6; i++) {
    tft.fillScreen(colors[i]);
    delay(1000);
  }
}
