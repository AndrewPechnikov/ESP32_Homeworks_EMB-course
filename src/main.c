#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_timer.h"

#define ADC_PIN          ADC_CHANNEL_7
#define ADC_UNIT         ADC_UNIT_1
#define GPIO_LED1        GPIO_NUM_37


#define DEBOUNCE_TIME_MS 100

#define LEDC_MODE        LEDC_LOW_SPEED_MODE
#define LEDC_TIMER       LEDC_TIMER_0
#define LEDC_DUTY_RES    LEDC_TIMER_12_BIT
#define LEDC_FREQUENCY   5000

#define CURRENT_MELODY pirates_theme

typedef enum {
    NOTE_REST = 0,
    
    NOTE_C4, NOTE_CS4, NOTE_D4, NOTE_DS4, NOTE_E4, NOTE_F4, 
    NOTE_FS4, NOTE_G4, NOTE_GS4, NOTE_A4, NOTE_AS4, NOTE_B4,
   
    NOTE_C5, NOTE_CS5, NOTE_D5, NOTE_DS5, NOTE_E5, NOTE_F5, 
    NOTE_FS5, NOTE_G5, NOTE_GS5, NOTE_A5, NOTE_AS5, NOTE_B5
} note_name_t;

const uint32_t note_frequencies[] = {
    0,     // REST
    262, 277, 294, 311, 330, 349, 370, 392, 415, 440, 466, 494, // 4-та
    523, 554, 587, 622, 659, 698, 740, 784, 831, 880, 932, 988  // 5-та
};


typedef struct {
    note_name_t note;
    uint32_t duration_ms;
} melody_note_t;


const melody_note_t campfire_melody[] = {
    {NOTE_E4, 300}, {NOTE_A4, 300}, {NOTE_B4, 300}, {NOTE_C5, 600},
    {NOTE_B4, 300}, {NOTE_A4, 300}, {NOTE_G4, 300}, {NOTE_E4, 800},
    {NOTE_REST, 100},
    {NOTE_D4, 300}, {NOTE_F4, 300}, {NOTE_A4, 300}, {NOTE_G4, 600},
    {NOTE_F4, 300}, {NOTE_E4, 300}, {NOTE_D4, 300}, {NOTE_E4, 800},
    {NOTE_REST, 2000} 
};


const melody_note_t bandit_radio[] = {
    {NOTE_E4, 150}, {NOTE_E4, 150}, {NOTE_A4, 300}, 
    {NOTE_G4, 150}, {NOTE_G4, 150}, {NOTE_A4, 300},
    {NOTE_E4, 150}, {NOTE_E4, 150}, {NOTE_B4, 300}, 
    {NOTE_A4, 150}, {NOTE_A4, 150}, {NOTE_B4, 300},
    
    {NOTE_E4, 150}, {NOTE_E4, 150}, {NOTE_C5, 300}, 
    {NOTE_B4, 150}, {NOTE_B4, 150}, {NOTE_C5, 300},
    {NOTE_B4, 150}, {NOTE_A4, 150}, {NOTE_G4, 150}, {NOTE_FS4, 150},
    {NOTE_E4, 300}, {NOTE_REST, 300}
};

const melody_note_t happy_birthday[] = {
    {NOTE_G4, 200}, {NOTE_G4, 200}, {NOTE_A4, 400}, {NOTE_G4, 400}, 
    {NOTE_C5, 400}, {NOTE_B4, 800},
    
    {NOTE_G4, 200}, {NOTE_G4, 200}, {NOTE_A4, 400}, {NOTE_G4, 400}, 
    {NOTE_D5, 400}, {NOTE_C5, 800},
    
    {NOTE_G4, 200}, {NOTE_G4, 200}, {NOTE_G5, 400}, {NOTE_E5, 400}, 
    {NOTE_C5, 400}, {NOTE_B4, 400}, {NOTE_A4, 600},
    
    {NOTE_F5, 200}, {NOTE_F5, 200}, {NOTE_E5, 400}, {NOTE_C5, 400}, 
    {NOTE_D5, 400}, {NOTE_C5, 1000}
};


