#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_timer.h"

#define ADC_PIN             ADC_CHANNEL_7
#define ADC_UNIT            ADC_UNIT_1
#define ADC_V_MIN           500    
#define ADC_V_MAX           3300   
#define ADC_RESOLUTION      4095   

#define GPIO_LED1           GPIO_NUM_37
#define LEDC_MODE           LEDC_LOW_SPEED_MODE
#define LEDC_TIMER          LEDC_TIMER_0
#define LEDC_DUTY_RES       LEDC_TIMER_12_BIT
#define LEDC_FREQUENCY      50     

#define SERVO_MIN_DUTY      102   
#define SERVO_MAX_DUTY      491   
#define SERVO_MAX_ANGLE     280   

#define LOG_INTERVAL_MS     500   
#define MAIN_LOOP_DELAY_MS  10     

adc_oneshot_unit_handle_t adc_handle;

void init_pwm() {
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


long map(long x, long in_min, long in_max, long out_min, long out_max) {
    if (x < in_min) return out_min;
    if (x > in_max) return out_max;
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void app_main() {
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

    int adc_raw;
    uint32_t last_log_time = (uint32_t)(esp_timer_get_time() / 1000);
    
    while (1) {
        ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, ADC_PIN, &adc_raw));

        int duty = map(adc_raw, ADC_V_MIN, ADC_V_MAX, SERVO_MIN_DUTY, SERVO_MAX_DUTY);
        int angle = map(adc_raw, 0, ADC_RESOLUTION, 0, SERVO_MAX_ANGLE);

        ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_0, duty);
        ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_0);

        uint32_t current_time_ms = (uint32_t)(esp_timer_get_time() / 1000);
        if (current_time_ms - last_log_time >= LOG_INTERVAL_MS) {
            ESP_LOGI("PWM", "ADC: %d | Angle: %d | Duty: %d", adc_raw, angle, duty);
            last_log_time = current_time_ms;
        }

        vTaskDelay(pdMS_TO_TICKS(MAIN_LOOP_DELAY_MS));
    }
}