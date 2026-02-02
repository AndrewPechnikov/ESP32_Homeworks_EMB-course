#include <Arduino.h>

#define LED_RED_PIN 16

#define LED_GREEN_PIN 4

#define FIRST_BUTTON_PIN 21
#define SECOND_BUTTON_PIN 0

#define SHORT_TIME_PAUSE 100
#define LONG_TIME_PAUSE 1000

#define DEBOUNCE_TIME 50

/*0 - повільне перемикання 
1 - швидке перемикання 
2 - одночасне перемикання*/
char mode = 0;

/*0 - не має закільцьованого перемикання 
1 - є закільцьоване перемикання з повернення на mode 0 при одночасному натисканні */
const bool SERIAL_MODE = 0;


void switchLedLogika(char mode);
void fast_blinking(int LED1, int LED2);
void blinking(int LED1, int LED2);
void alarm(int LED1, int LED2);


void setup()
{

  pinMode(LED_RED_PIN, OUTPUT);
  pinMode(LED_GREEN_PIN, OUTPUT);

  pinMode(FIRST_BUTTON_PIN, INPUT);
  pinMode(SECOND_BUTTON_PIN, INPUT_PULLUP);
}

void loop()
{

  bool firstButton = digitalRead(FIRST_BUTTON_PIN);
  bool secondButton = digitalRead(!SECOND_BUTTON_PIN);


  if(!SERIAL_MODE){

    mode = (firstButton) ? 1 : mode;
    mode = (secondButton) ? 2 : mode;
    mode = (secondButton && firstButton) ? 0 : mode;

    //delay(LONG_TIME_PAUSE);
    switchLedLogika(mode);

  }


  else{

    mode = (firstButton) ? ((mode + 1) % 3) : mode;
    mode = (secondButton) ? ((mode - 1 + 3) % 3) : mode;
    mode = (secondButton && firstButton) ? 0 : mode;
    switchLedLogika(mode);

  }


  delay(DEBOUNCE_TIME);
}

void switchLedLogika(char mode){
  if (mode == 0)
    {
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

}

void fast_blinking(int LED1, int LED2)
{
  for (int i = 0; i < 4; i++)
  {
    digitalWrite(LED1, HIGH);
    delay(SHORT_TIME_PAUSE);
    digitalWrite(LED1, LOW);
    delay(SHORT_TIME_PAUSE);
    digitalWrite(LED2, HIGH);
    delay(SHORT_TIME_PAUSE);
    digitalWrite(LED2, LOW);
    delay(SHORT_TIME_PAUSE);
  }
}

void alarm(int LED1, int LED2)
{

  digitalWrite(LED1, HIGH);
  digitalWrite(LED2, HIGH);
  delay(LONG_TIME_PAUSE);
  digitalWrite(LED1, LOW);
  digitalWrite(LED2, LOW);
  delay(LONG_TIME_PAUSE);
}

void blinking(int LED1, int LED2)
{

  digitalWrite(LED1, HIGH);
  delay(LONG_TIME_PAUSE);
  digitalWrite(LED1, LOW);
  digitalWrite(LED2, HIGH);
  delay(LONG_TIME_PAUSE);
  digitalWrite(LED2, LOW);
}