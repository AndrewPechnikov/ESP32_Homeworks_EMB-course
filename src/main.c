#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/pulse_cnt.h"
#include "driver/gpio.h"
#include "esp_log.h"

static const char *TAG = "ENCODER";

#define ENCODER_PIN_CLK  14
#define ENCODER_PIN_DT   13
#define MAX_GLITCH_NS    1000

#define PCNT_HIGH_LIMIT 30000
#define PCNT_LOW_LIMIT  -30000

void app_main(void){
    ESP_LOGI(TAG, "Initializing pulse counter...");


    pcnt_unit_config_t unit_config = {
        .high_limit = PCNT_HIGH_LIMIT,
        .low_limit = PCNT_LOW_LIMIT,
    };
    pcnt_unit_handle_t pcnt_unit = NULL;
    ESP_ERROR_CHECK(pcnt_new_unit(&unit_config, &pcnt_unit));

    pcnt_glitch_filter_config_t filter_config = {
        .max_glitch_ns = MAX_GLITCH_NS,
    };
    ESP_ERROR_CHECK(pcnt_unit_set_glitch_filter(pcnt_unit, &filter_config));

    pcnt_chan_config_t chan_clk_config = {
        .edge_gpio_num = ENCODER_PIN_CLK,
        .level_gpio_num = ENCODER_PIN_DT,
    };
    pcnt_channel_handle_t pcnt_chan_clk = NULL;
    ESP_ERROR_CHECK(pcnt_new_channel(pcnt_unit, &chan_clk_config, &pcnt_chan_clk));

    pcnt_chan_config_t chan_dt_config = {
        .edge_gpio_num = ENCODER_PIN_DT,
        .level_gpio_num = ENCODER_PIN_CLK,
    };

    pcnt_channel_handle_t pcnt_chan_dt = NULL;
    ESP_ERROR_CHECK(pcnt_new_channel(pcnt_unit, &chan_dt_config, &pcnt_chan_dt));

    ESP_ERROR_CHECK(pcnt_channel_set_edge_action(pcnt_chan_clk, PCNT_CHANNEL_EDGE_ACTION_DECREASE, PCNT_CHANNEL_EDGE_ACTION_INCREASE));
    ESP_ERROR_CHECK(pcnt_channel_set_level_action(pcnt_chan_clk, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_INVERSE));

    ESP_ERROR_CHECK(pcnt_channel_set_edge_action(pcnt_chan_dt, PCNT_CHANNEL_EDGE_ACTION_INCREASE, PCNT_CHANNEL_EDGE_ACTION_DECREASE));
    ESP_ERROR_CHECK(pcnt_channel_set_level_action(pcnt_chan_dt, PCNT_CHANNEL_LEVEL_ACTION_KEEP, PCNT_CHANNEL_LEVEL_ACTION_INVERSE));

    ESP_ERROR_CHECK(pcnt_unit_enable(pcnt_unit));
    ESP_ERROR_CHECK(pcnt_unit_clear_count(pcnt_unit));
    ESP_ERROR_CHECK(pcnt_unit_start(pcnt_unit));

    ESP_LOGI(TAG, "Pulse counter initialized. Starting to count...");


    int pulse_count = 0;
    while (1) {
        ESP_ERROR_CHECK(pcnt_unit_get_count(pcnt_unit, &pulse_count));
        ESP_LOGI(TAG, "Raw count: %d | Position: %d", pulse_count, pulse_count / 4);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
    
}    