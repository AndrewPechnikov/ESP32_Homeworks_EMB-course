#include <Arduino.h>
#include <math.h>
#define ADC_PIN 8

#define ADC_DB ADC_11db //ADC_0db, ADC_2_5db, ADC_6db, ADC_11db)
#define REF_VOLTAGE 3.3 // 0.8, 1.1, 1.35, 3.3

#define BITS 12// 9, 10, 11, 12



const int maxValue = pow(2, BITS) - 1; 
void setup() {
  Serial0.begin(115200);
  analogSetAttenuation(ADC_DB);
  analogReadResolution(BITS);
}

void loop() {
  int raw = analogRead(ADC_PIN);
  float voltage = raw * REF_VOLTAGE / maxValue;


  float calibrVoltage = analogReadMilliVolts(ADC_PIN);

  Serial0.println("Калібрована напруга:" + String(calibrVoltage)+ "mV");
  Serial0.println("Напруга: " + String(voltage) + "V");
  int error = 100 - (voltage * 100 /(calibrVoltage/1000));
  Serial0.println("Похибка:  " + String(error) + "%");
  delay(1000);
}