const melody_note_t ode_to_joy[] = {
    {NOTE_E4, 300}, {NOTE_E4, 300}, {NOTE_F4, 300}, {NOTE_G4, 300},
    {NOTE_G4, 300}, {NOTE_F4, 300}, {NOTE_E4, 300}, {NOTE_D4, 300},
    {NOTE_C4, 300}, {NOTE_C4, 300}, {NOTE_D4, 300}, {NOTE_E4, 300},
    {NOTE_E4, 450}, {NOTE_D4, 150}, {NOTE_D4, 600},
    
    {NOTE_E4, 300}, {NOTE_E4, 300}, {NOTE_F4, 300}, {NOTE_G4, 300},
    {NOTE_G4, 300}, {NOTE_F4, 300}, {NOTE_E4, 300}, {NOTE_D4, 300},
    {NOTE_C4, 300}, {NOTE_C4, 300}, {NOTE_D4, 300}, {NOTE_E4, 300},
    {NOTE_D4, 450}, {NOTE_C4, 150}, {NOTE_C4, 600}
};


const melody_note_t imperial_march[] = {
    {NOTE_A4, 400}, {NOTE_A4, 400}, {NOTE_A4, 400}, 
    {NOTE_F4, 300}, {NOTE_C5, 100}, {NOTE_A4, 400}, 
    {NOTE_F4, 300}, {NOTE_C5, 100}, {NOTE_A4, 800},
    
    {NOTE_E5, 400}, {NOTE_E5, 400}, {NOTE_E5, 400}, 
    {NOTE_F5, 300}, {NOTE_C5, 100}, {NOTE_GS4, 400}, 
    {NOTE_F4, 300}, {NOTE_C5, 100}, {NOTE_A4, 800}
};


const melody_note_t pirates_theme[] = {
    
    {NOTE_A4, 150}, {NOTE_C5, 150}, {NOTE_D5, 300}, {NOTE_D5, 300},
    {NOTE_D5, 150}, {NOTE_E5, 150}, {NOTE_F5, 300}, {NOTE_F5, 300},
    {NOTE_F5, 150}, {NOTE_G5, 150}, {NOTE_E5, 300}, {NOTE_E5, 300},
    {NOTE_D5, 150}, {NOTE_C5, 150}, {NOTE_D5, 450}, {NOTE_REST, 300},


    {NOTE_A4, 150}, {NOTE_C5, 150}, {NOTE_D5, 300}, {NOTE_D5, 300},
    {NOTE_D5, 150}, {NOTE_E5, 150}, {NOTE_F5, 300}, {NOTE_F5, 300},
    {NOTE_F5, 150}, {NOTE_G5, 150}, {NOTE_E5, 300}, {NOTE_E5, 300},
    {NOTE_D5, 150}, {NOTE_C5, 150}, {NOTE_D5, 450}, {NOTE_REST, 300},

    {NOTE_A4, 150}, {NOTE_C5, 150}, {NOTE_D5, 300}, {NOTE_D5, 300},
    {NOTE_D5, 150}, {NOTE_F5, 150}, {NOTE_G5, 300}, {NOTE_G5, 300},
    {NOTE_G5, 150}, {NOTE_A5, 150}, {NOTE_AS5, 300}, {NOTE_AS5, 300},
    {NOTE_A5, 150}, {NOTE_G5, 150}, {NOTE_A5, 300}, {NOTE_D5, 300},

  
    {NOTE_D5, 150}, {NOTE_E5, 150}, {NOTE_F5, 300}, {NOTE_F5, 300},
    {NOTE_F5, 150}, {NOTE_G5, 150}, {NOTE_E5, 300}, {NOTE_E5, 300},
    {NOTE_D5, 150}, {NOTE_C5, 150}, {NOTE_C5, 150}, {NOTE_D5, 600},
    {NOTE_REST, 1000}
};


