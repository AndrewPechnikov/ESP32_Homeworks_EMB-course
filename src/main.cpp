#include <Arduino.h>


#define LED_RED_PIN 16

#define LED_GREEN_PIN 4

#define FIRST_BUTTON_PIN 21
#define SECOND_BUTTON_PIN 0
int mode;


void fast_blinking(int LED1, int LED2){
  for(int i = 0; i < 4; i++){
    digitalWrite(LED1, 1);
    delay(100);
    digitalWrite(LED1, 0);
    delay(100);
    digitalWrite(LED2, 1);
    delay(100);
    digitalWrite(LED2, 0);
    delay(100);
  }


}

void blinking(int LED1, int LED2){

  digitalWrite(LED1, 1);
  delay(1000);
  digitalWrite(LED1, 0);
  digitalWrite(LED2, 1);
  delay(1000);
  digitalWrite(LED2, 0);
}

void alarm(int LED1, int LED2){

     digitalWrite(LED1, 1);
    digitalWrite(LED2, 1);
    delay(1000);
    digitalWrite(LED1, 0);
    digitalWrite(LED2, 0);
    delay(1000);
}

void setup() {

  pinMode(LED_RED_PIN, OUTPUT);
  pinMode(LED_GREEN_PIN, OUTPUT);

  pinMode(FIRST_BUTTON_PIN, INPUT);
  pinMode(FIRST_BUTTON_PIN, INPUT_PULLUP);


  
}



void loop() {

  bool firstButton = digitalRead(FIRST_BUTTON_PIN);
  bool secondButton = digitalRead(SECOND_BUTTON_PIN);

  

  mode = (firstButton) ? 1 : mode;
  mode = (!secondButton) ? 2 : mode;
  mode = (!secondButton && firstButton) ? 0 : mode;
  delay(1000);
  if (mode == 0){
    blinking(LED_RED_PIN, LED_GREEN_PIN);
  }
  else if (mode == 1)
  {
    fast_blinking(LED_RED_PIN, LED_GREEN_PIN);
  }

  else if (mode == 2)
  {
    
    alarm(LED_RED_PIN, LED_GREEN_PIN);
  }



  
  
 
  

  delay(50);

}

