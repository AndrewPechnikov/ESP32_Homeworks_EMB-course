#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h" 

#define ENCODER_CLK_GPIO 5
#define ENCODER_DT_GPIO  4
#define ENCODER_SW_GPIO  1

static const char *TAG = "ENCODER";
static int64_t encoder_counter = 0;
static QueueHandle_t gpio_evt_queue = NULL;

// Обробник переривання (ISR) - виконується максимально швидко
static void IRAM_ATTR gpio_isr_handler(void* arg) {
    uint32_t gpio_num = (uint32_t) arg;
    
    static uint64_t last_clk_time = 0;
    static uint64_t last_sw_time = 0;
    
    uint64_t current_time = esp_timer_get_time();

    if (gpio_num == ENCODER_CLK_GPIO) {
        if ((current_time - last_clk_time) > 2000) {
            xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
            last_clk_time = current_time;
        }
    } 
    else if (gpio_num == ENCODER_SW_GPIO) {
        if ((current_time - last_sw_time) > 50000) {
            xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
            last_sw_time = current_time;
        }
    }
}

void encoder_task(void* arg) {
    uint32_t io_num;
    int last_clk_level = gpio_get_level(ENCODER_CLK_GPIO);
    
    while (1) {
        if (xQueueReceive(gpio_evt_queue, &io_num, portMAX_DELAY)) {
            if (io_num == ENCODER_CLK_GPIO) {
                int clk_level = gpio_get_level(ENCODER_CLK_GPIO);
                if (clk_level != last_clk_level) { // Перевірка на зміну стану (Edge)
                    int dt_level = gpio_get_level(ENCODER_DT_GPIO);
                    
                    if (clk_level != dt_level) {
                        encoder_counter++;
                    } else {
                        encoder_counter--;
                    }
                    last_clk_level = clk_level;
                    ESP_LOGI(TAG, "Count: %lld", encoder_counter);
                }
            } else if (io_num == ENCODER_SW_GPIO) {
                if (gpio_get_level(ENCODER_SW_GPIO) == 0) {
                    ESP_LOGW(TAG, "Button Pressed!");
                }
            }
        }
    }
}

void app_main(void) {
  
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_ANYEDGE,
        .pin_bit_mask = (1ULL << ENCODER_CLK_GPIO),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE
    };
    gpio_config(&io_conf);

    // DT пін просто як вхід
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.pin_bit_mask = (1ULL << ENCODER_DT_GPIO);
    gpio_config(&io_conf);

    // Кнопка SW
    io_conf.intr_type = GPIO_INTR_NEGEDGE; 
    io_conf.pin_bit_mask = (1ULL << ENCODER_SW_GPIO);
    gpio_config(&io_conf);

    // 2. Створення черги для обробки подій поза ISR
    gpio_evt_queue = xQueueCreate(10, sizeof(uint32_t));
    xTaskCreate(encoder_task, "encoder_task", 2048, NULL, 10, NULL);

    // 3. Встановлення сервісу переривань
    gpio_install_isr_service(0);
    gpio_isr_handler_add(ENCODER_CLK_GPIO, gpio_isr_handler, (void*) ENCODER_CLK_GPIO);
    gpio_isr_handler_add(ENCODER_SW_GPIO, gpio_isr_handler, (void*) ENCODER_SW_GPIO);
}