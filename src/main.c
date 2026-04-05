#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/pulse_cnt.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_log.h"

static const char *TAG = "ENCODER";


#define GPIO_LED1           GPIO_NUM_37
#define LEDC_MODE           LEDC_LOW_SPEED_MODE
#define LEDC_TIMER          LEDC_TIMER_0
#define LEDC_DUTY_RES       LEDC_TIMER_12_BIT
#define LEDC_FREQUENCY      50     
#define SERVO_MIN_DUTY      102   
#define SERVO_MAX_DUTY      491   
#define SERVO_MAX_ANGLE     280   
#define LOG_INTERVAL_MS     500   
#define MAIN_LOOP_DELAY_MS  5  
#define ENCODER_PIN_CLK     14
#define ENCODER_PIN_DT      13
#define ENCODER_PIN_SW      8
#define MAX_GLITCH_NS       200
#define PCNT_HIGH_LIMIT     30000
#define PCNT_LOW_LIMIT      -30000
#define DEBOUNCE_TIME       500
bool fast_mode_flag =       false;
static QueueHandle_t gpio_evt_queue = NULL;


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

static void IRAM_ATTR button_isr_handler(void* arg) {
    uint32_t gpio_num = (uint32_t)(uintptr_t) arg;
    xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
}

void app_main(void){
    init_pwm();
    ESP_LOGI(TAG, "Initializing pulse counter and button...");
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << ENCODER_PIN_SW),
        .mode = GPIO_MODE_INPUT,                  
        .pull_up_en = GPIO_PULLUP_ENABLE,         
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_NEGEDGE            
    };
    ESP_ERROR_CHECK(gpio_config(&io_conf));
    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
    ESP_ERROR_CHECK(gpio_install_isr_service(0));
    ESP_ERROR_CHECK(gpio_isr_handler_add(ENCODER_PIN_SW, button_isr_handler, (void*)(uintptr_t) ENCODER_PIN_SW));
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
    ESP_LOGI(TAG, "Initialization complete. Starting to count...");

  ESP_LOGI(TAG, "Initialization complete. Starting to count...");

    int software_pulse_count = 0; 
    int last_hw_count = 0;        
    uint32_t io_num;
    TickType_t last_log_time = xTaskGetTickCount();
    TickType_t last_button_press_time = 0;

    while (1) {
        int current_hw_count = 0;
        ESP_ERROR_CHECK(pcnt_unit_get_count(pcnt_unit, &current_hw_count));
        int delta = current_hw_count - last_hw_count;
        last_hw_count = current_hw_count;
        software_pulse_count += delta;
        int max_position_value = fast_mode_flag ? 10 : 25;
        int min_position_value = fast_mode_flag ? -9 : -24;
        int max_pulse_limit = max_position_value * 4;
        int min_pulse_limit = min_position_value * 4;
        if (software_pulse_count > max_pulse_limit) {
            software_pulse_count = max_pulse_limit;
        } else if (software_pulse_count < min_pulse_limit) {
            software_pulse_count = min_pulse_limit;
        }
        int position = software_pulse_count / 4;
        int duty = map(position, min_position_value, max_position_value, SERVO_MIN_DUTY, SERVO_MAX_DUTY);
        ledc_set_duty(LEDC_MODE, LEDC_CHANNEL_0, duty);
        ledc_update_duty(LEDC_MODE, LEDC_CHANNEL_0);
        if (xQueueReceive(gpio_evt_queue, &io_num, pdMS_TO_TICKS(10))) {
            TickType_t current_time = xTaskGetTickCount();
            if (current_time - last_button_press_time > pdMS_TO_TICKS(DEBOUNCE_TIME)) {
                fast_mode_flag = !fast_mode_flag;
                last_button_press_time = current_time;
            }
        }
        if ((xTaskGetTickCount() - last_log_time) >= pdMS_TO_TICKS(LOG_INTERVAL_MS)) { 
            ESP_LOGI(TAG, "Raw count: %d | Position: %d", software_pulse_count, position);
            ESP_LOGI(TAG, "Duty: %d | Flag: %d", duty, fast_mode_flag);
            last_log_time = xTaskGetTickCount();
        }
        vTaskDelay(pdMS_TO_TICKS(MAIN_LOOP_DELAY_MS));
    }
}