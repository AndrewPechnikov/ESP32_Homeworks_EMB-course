#include <Arduino.h>
#include <math.h>
#define ADC_PIN 8

#define LED_PIN 40

#define ADC_DB ADC_11db 
#define REF_VOLTAGE 3.3 
#define BITS 12



const float maxValue = pow(2, BITS) - 1; 
void setup() {
  Serial0.begin(115200);
  analogSetAttenuation(ADC_DB);
  analogReadResolution(BITS);

  pinMode(LED_PIN, OUTPUT);
}

void loop() {
  int raw = analogRead(ADC_PIN);
  float voltage = raw * REF_VOLTAGE / maxValue;


  float PWM = raw / maxValue * 255;

  analogWrite(LED_PIN, PWM);

  
  //Serial0.println("Напруга: " + String(PWM) + "%");
  delay(1);
  
}
