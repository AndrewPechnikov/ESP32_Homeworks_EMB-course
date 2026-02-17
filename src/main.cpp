#include <Arduino.h>


constexpr uint8_t LED_PIN         = 13;
constexpr uint8_t POT_PIN         = 34;
constexpr uint8_t BUTTON_PIN      = 14;
constexpr uint16_t WRITE_PAUSE    = 200;
constexpr uint16_t DEBOUNCE_TIME  = 200;


class ToggleButton{
  private:
    volatile bool       _state;
    uint8_t             _pin; 
    volatile uint32_t    _last_interrupt_time;


  public:
    ToggleButton(uint8_t pin,
                    bool state = false) : 
                    _pin(pin), 
                    _state(state){}


  void init(void (*callback)()){
    pinMode(_pin, INPUT);
    attachInterrupt(_pin, callback, FALLING);
  }

  void toggle(){

    unsigned long interrupt_time = millis();
    if(interrupt_time - _last_interrupt_time > DEBOUNCE_TIME){
      _state = !_state;
    }
    _last_interrupt_time = interrupt_time;
  }

  bool isActive(){
    return _state;
  }
};

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
    uint8_t         _adc_bit;
    VoltageRange    _range;
    uint32_t        _max_adc_value;

  public:
    Potentiometer(
      uint8_t pin, 
      uint8_t  adc_bit = 12,
      VoltageRange range = VoltageRange::Voltage_3v3): 
      _pin(pin),  
      _adc_bit(adc_bit),
      _range(range) 
      {

        _max_adc_value = (1 << _adc_bit) - 1;
      }

    void init(){
      analogReadResolution(_adc_bit);
      analogSetPinAttenuation(_pin, (adc_attenuation_t)_range);
    }

    int getRaw(){
      return analogRead(_pin);
    }

    float getPercent(){
      return (getRaw() * 100.0f / _max_adc_value);
    }
};

class PWM_generator{
  private:
    uint8_t   _pin;
    uint32_t  _freq;
    uint8_t   _resolution;
    uint8_t   _chanel;
    uint32_t  _max_duty;
    
  public:
    PWM_generator(
      uint8_t pin, 
      uint32_t freq = 5000, 
      uint8_t chanel = 0, 
      uint8_t resolution = 10): 
          _pin(pin), 
          _freq(freq),
          _resolution(resolution),
          _chanel(chanel)
          {
            _max_duty = (1 << _resolution) - 1;
          }

    void init(){
      ledcSetup(_chanel, _freq, _resolution);
      ledcAttachPin(_pin, _chanel);
    }

    void setDuty(uint32_t duty){
      ledcWrite(_chanel, duty);
    }

    void setPercentDuty(float percent){
      static constexpr float MAX_P = 100.0f;
      static constexpr float MIN_P = 0.0f;

      percent = (percent > MAX_P)? 100.0f : percent; 
      percent = (percent < MIN_P)? 0.0f : percent; 

      
      uint32_t duty = percent / 100.0f * _max_duty;
      setDuty(duty);

    }

    void stop_pwm(){
      setPercentDuty(0);
    }
};



Potentiometer duty_controler(POT_PIN);
PWM_generator led(LED_PIN);
ToggleButton button(BUTTON_PIN);

void IRAM_ATTR handleButton(){
  button.toggle();
}

void setup(){
  led.init();
  duty_controler.init();
  button.init(handleButton);


  Serial0.begin(115200);
  Serial0.printf("System Started: Dimmer Control\n");

}

void loop(){

  static unsigned long lastPrint = 0;
  float level = duty_controler.getPercent();
  
  if(button.isActive()){
    led.stop_pwm(); 
  }
  else{
    led.setPercentDuty(level);
  }
 

  if (millis() - lastPrint > WRITE_PAUSE){
    Serial0.printf("Level: %f \n", level);
    Serial0.printf("State button: %f \n", button.isActive());
    lastPrint = millis();
  }
  
  delay(10);
  
}