#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_log.h"

#define POLLING_RATE_MS     10
#define GPIO_YELLOW_MASTER  2
#define GPIO_RED_MASTER     4  
#define GPIO_GREEN_MASTER   5


#define GPIO_YELLOW_SLAVE   18
#define GPIO_RED_SLAVE      19
#define GPIO_GREEN_SLAVE    21


#define GPIO_SWITCH         0


#define TIME_RED_MASTER      5000
#define TIME_GREEN_MASTER    4000

#define TIME_RED_SLAVE       TIME_GREEN_MASTER
#define TIME_GREEN_SLAVE     TIME_RED_MASTER

#define TIME_RED_YELLOW      1000

#define TIME_FLASING_GREEN   1000

#define BLINK_TIME           500

#define TIME_YELLOW          1000










static const char *TAG = "MY_APP";

typedef enum {
    STATE_FLASHING_YELLOW,
    STATE_RED,
    STATE_GREEN,
    STATE_YELLOW,
    STATE_FLASHING_GREEN,
    STATE_RED_YELLOW

}traffic_light_state_t;

typedef struct {
    gpio_num_t gpio_red;
    gpio_num_t gpio_yellow;
    gpio_num_t gpio_green;
    bool is_master;
    bool is_on;

    traffic_light_state_t state;

    uint32_t last_transition_time;
    uint32_t last_blink_time;
} traffic_light_t;

traffic_light_t master_light;
traffic_light_t slave_light;




void init(void);
void traffic_light_FSM(traffic_light_t *tl);
void init_traffic_light(traffic_light_t *light, gpio_num_t gpio_red, gpio_num_t gpio_yellow, gpio_num_t gpio_green, bool is_master);
void set_leds(traffic_light_t *tl, bool red, bool yellow, bool green);







void app_main(void)
{
    init();
    ESP_LOGI(TAG, "Init complete.");

    while (1) {
        

            
        traffic_light_FSM(&master_light);
        traffic_light_FSM(&slave_light);
        
        if(!gpio_get_level(GPIO_SWITCH)){
            master_light.is_on = true;
            slave_light.is_on = true;
        }
        else{
            master_light.is_on = false;
            slave_light.is_on = false;
        }
        
        
        vTaskDelay(POLLING_RATE_MS / portTICK_PERIOD_MS);
    }
}

void init() {

    gpio_reset_pin(GPIO_YELLOW_MASTER);
    gpio_set_direction(GPIO_YELLOW_MASTER, GPIO_MODE_INPUT_OUTPUT);
    gpio_reset_pin(GPIO_RED_MASTER);
    gpio_set_direction(GPIO_RED_MASTER, GPIO_MODE_INPUT_OUTPUT);
    gpio_reset_pin(GPIO_GREEN_MASTER);
    gpio_set_direction(GPIO_GREEN_MASTER, GPIO_MODE_INPUT_OUTPUT);

    gpio_reset_pin(GPIO_YELLOW_SLAVE);
    gpio_set_direction(GPIO_YELLOW_SLAVE, GPIO_MODE_INPUT_OUTPUT);
    gpio_reset_pin(GPIO_RED_SLAVE);
    gpio_set_direction(GPIO_RED_SLAVE, GPIO_MODE_INPUT_OUTPUT);
    gpio_reset_pin(GPIO_GREEN_SLAVE);
    gpio_set_direction(GPIO_GREEN_SLAVE, GPIO_MODE_INPUT_OUTPUT);


    set_leds(&master_light, 0, 0, 0);
    set_leds(&slave_light, 0, 0, 0);
   

    init_traffic_light(&master_light, GPIO_RED_MASTER, GPIO_YELLOW_MASTER, GPIO_GREEN_MASTER, true);
    init_traffic_light(&slave_light, GPIO_RED_SLAVE, GPIO_YELLOW_SLAVE, GPIO_GREEN_SLAVE, false);


   
    
}

