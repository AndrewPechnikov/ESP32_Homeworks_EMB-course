#include <Arduino.h>

#define BUTTON_RIGHT 9

volatile int16_t counter_right = 0;
volatile unsigned long firstImpulseTime = 0;
volatile unsigned long lastImpulseTime = 0;

void reaction_right() {
    unsigned long time = micros();
    if (counter_right == 0) {
        firstImpulseTime = time;
    }
    lastImpulseTime = time;
    counter_right++;
}

void setup() {
    pinMode(BUTTON_RIGHT, INPUT_PULLUP);
    Serial0.begin(115200);
    attachInterrupt(digitalPinToInterrupt(BUTTON_RIGHT), reaction_right, FALLING);
}

void loop() {
    int16_t count;
    unsigned long startTime, endTime;

    if (counter_right > 0 && (micros() - lastImpulseTime > 50000)) {

        noInterrupts();   
        count = counter_right;
        startTime = firstImpulseTime;
        endTime = lastImpulseTime;

        counter_right = 0;   
        interrupts();    

        Serial0.print("Кількість імпульсів (дребезг): ");
        Serial0.println(count);
        Serial0.print("Тривалість дребезгу (мкс): ");
        Serial0.println(endTime - startTime);
        Serial0.println("---");
    }
}