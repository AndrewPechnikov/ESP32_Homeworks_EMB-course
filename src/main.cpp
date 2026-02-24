#include <Arduino.h>

#define LED_PIN1 38
#define LED_PIN2 39
#define LED_PIN3 40


#define TIMER_INTERVAL_US 4
#define TIMER_PRESCALER_1 80


constexpr uint8_t POT_PIN       = 5;
volatile uint8_t counter       = 0;

volatile uint16_t blinkCounter1 = 0;
volatile uint16_t blinkCounter2 = 0;
volatile uint16_t blinkCounter3 = 0;

volatile uint16_t pwmUpdateCounter = 0;


hw_timer_t *timer = NULL;



class Potentiometer{
  public:
    enum class VoltageRange {
      Voltage_1v1 = ADC_0db,
      Voltage_1v5 = ADC_2_5db,
      Voltage_2v2 = ADC_6db,
      Voltage_3v3 = ADC_11db
    };

  private:
    uint8_t         _pin;
    uint8_t         _adcBit;
    VoltageRange    _range;
    uint32_t        _maxAdcValue;

  public:
    Potentiometer(
      uint8_t pin, 
      uint8_t adc_bit    = 12,
      VoltageRange range = VoltageRange::Voltage_3v3): 
      _pin(pin),  
      _adcBit(adc_bit),
      _range(range) 
      {

        _maxAdcValue = (1 << _adcBit) - 1;
      }

    void init(){
      analogReadResolution(_adcBit);
      analogSetPinAttenuation(_pin, (adc_attenuation_t)_range);
    }

    int getRaw(){
      return analogRead(_pin);
    }

    float getPercent(){
      return (getRaw() * 100.0f / _maxAdcValue);
    }

    uint8_t getMappedValue(){
        return (getRaw() * 255 / _maxAdcValue);
    }
};


Potentiometer duty_controler(POT_PIN);

void initLEDs()
{
    pinMode(LED_PIN1, OUTPUT);
    pinMode(LED_PIN2, OUTPUT);
    pinMode(LED_PIN3, OUTPUT);
}




void IRAM_ATTR onTimer()
{
    counter++;

    if (counter == 0)
    {
        blinkCounter1++;
        blinkCounter2++;
        blinkCounter3++;
        pwmUpdateCounter++;
    }
}

void PWMIteration(uint16_t currentCounter ,uint16_t dutyCycleLed ,uint8_t ledPin, bool blinkState){

    if(!blinkState){
        digitalWrite(ledPin, LOW);
        return;
    }



    if (currentCounter >= dutyCycleLed)
    {
        digitalWrite(ledPin, LOW);
    }
    else
    {
        digitalWrite(ledPin, HIGH);
    }

}


void blinkIteration(int timeMs, bool &blinkState, volatile uint16_t &blinkCounter){

        if (blinkCounter >= timeMs)
    {
      blinkCounter = 0;
      blinkState = !blinkState;
    }

}

void setup()
{
    Serial0.begin(115200);
    Serial0.println("Setup complete");

    timer = timerBegin(0, TIMER_PRESCALER_1, true);
    timerAttachInterrupt(timer, &onTimer, true);
    timerAlarmWrite(timer, TIMER_INTERVAL_US, true);
    timerAlarmEnable(timer);

    initLEDs();

    duty_controler.init();
}




void loop()
{
    uint8_t currentCounter = counter;
    static uint8_t activeDutyCycle = 255 - duty_controler.getMappedValue();
    

    static bool blinkState1 = true;
    static bool blinkState2 = true; 
    static bool blinkState3 = true;


    PWMIteration(currentCounter, activeDutyCycle, LED_PIN1, blinkState1);
    PWMIteration(currentCounter, activeDutyCycle, LED_PIN2, blinkState2);
    PWMIteration(currentCounter, activeDutyCycle, LED_PIN3, blinkState3);



    blinkIteration(100, blinkState1, blinkCounter1);
    blinkIteration(500, blinkState2, blinkCounter2);
    blinkIteration(1000, blinkState3, blinkCounter3); 

    
  


    if(pwmUpdateCounter >= 100){
        pwmUpdateCounter = 0;
        activeDutyCycle = 255 - duty_controler.getMappedValue();
    }

    
}