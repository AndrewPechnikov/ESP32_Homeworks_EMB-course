#include <Arduino.h>

// Конфігурація пінів
#define ADC_PIN          1 
#define LED_PWM_PIN      40
#define SERVO_PWM_PIN    10

// Налаштування LED
#define LED_CHANNEL      2
#define LED_FREQ         1000
#define LED_RES          12

// Налаштування Серво 
#define SERVO_CHANNEL    0
#define SERVO_FREQ       50
#define SERVO_RES        12

// Математичні константи для Серво (12 біт)
#define SERVO_MIN_DUTY   205  // 1 мс (0 градусів)
#define SERVO_MAX_DUTY   410  // 2 мс (180 градусів)
#define ADC_MAX_VALUE    4095 // 12 біт АЦП

void setup() {

  ledcSetup(SERVO_CHANNEL, SERVO_FREQ, SERVO_RES);
  ledcAttachPin(SERVO_PWM_PIN, SERVO_CHANNEL);

  ledcSetup(LED_CHANNEL, LED_FREQ, LED_RES);
  ledcAttachPin(LED_PWM_PIN, LED_CHANNEL);
}

void loop() {

  int raw = analogRead(ADC_PIN);
  

  long adjustedLED = (long)raw * raw / ADC_MAX_VALUE; 
  ledcWrite(LED_CHANNEL, (uint32_t)adjustedLED);


  int servoDuty = map(raw, 0, ADC_MAX_VALUE, SERVO_MIN_DUTY, SERVO_MAX_DUTY);
  ledcWrite(SERVO_CHANNEL, servoDuty);

  delay(15);
}