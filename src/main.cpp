#include <Arduino.h>

#define BUTTON_LEFT 15
#define BUTTON_RIGHT 3

volatile int16_t counter_left = 0;
volatile int16_t counter_right = 0;

volatile unsigned long lastPressTime = 0;
volatile bool buttonState = HIGH;    // поточний стабільний стан
volatile bool newEvent = false;      // сигнал про зміну стану кнопки

void IRAM_ATTR reaction_left() {
  counter_left++;
}

// ISR для правої кнопки
void IRAM_ATTR reaction_right() {
  unsigned long now = millis();
  bool current = digitalRead(BUTTON_RIGHT);
  counter_right++;
  // антидребезг: ігноруємо зміни раніше, ніж через 50 мс
  if (now - lastPressTime > 20) {
    if (current != buttonState) {
      buttonState = current;
      newEvent = true;
      lastPressTime = now;
    }
  }
}

void setup() {
  pinMode(BUTTON_LEFT, INPUT);
  pinMode(BUTTON_RIGHT, INPUT_PULLUP);
  Serial.begin(115200);

  attachInterrupt(digitalPinToInterrupt(BUTTON_LEFT), reaction_left, FALLING);
  attachInterrupt(digitalPinToInterrupt(BUTTON_RIGHT), reaction_right, CHANGE);
}

void loop() {
  if (newEvent) {
    noInterrupts();
    bool state = buttonState;
    int16_t copy_counter_right = counter_right;
    counter_right = 0;
    newEvent = false;
    interrupts();

    String position = (state == HIGH) ? "UNPRESS" : "PRESS";
    Serial.println("\nRIGHT Button " + position + "! Count: " + String(copy_counter_right));
    
  }
}