void init_traffic_light(traffic_light_t *tl, gpio_num_t gpio_red, gpio_num_t gpio_yellow, gpio_num_t gpio_green, bool is_master){
    tl->gpio_red = gpio_red;
    tl->gpio_yellow = gpio_yellow;
    tl->gpio_green = gpio_green;
    tl->is_master = is_master;
    tl->is_on = false;
    tl->state = STATE_FLASHING_YELLOW;
    tl->last_transition_time = xTaskGetTickCount() / portTICK_PERIOD_MS;
    tl->last_blink_time = xTaskGetTickCount() / portTICK_PERIOD_MS;

}



void traffic_light_FSM(traffic_light_t *tl) {

    uint32_t current_time = xTaskGetTickCount() / portTICK_PERIOD_MS;

    switch (tl->state) {
        case STATE_FLASHING_YELLOW:

            

            if(current_time - tl->last_blink_time > BLINK_TIME){
                gpio_set_level(tl->gpio_yellow, !gpio_get_level(tl->gpio_yellow));
                tl->last_blink_time = xTaskGetTickCount() / portTICK_PERIOD_MS;
            }

            if(tl->is_on && tl->is_master){
                    tl->state = STATE_GREEN;
                    tl->last_transition_time = xTaskGetTickCount() / portTICK_PERIOD_MS;
            }
            else if(tl->is_on  && !tl->is_master){
                    tl->state = STATE_RED;
                    tl->last_transition_time = xTaskGetTickCount() / portTICK_PERIOD_MS;
            }
            
            break;


            case STATE_RED:
            set_leds(tl, true, false, false);

            

            if((current_time - tl->last_transition_time > (TIME_RED_MASTER-TIME_RED_YELLOW)) && tl->is_master){
                    tl->state = STATE_RED_YELLOW;
                    tl->last_transition_time = xTaskGetTickCount() / portTICK_PERIOD_MS;
                    
            }
            else if((current_time - tl->last_transition_time  > (TIME_RED_SLAVE - TIME_RED_YELLOW)) && !(tl->is_master)){
                    tl->state = STATE_RED_YELLOW;
                    tl->last_transition_time = xTaskGetTickCount() / portTICK_PERIOD_MS;
            }

            break;

            case STATE_RED_YELLOW:
            set_leds(tl, 1, 1, 0);
            

            if(current_time - tl->last_transition_time > TIME_RED_YELLOW){
                tl->state = STATE_GREEN;
                tl->last_transition_time = xTaskGetTickCount() / portTICK_PERIOD_MS; 
            }
                     
            break;

            case STATE_GREEN:
            set_leds(tl, 0, 0, 1);
        
            if(current_time - tl->last_transition_time > TIME_GREEN_MASTER && tl->is_master){
                tl->state = STATE_FLASHING_GREEN;
                tl->last_transition_time = xTaskGetTickCount() / portTICK_PERIOD_MS;
            }
            else if(current_time - tl->last_transition_time> TIME_GREEN_SLAVE-TIME_FLASING_GREEN && !(tl->is_master)){
                tl->state = STATE_FLASHING_GREEN;;
                tl->last_transition_time = xTaskGetTickCount() / portTICK_PERIOD_MS;
            }
            break;

            case STATE_FLASHING_GREEN:
           
            
            if(current_time - tl->last_transition_time > TIME_FLASING_GREEN){
                tl->state = STATE_YELLOW;
                tl->last_transition_time = xTaskGetTickCount() / portTICK_PERIOD_MS;
            }
            if(current_time - tl->last_blink_time > BLINK_TIME){
                gpio_set_level(tl->gpio_green, !gpio_get_level(tl->gpio_green));
                tl->last_blink_time = xTaskGetTickCount() / portTICK_PERIOD_MS;
            }


            
            break;

            case STATE_YELLOW:
            set_leds(tl, 0, 1, 0);
            
            if(current_time - tl->last_transition_time > TIME_YELLOW){
                tl->state = STATE_RED;
                tl->last_transition_time = xTaskGetTickCount() / portTICK_PERIOD_MS;
            }
            break;

            default:
            set_leds(tl, 1, 1, 1);
            break;
    
    }

}


void set_leds(traffic_light_t *tl, bool red, bool yellow, bool green){
    gpio_set_level(tl->gpio_red, red);
    gpio_set_level(tl->gpio_yellow, yellow);
    gpio_set_level(tl->gpio_green, green);
}

    
