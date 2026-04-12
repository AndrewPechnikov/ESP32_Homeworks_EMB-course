#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"

#define TXD_PIN (GPIO_NUM_1)
#define RXD_PIN (GPIO_NUM_3)
#define LED_PIN (GPIO_NUM_45)
#define BTN_PIN (GPIO_NUM_8)

#define UART_PORT_NUM UART_NUM_0
#define BUF_SIZE (1024)

static uint8_t led_enabled = 0;

void init_hw() {
    const uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    uart_driver_install(UART_PORT_NUM, BUF_SIZE * 2, 0, 0, NULL, 0);
    uart_param_config(UART_PORT_NUM, &uart_config);
    uart_set_pin(UART_PORT_NUM, TXD_PIN, RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    gpio_reset_pin(LED_PIN);
    gpio_set_direction(LED_PIN, GPIO_MODE_OUTPUT);
    gpio_reset_pin(BTN_PIN);
    gpio_set_direction(BTN_PIN, GPIO_MODE_INPUT);
    gpio_set_pull_mode(BTN_PIN, GPIO_PULLUP_ONLY);
}

void uart_rx_task(void *arg) {
    uint8_t* data = (uint8_t*) malloc(BUF_SIZE);
    while (1) {
        int len = uart_read_bytes(UART_PORT_NUM, data, BUF_SIZE, 20 / portTICK_PERIOD_MS);
        if (len > 0) {
            for (int i = 0; i < len; i++) {
                if (data[i] == '1') {
                    gpio_set_level(LED_PIN, 1);
                } else if (data[i] == '0') {
                    gpio_set_level(LED_PIN, 0);
                }
            }
        }
    }
}

void button_task(void *arg) {
    int last_state = 1;
    while (1) {
        int current_state = gpio_get_level(BTN_PIN);
    
        if (last_state == 1 && current_state == 0) {
            vTaskDelay(pdMS_TO_TICKS(50));
            if (gpio_get_level(BTN_PIN) == 0) {
                led_enabled = !led_enabled;
                const char* msg = led_enabled ? "1" : "0";
                uart_write_bytes(UART_PORT_NUM, msg, 1);
            }
        }
        last_state = current_state;
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void app_main(void) {
    init_hw();

    xTaskCreate(uart_rx_task, "uart_rx_task", 4096, NULL, 10, NULL);
    xTaskCreate(button_task, "button_task", 4096, NULL, 10, NULL);
}