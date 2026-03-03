#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_log.h"

#define LED_PIN GPIO_NUM_36
#define BUTTON_PIN GPIO_NUM_1
#define DEBOUNCE_DELAY_MS 50
#define POLLING_RATE_MS 10  

static const char *TAG = "MY_APP";

typedef enum {
    STATE_IDLE,
    STATE_DEBOUNCE_PRESS,
    STATE_PRESSED,
    STATE_DEBOUNCE_RELEASE
} button_state_t;

void timer_callback(void* arg);

const esp_timer_create_args_t timer_args = {
    .callback   = &timer_callback,
    .name       = "my_high_res_timer"
};

volatile bool button_pressed = false;
bool debounce_flag = true;
volatile int counter = 0;
static volatile bool led_state = false;


button_state_t btn_state = STATE_IDLE;
uint32_t state_timer = 0; 

esp_timer_handle_t my_timer;


void init(void);
void without_debounce(void);
void soft_debounce(void);
void soft_state_debounce(void);
void button_FSM(void);


void IRAM_ATTR gpio_isr_handler(void *arg){
    button_pressed = true; 
    counter++;
}


void app_main(void)
{
    init();
    ESP_LOGI(TAG, "Init complete. Starting Polling FSM (Task 4)...");

    while (1) {
        

        //without_debounce(); // task 1
        //soft_debounce();   // task 2    
        //soft_state_debounce(); // task 3
        
        button_FSM(); // task 4
        
        
        
        vTaskDelay(POLLING_RATE_MS / portTICK_PERIOD_MS);
    }
}

void init() {


    esp_timer_create(&timer_args, &my_timer);
   
    gpio_reset_pin(LED_PIN);
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);


    gpio_reset_pin(BUTTON_PIN);
    gpio_set_direction(BUTTON_PIN, GPIO_MODE_INPUT);
    gpio_set_pull_mode(BUTTON_PIN, GPIO_PULLUP_ONLY);

      
    
    gpio_set_intr_type(BUTTON_PIN, GPIO_INTR_NEGEDGE);
    gpio_install_isr_service(0);
    gpio_isr_handler_add(BUTTON_PIN, gpio_isr_handler, (void*) BUTTON_PIN);
    
}

void button_FSM() {
    bool is_pin_low = (gpio_get_level(BUTTON_PIN) == 0);

    switch (btn_state) {
        case STATE_IDLE:
            if (is_pin_low) {
                btn_state = STATE_DEBOUNCE_PRESS;
                state_timer = 0; 
            }
            break;

        case STATE_DEBOUNCE_PRESS:
            state_timer++;
            if (state_timer >= (DEBOUNCE_DELAY_MS / POLLING_RATE_MS)) { 
                if (is_pin_low) {
                    btn_state = STATE_PRESSED;
                    led_state = !led_state; 
                    gpio_set_level(LED_PIN, led_state);
                    counter++;
                    ESP_LOGI(TAG, "Button pressed via FSM! LED: %d, Count: %d", led_state, counter);
                } else {
                    btn_state = STATE_IDLE; 
                }
            }
            break;

        case STATE_PRESSED:
            if (!is_pin_low) {
                btn_state = STATE_DEBOUNCE_RELEASE;
                state_timer = 0;
            }
            break;

        case STATE_DEBOUNCE_RELEASE:
            state_timer++;
            if (state_timer >= (DEBOUNCE_DELAY_MS / POLLING_RATE_MS)) {
                if (!is_pin_low) {
                    btn_state = STATE_IDLE; 
                } else {
                    btn_state = STATE_PRESSED; 
                }
            }
            break;
    }
}


void without_debounce() {
    if (button_pressed) {
        button_pressed = false;
        led_state = !led_state; 
        gpio_set_level(LED_PIN, led_state);
        ESP_LOGI(TAG, "Button pressed! Toggling LED... Count: %d", counter);
    }
}

void soft_debounce() {
    if (button_pressed && debounce_flag) {
        button_pressed = false;
        debounce_flag = false;
        led_state = !led_state; 
        gpio_set_level(LED_PIN, led_state);
        esp_timer_start_once(my_timer, 50000); 
        ESP_LOGI(TAG, "Button pressed! Toggling LED... Count: %d", counter);
    }
}

void soft_state_debounce() {
    if (button_pressed && !gpio_get_level(BUTTON_PIN)) {
        button_pressed = false;
        led_state = !led_state; 
        gpio_set_level(LED_PIN, led_state);
        ESP_LOGI(TAG, "Button pressed! Toggling LED... Count: %d", counter);
    }
}

void timer_callback(void* arg){

    button_pressed = false;

    debounce_flag = true;

}