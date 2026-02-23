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
};

class PWM_generator{
  private:
    uint8_t   _pin;
    uint32_t  _freq;
    uint8_t   _resolution;
    uint8_t   _chanel;
    uint32_t  _maxDuty;
    
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
            _maxDuty = (1 << _resolution) - 1;
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

      
      uint32_t duty = percent / 100.0f * _maxDuty;
      setDuty(duty);

    }

    void stop_pwm(){
      setPercentDuty(0);
    }
};

class Soft_PWM_generator{
  private:
    uint8_t         _pin;
    float           _percent;
    unsigned long   _previousTime = 0;
    unsigned long   _onTime;
    unsigned long   _offTime;
    unsigned long   _period;
    bool            _pinState = false;
    
  public:
    Soft_PWM_generator(uint8_t pin, uint16_t freq, uint16_t percent = 50):
    _pin(pin), _period(1000000UL / freq), _percent(percent) {
        _onTime = (_period * _percent) / 100;
        _offTime = _period - _onTime;


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
        if((current_time - _previousTime) >= _onTime){
          digitalWrite(_pin, LOW);
           setPinState(false); 
          _previousTime = current_time;
        }
      }
      else if((current_time - _previousTime) >= _offTime){
          digitalWrite(_pin, HIGH);
          setPinState(true); 
          _previousTime = current_time;
      }
    }
    
    void setPercentDuty(float percent){
      _percent = percent;
      static constexpr float MAX_P = 100.0f;
      static constexpr float MIN_P = 0.0f;

      _percent = (_percent > MAX_P)? 100.0f : _percent; 
      _percent = (_percent < MIN_P)? 0.0f : _percent; 

      _onTime = (_period * _percent) / 100;
      _offTime = _period - _onTime;

    }

    void stop_pwm(){
      setPercentDuty(0);
    }


};

class RelayControlAndMeasurement{
  private:
    uint8_t _pinCoil;
    uint8_t _pinContact;
    bool    _relayState                    = false;
    volatile bool    _measureDone           = false;
    volatile unsigned long _commandOnTime  = 0;
    volatile unsigned long _resultTime     = 0;

  public:
    RelayControlAndMeasurement(uint8_t pinCoil, uint8_t pinContact):
                                  _pinCoil(pinCoil), _pinContact(pinContact)
                                  {}


  void init(void(*callback)()){
    pinMode(_pinCoil, OUTPUT);
    digitalWrite(_pinCoil, HIGH); // OFF Relay
    pinMode(_pinContact, INPUT_PULLDOWN);
    attachInterrupt(_pinContact, callback, RISING);
  }

  bool getRelayState(){
    return _relayState;
  }



  void turnOn(){
    _measureDone = false;
    _resultTime= 0;
    _commandOnTime = micros();
    digitalWrite(_pinCoil, LOW); 
    _relayState = true;
  }

  void turnOff(){
    
    digitalWrite(_pinCoil, HIGH); 
    _relayState = false;
  }

  void IRAM_ATTR measureWorkTime(){
    if(!_measureDone && _relayState){
         _resultTime= micros() - _commandOnTime;
         _measureDone = true;
    }
 

  }

  unsigned long getResultTime(){
    return _resultTime;
  }






};

Potentiometer duty_controler(POT_PIN);
Soft_PWM_generator motor(MOTOR_PIN, FREQUENCY_PWM);
RelayControlAndMeasurement relay(RELAY_COIL_PIN, RELAY_CONTACT_PIN);

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
  static unsigned long lastAdcTime = 0;
  static unsigned long lastRelayTime = 0;
  static uint8_t counterMeasure = 0;
  static uint32_t sumOfMeasure = 0;
  static bool endMeasure = false;
  motor.dutyControl();
  if (millis() - lastAdcTime > UPDATE_PWM_TIME) {
    motor.setPercentDuty(duty_controler.getPercent());
    lastAdcTime = millis();
  }

  if (millis() - lastRelayTime > RELAY_TOGGLE_TIME && !endMeasure) {
    lastRelayTime = millis();
    if(relay.getRelayState()){
      relay.turnOff();
      sumOfMeasure += relay.getResultTime();
      counterMeasure++;
      /*
      Serial0.print("Relay trigger time (micros): ");
      Serial0.print(relay.getResultTime());
      Serial0.println(" mks.");
      */
    }
    else{
      relay.turnOn();
    }
  }

  if (counterMeasure >= NUM_RELAY_TOGGLE){
    uint32_t averageMeasureTime = sumOfMeasure / NUM_RELAY_TOGGLE;
    Serial0.print("average work relay (micros): ");
    Serial0.print(averageMeasureTime);
    Serial0.println(" mks.");
    counterMeasure = 0;
    sumOfMeasure = 0;
    endMeasure = true;
  }
  



}