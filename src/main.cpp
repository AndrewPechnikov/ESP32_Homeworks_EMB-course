#include <Arduino.h>


#define LED_RED_PIN 15
#define LED_BLUE_PIN 2
#define LED_GREEN_PIN 4

bool flagOnePIN = 0;
bool flagAdvanceBlink = 1;

void advanced_blinking(int LED1, int LED2, int LED3){
  for(int i = 0; i < 4; i++){
    digitalWrite(LED1, 1);
    delay(100);
    digitalWrite(LED1, 0);
    delay(100);
  }

   for(int i = 0; i < 4; i++){
    digitalWrite(LED3, 1);
    delay(100);
    digitalWrite(LED3, 0);
    delay(100);
  }

  for(int i = 0; i < 4; i++){
    digitalWrite(LED2, 1);
    delay(100);
    digitalWrite(LED2, 0);
    delay(100);
  }

  for(int i = 0; i < 4; i++){
    digitalWrite(LED3, 1);
    delay(100);
    digitalWrite(LED3, 0);
    delay(100);
  }

}

void blinking(int LED1, int LED2, int LED3){

  digitalWrite(LED1, 1);
  digitalWrite(LED2, 0);
  delay(1000);
  digitalWrite(LED1, 0);
  digitalWrite(LED2, 1);
  delay(1000);
  
}

void setup() {

  pinMode(LED_RED_PIN, OUTPUT);
  pinMode(LED_BLUE_PIN, OUTPUT);
  pinMode(LED_GREEN_PIN, OUTPUT);
}



void loop() {

  

  if(!flagOnePIN && flagAdvanceBlink){
    
    advanced_blinking(LED_BLUE_PIN, LED_RED_PIN, LED_GREEN_PIN);
  }
  else if (flagOnePIN){
    digitalWrite(LED_RED_PIN, 1);
    delay(1000);
    digitalWrite(LED_RED_PIN, 0);
    delay(1000);
  }
  else {
    blinking(LED_BLUE_PIN, LED_RED_PIN, LED_GREEN_PIN);
  }

}

