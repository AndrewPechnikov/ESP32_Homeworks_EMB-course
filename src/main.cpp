#include <Arduino.h>


#define LED_RED_PIN 1
#define LED_BLUE_PIN 2

void setup() {

  pinMode(LED_RED_PIN, OUTPUT);

}

void loop() {
  delay(1000);
  digitalWrite(LED_RED_PIN, 1);
  digitalWrite(LED_BLUE_PIN, 0);
  delay(1000);
  digitalWrite(LED_RED_PIN, 0);
  digitalWrite(LED_BLUE_PIN, 1);
}

