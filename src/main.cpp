#include <Arduino.h>

#define LED_PIN1 13
#define LED_PIN2 12
#define LED_PIN3 11

#define TIMER_INTERVAL_US 4
#define TIMER_PRESCALER 80



volatile uint8_t counter = 0;
volatile uint8_t dutyCycleLed1 = 128;
volatile uint8_t dutyCycleLed2 = 50;
volatile uint8_t dutyCycleLed3 = 200;

hw_timer_t * timer = NULL;

void initLEDs() {
  pinMode(LED_PIN1, OUTPUT);
  pinMode(LED_PIN2, OUTPUT);
  pinMode(LED_PIN3, OUTPUT);
}

void IRAM_ATTR onTimer() {
    counter++;
}



void setup() {
  Serial0.begin(115200);
  Serial0.println("Setup complete");

  timer = timerBegin(0, TIMER_PRESCALER, true);
  
  timerAttachInterrupt(timer, &onTimer, true);

  timerAlarmWrite(timer, TIMER_INTERVAL_US, true);

  timerAlarmEnable(timer);

  initLEDs();
}

void loop() {
    uint8_t currentCounter = counter;


    if (currentCounter > dutyCycleLed1){
        digitalWrite(LED_PIN1, LOW);
    }
    else{
        digitalWrite(LED_PIN1, HIGH);
    }

    if (currentCounter > dutyCycleLed2){
        digitalWrite(LED_PIN2, LOW);
    }
    else{
        digitalWrite(LED_PIN2, HIGH);
    }

    if (currentCounter > dutyCycleLed3){
        digitalWrite(LED_PIN3, LOW);
    }
    else{
        digitalWrite(LED_PIN3, HIGH);
    }


}