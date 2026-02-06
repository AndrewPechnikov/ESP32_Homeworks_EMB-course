#include <Arduino.h>

#define ADC_PIN 18
#define REF_VOLTAGE 3.3
#define MAX_VALUE 4095

void setup() {
  Serial0.begin(115200);
  analogSetAttenuation(ADC_11db);
}

void loop() {
  int raw = analogRead(ADC_PIN);
  float voltage = raw * REF_VOLTAGE / MAX_VALUE;


  float calibrVoltage = analogReadMilliVolts(ADC_PIN);

  Serial0.println("Калібрована напруга:" + String(calibrVoltage));
  Serial0.println("Напруга: " + String(voltage));
  
  delay(500);
}
