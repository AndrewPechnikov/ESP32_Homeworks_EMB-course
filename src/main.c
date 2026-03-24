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
#define GPIO_LED1        GPIO_NUM_45
#define GPIO_LED2        GPIO_NUM_37
#define GPIO_BUTTON      GPIO_NUM_5

#define DEBOUNCE_TIME_MS 100


#define LEDC_MODE        LEDC_LOW_SPEED_MODE
#define LEDC_TIMER       LEDC_TIMER_0
#define LEDC_DUTY_RES    LEDC_TIMER_12_BIT
#define LEDC_FREQUENCY   5000

volatile static int64_t last_interrupt_time = 0;
volatile bool isLed = true;

static void IRAM_ATTR gpio_isr_handler(void *arg)
{
    int64_t current_time = esp_timer_get_time() / 1000;
    if (current_time - last_interrupt_time > DEBOUNCE_TIME_MS)
    {
        isLed = !isLed;
        last_interrupt_time = current_time;
    }
}

void init_dual_pwm()
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

    ledc_channel_config_t chan1 = {
        .speed_mode = LEDC_MODE,
        .channel = LEDC_CHANNEL_1,
        .timer_sel = LEDC_TIMER,
        .gpio_num = GPIO_LED2,
        .duty = 0,
        .hpoint = 0
    };
    ledc_channel_config(&chan1);
}

void app_main()
{
    init_dual_pwm();

    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << GPIO_BUTTON),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .intr_type = GPIO_INTR_NEGEDGE,
    };
    gpio_config(&io_conf);

    gpio_install_isr_service(0);
    gpio_isr_handler_add(GPIO_BUTTON, gpio_isr_handler, (void *)GPIO_BUTTON);

    adc_oneshot_unit_handle_t adc_handle;
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
    
    while (1)
    {
        ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, ADC_PIN, &adc_raw));
        uint32_t duty = adc_raw;
        
        if (isLed == 0)
        {
            ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_0, duty);
            ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_0);
            ESP_LOGI("PWM", "Focus: LED 1, Duty: %lu", duty);
        }
        else
        {
            ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_1, duty);
            ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_1);
            ESP_LOGI("PWM", "Focus: LED 2, Duty: %lu", duty);
        }
        
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
