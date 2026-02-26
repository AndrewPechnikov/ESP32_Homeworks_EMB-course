#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_timer.h"

#define LED_PIN GPIO_NUM_2
#define BUTTON_PIN GPIO_NUM_3

void init (void);
void timer_callback(void* arg);


volatile bool button_pressed = false;
bool debounce_flag = true; // прапорець для реалізації програмного дебаунсу

const gpio_config_t io_config = {
    .intr_type = GPIO_INTR_NEGEDGE,             // перепивання по спадаючому фронту
    .mode =  GPIO_MODE_INPUT,                   // режим роботи на вхід
    .pin_bit_mask = (1ULL << BUTTON_PIN),       // масла для піну 4
    .pull_down_en = 0,                          // вимкнути внутрішню підтяжку до землі
    .pull_up_en = 1                             // увімкнута внутрішня підтяжка до живлення
};
const esp_timer_create_args_t timer_args = {
    .callback   = &timer_callback,
    .name       = "my_high_res_timer"
};



static volatile bool led_state = false;
esp_timer_handle_t my_timer;

static void IRAM_ATTR gpio_isr_handler(void *arg){
    button_pressed = true; 
}



    void app_main(void)
{
    init();


    while (1) {
        if (button_pressed && debounce_flag){
            led_state = !led_state; // змінюємо стан світлодіода
            gpio_set_level(LED_PIN, led_state);
            esp_timer_start_once(my_timer, 50000);
            debounce_flag = false; // вимикаємо дебаунс після натискання кнопки
        }
    }
}





void init(){

    
    esp_timer_create(&timer_args, &my_timer);

    gpio_reset_pin(LED_PIN);
    gpio_set_direction(LED_PIN , GPIO_MODE_OUTPUT);

    gpio_reset_pin(BUTTON_PIN);
    gpio_set_direction(BUTTON_PIN , GPIO_MODE_INPUT);
    gpio_config(&io_config);

        // встановити обробник переривання для кнопки
    gpio_install_isr_service(0);
    gpio_isr_handler_add(BUTTON_PIN, gpio_isr_handler, (void*) BUTTON_PIN);


}




void timer_callback(void* arg){
    button_pressed = false; // скидаємо прапорець натискання кнопки
    debounce_flag = true; // вмикаємо дебаунс після закінчення таймера
}