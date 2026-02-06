#include <Arduino.h>

#define ADC_PIN 18

#define ADC_DB ADC_0db //ADC_2_5db, ADC_6db, ADC_11db)
#define REF_VOLTAGE 0.8 // 1.1, 1.35, 3.3
#define MAX_VALUE 511 // 1023, 2047, 4095
#define BITS 9 // 10, 11, 12


void setup() {
  Serial0.begin(115200);
  analogSetAttenuation(ADC_DB);
  analogReadResolution(BITS);
}

void loop() {
  int raw = analogRead(ADC_PIN);
  float voltage = raw * REF_VOLTAGE / MAX_VALUE;


  float calibrVoltage = analogReadMilliVolts(ADC_PIN);

  Serial0.println("Калібрована напруга:" + String(calibrVoltage));
  Serial0.println("Напруга: " + String(voltage));
  int error = 100 - (voltage * 100 /(calibrVoltage/1000));
  Serial0.println("Похибка:  " + String(error) + "%");
  delay(1000);
}
