#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_adc/adc_oneshot.h>
#include <esp_log.h>
#include <sdkconfig.h>
#include "driver/gpio.h"


#define ADC_PIN         ADC_CHANNEL_7
#define ADC_unit        ADC_UNIT_1
#define ADC_BITWIDTH    ADC_BITWIDTH_12
#define ADC_ATTEN       ADC_ATTEN_DB_12
#define WINDOW_SIZE     10
#define GPIO_LED        GPIO_NUM_36
#define ENABLE_LEVEL    2000
#define HYST            100






typedef struct {
    int     values[WINDOW_SIZE];  //Масив для зберігання останніх значень
    int     index;
    long    sum;
    bool    full;
} sma_filter_t;

void sma_init(sma_filter_t *f){
    f -> index = 0;
    f -> sum = 0;
    f -> full = false;
    for (int i = 0; i < WINDOW_SIZE; i++) f->values[i] = 0;
}


int b_add_value (sma_filter_t *f, int new_value){
    f->sum -= f->values[f->index];
    f->values[f->index] = new_value;
    f->sum += new_value;
    f->index = (f->index + 1) % WINDOW_SIZE;

    if (f->index == 0) f->full = true;

    return f->sum / (f->full ? WINDOW_SIZE : f->index);
}


void app_main() {
    sma_filter_t sma_filter;
    sma_init(&sma_filter);

    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << GPIO_LED),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&io_conf));





    int adc_value;
    adc_oneshot_unit_handle_t adc_handle;

    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_unit,
        .clk_src = ADC_RTC_CLK_SRC_DEFAULT,
    };

    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc_handle));


    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH,
        .atten = ADC_ATTEN,
    };

    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, ADC_PIN, &config));




    while (1){

        

        ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, ADC_PIN, &adc_value));

        int filtered_val = b_add_value(&sma_filter, adc_value);


        ESP_LOGI("ADC", "Raw: %d | Filtered: %d", adc_value, filtered_val);

        vTaskDelay(pdMS_TO_TICKS(100));

        if(filtered_val > ENABLE_LEVEL){
            gpio_set_level(GPIO_LED, 1);
        }

        else if(filtered_val < (ENABLE_LEVEL - HYST)){
            gpio_set_level(GPIO_LED, 0);
        }    
    }
}