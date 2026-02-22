#include <Arduino.h>


constexpr uint8_t MOTOR_PIN           = 40;
constexpr uint8_t POT_PIN             = 5;
constexpr uint8_t BUTTON_PIN          = 14;
constexpr uint8_t RELAY_COIL_PIN      = 39;
constexpr uint8_t RELAY_CONTACT_PIN   = 38;

constexpr uint16_t WRITE_PAUSE          = 200;
constexpr uint16_t DEBOUNCE_TIME        = 200;
constexpr uint16_t FREQUENCY_PWM        = 5000;
constexpr uint16_t UPDATE_PWM_TIME      = 50;

constexpr uint16_t RELAY_TOGGLE_TIME    = 200;
constexpr uint16_t NUM_RELAY_TOGGLE    = 20;

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

class Soft_PWM_generator{
  private:
    uint8_t         _pin;
    uint16_t        _percent;
    unsigned long   _previous_time = 0;
    unsigned long   _on_time;
    unsigned long   _off_time;
    unsigned long   _period;
    bool            _pinState = false;
    
  public:
    Soft_PWM_generator(uint8_t pin, uint16_t freq, uint16_t persent = 50):
    _pin(pin), _period(1000000UL / freq), _percent(persent) {
        _on_time = (_period * _percent) / 100;
        _off_time = _period - _on_time;


    }

    void init(){
      pinMode(_pin, OUTPUT);
      digitalWrite(_pin, LOW);

      Serial0.println("PWM init!");
    }

    bool getPinState(){
      return _pinState;
    }

    void setPinState(bool state){
      _pinState = state;

    }

    void dutyControl(){
      uint32_t  current_time =  micros(); 

      if(_percent == 100){
        if(!_pinState){
          digitalWrite(_pin,HIGH);
          setPinState(true);
        }
        return;
      }

      if(_percent == 0){
        if(_pinState){
          digitalWrite(_pin, LOW);
          setPinState(false);
        }
        return;
      }



      if(getPinState()){
        if((current_time - _previous_time) >= _on_time){
          digitalWrite(_pin, LOW);
           setPinState(false); 
          _previous_time = current_time;
        }
      }
      else if((current_time - _previous_time) >= _off_time){
          digitalWrite(_pin, HIGH);
          setPinState(true); 
          _previous_time = current_time;
      }
    }
    
    void setPercentDuty(float percent){
      _percent = percent;
      static constexpr float MAX_P = 100.0f;
      static constexpr float MIN_P = 0.0f;

      _percent = (_percent > MAX_P)? 100.0f : _percent; 
      _percent = (_percent < MIN_P)? 0.0f : _percent; 

      _on_time = (_period * _percent) / 100;
      _off_time = _period - _on_time;

    }

    void stop_pwm(){
      setPercentDuty(0);
    }


};

class Relay_control_and_measurement{
  private:
    uint8_t _pin_coil;
    uint8_t _pin_contact;
    bool    _relay_state                     = false;
    volatile bool    _measure_done           = false;
    volatile unsigned long _command_on_time  = 0;
    volatile unsigned long _result_time      = 0;

  public:
    Relay_control_and_measurement(uint8_t pin_coil, uint8_t pin_contact):
                                  _pin_coil(pin_coil), _pin_contact(pin_contact)
                                  {}


  void init(void(*callback)()){
    pinMode(_pin_coil, OUTPUT);
    digitalWrite(_pin_coil, HIGH); // OFF Relay
    pinMode(_pin_contact, INPUT_PULLDOWN);
    attachInterrupt(_pin_contact, callback, RISING);
  }

  bool getRelayState(){
    return _relay_state;
  }



  void turnOn(){
    _measure_done = false;
    _result_time = 0;
    _command_on_time = micros();
    digitalWrite(_pin_coil, LOW); 
    _relay_state = true;
  }

  void turnOff(){
    
    digitalWrite(_pin_coil, HIGH); 
    _relay_state = false;
  }

  void IRAM_ATTR measureWorkTime(){
    if(!_measure_done && _relay_state){
         _result_time = micros() - _command_on_time;
         _measure_done = true;
    }
 

  }

  unsigned long getResultTime(){
    return _result_time;
  }






};

Potentiometer duty_controler(POT_PIN);
Soft_PWM_generator motor(MOTOR_PIN, FREQUENCY_PWM);
Relay_control_and_measurement relay(RELAY_COIL_PIN, RELAY_CONTACT_PIN);

void IRAM_ATTR handleRelay(){
  relay.measureWorkTime();


}


void setup(){
  Serial0.begin(115200);
  Serial0.printf("System Started: Dimmer Control\n");

  motor.init();
  duty_controler.init();
  relay.init(handleRelay);
}

void loop(){
  static unsigned long last_adc_time = 0;
  static unsigned long last_relay_time = 0;
  static uint8_t counterMeasure = 0;
  static uint32_t sumOfMeasure = 0;
  static bool endMeasure = false;
  motor.dutyControl();
  if (millis() - last_adc_time > UPDATE_PWM_TIME) {
    motor.setPercentDuty(duty_controler.getPercent());
    last_adc_time = millis();
  }

  if (millis() - last_relay_time > RELAY_TOGGLE_TIME && !endMeasure) {
    last_relay_time = millis();
    if(relay.getRelayState()){
      relay.turnOff();
      sumOfMeasure += relay.getResultTime();
      counterMeasure++;
      /*
      Serial0.print("Relay trigger time (micros): ");
      Serial0.print(relay.getResultTime());
      Serial0.println(" ms.");
      */
    }
    else{
      relay.turnOn();
    }
  }

  if (counterMeasure >= NUM_RELAY_TOGGLE){
    uint32_t avarageMeasureTime = sumOfMeasure / NUM_RELAY_TOGGLE;
    Serial0.print("Avarage work relay (micros): ");
    Serial0.print(avarageMeasureTime);
    Serial0.println(" ms.");
    counterMeasure = 0;
    sumOfMeasure = 0;
    endMeasure = true;
  }
  



}