const melody_note_t tetris_theme[] = {
    // Частина 1
    {NOTE_E5, 400}, {NOTE_B4, 200}, {NOTE_C5, 200}, {NOTE_D5, 400}, {NOTE_C5, 200}, {NOTE_B4, 200},
    {NOTE_A4, 400}, {NOTE_A4, 200}, {NOTE_C5, 200}, {NOTE_E5, 400}, {NOTE_D5, 200}, {NOTE_C5, 200},
    {NOTE_B4, 600}, {NOTE_C5, 200}, {NOTE_D5, 400}, {NOTE_E5, 400},
    {NOTE_C5, 400}, {NOTE_A4, 400}, {NOTE_A4, 600}, {NOTE_REST, 200},

    // Частина 2
    {NOTE_REST, 200}, {NOTE_D5, 600}, {NOTE_F5, 200}, {NOTE_A5, 400}, {NOTE_G5, 200}, {NOTE_F5, 200},
    {NOTE_E5, 600}, {NOTE_C5, 200}, {NOTE_E5, 400}, {NOTE_D5, 200}, {NOTE_C5, 200},
    {NOTE_B4, 400}, {NOTE_B4, 200}, {NOTE_C5, 200}, {NOTE_D5, 400}, {NOTE_E5, 400},
    {NOTE_C5, 400}, {NOTE_A4, 400}, {NOTE_A4, 600}, {NOTE_REST, 400},

    // Повторення Частини 1 
    {NOTE_E5, 400}, {NOTE_B4, 200}, {NOTE_C5, 200}, {NOTE_D5, 400}, {NOTE_C5, 200}, {NOTE_B4, 200},
    {NOTE_A4, 400}, {NOTE_A4, 200}, {NOTE_C5, 200}, {NOTE_E5, 400}, {NOTE_D5, 200}, {NOTE_C5, 200},
    {NOTE_B4, 600}, {NOTE_C5, 200}, {NOTE_D5, 400}, {NOTE_E5, 400},
    {NOTE_C5, 400}, {NOTE_A4, 400}, {NOTE_A4, 600}, {NOTE_REST, 1000}
};





adc_oneshot_unit_handle_t adc_handle;
void play_note(note_name_t note, uint32_t duration_ms, uint32_t pause_ms);

void init_pwm()
{
    ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_MODE,
        .duty_resolution = LEDC_DUTY_RES,
        .timer_num = LEDC_TIMER,
        .freq_hz = LEDC_FREQUENCY,
        .clk_cfg = LEDC_AUTO_CLK
    };
    ledc_timer_config(&ledc_timer);

    ledc_channel_config_t chan0 = {
        .speed_mode = LEDC_MODE,
        .channel = LEDC_CHANNEL_0,
        .timer_sel = LEDC_TIMER,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = GPIO_LED1,
        .duty = 0,
        .hpoint = 0
    };
    ledc_channel_config(&chan0);
}

void play_melody(const melody_note_t *melody, size_t length) {
    for (size_t i = 0; i < length; i++) {
        play_note(melody[i].note, melody[i].duration_ms, 50);
    }
}


void app_main()
{
    init_pwm();

    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT,
    };
    adc_oneshot_new_unit(&init_config, &adc_handle);

    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_12,
        .atten = ADC_ATTEN_DB_12,
    };
    adc_oneshot_config_channel(adc_handle, ADC_PIN, &config);
    
    
    while (1) {
    
    size_t melody_len = sizeof(CURRENT_MELODY) / sizeof(CURRENT_MELODY[0]);
            play_melody(CURRENT_MELODY, melody_len);
    }
}

void play_note(note_name_t note, uint32_t duration_ms, uint32_t pause_ms) {
    int adc_raw;
    adc_oneshot_read(adc_handle, ADC_PIN, &adc_raw);
    
    uint32_t volume = (uint32_t)adc_raw * adc_raw / (4095 * 2);

    if (note == NOTE_REST) {
        ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_0, 0);
        ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_0);
    } 
    else {
        ledc_set_freq(LEDC_MODE, LEDC_TIMER, note_frequencies[note]);
        ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_0, volume);
        ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_0);
    }

    vTaskDelay(pdMS_TO_TICKS(duration_ms));

    ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_0, 0);
    ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_0);
    
    if (pause_ms > 0) {
        vTaskDelay(pdMS_TO_TICKS(pause_ms));
    }
}