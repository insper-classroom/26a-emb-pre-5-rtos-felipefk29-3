/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <FreeRTOS.h>
#include <task.h>
#include <semphr.h>
#include <queue.h>


#include <stdio.h>
#include <string.h> 
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"
#include "hardware/irq.h"

const int BTN_PIN_R = 28;
const int BTN_PIN_Y = 21;

const int LED_PIN_R = 5;
const int LED_PIN_Y = 10;


QueueHandle_t xQueueButId;
SemaphoreHandle_t xSemaphore_r;

QueueHandle_t xQueueBtn2;
SemaphoreHandle_t xSemaphore_y;



void btn_callback(uint gpio, uint32_t events) {
    if (events == 0x4) { // fall edge
        if(gpio == BTN_PIN_R){
            xSemaphoreGiveFromISR(xSemaphore_r, 0);
        } else if(gpio == BTN_PIN_Y){
            xSemaphoreGiveFromISR(xSemaphore_y, 0);
        }
    }
}

void led_1_task(void *p) {
  gpio_init(LED_PIN_R);
  gpio_set_dir(LED_PIN_R, GPIO_OUT);
  int delay = 100;
  int piscando_r = 0;
  while (true) {
    xQueueReceive(xQueueButId, &piscando_r, pdMS_TO_TICKS(delay));
    if (piscando_r) {
      gpio_put(LED_PIN_R, !gpio_get(LED_PIN_R));
    } else {
      gpio_put(LED_PIN_R, 0);
    }
  }
}

void led_2_task(void *p) {
  gpio_init(LED_PIN_Y);
  gpio_set_dir(LED_PIN_Y, GPIO_OUT);
  int delay = 100;
  int piscando_y = 0;
  while (true) {
    
    xQueueReceive(xQueueBtn2, &piscando_y, pdMS_TO_TICKS(delay));
    if (piscando_y) {
      gpio_put(LED_PIN_Y, !gpio_get(LED_PIN_Y));
    } else {
      gpio_put(LED_PIN_Y, 0);
    }
  }
}



void btn_task(void* p) {
    gpio_init(BTN_PIN_R);
    gpio_set_dir(BTN_PIN_R, GPIO_IN);
    gpio_pull_up(BTN_PIN_R);

    gpio_init(BTN_PIN_Y);
    gpio_set_dir(BTN_PIN_Y, GPIO_IN);
    gpio_pull_up(BTN_PIN_Y);
    gpio_set_irq_enabled_with_callback(BTN_PIN_R, GPIO_IRQ_EDGE_FALL, true,
                                       &btn_callback);
    gpio_set_irq_enabled(BTN_PIN_Y, GPIO_IRQ_EDGE_FALL, true);

    int piscando_r = 0;
    int piscando_y = 0;
    while (true) {
        if (xSemaphoreTake(xSemaphore_r, pdMS_TO_TICKS(10)) == pdTRUE){
            piscando_r = !piscando_r;
            xQueueSend(xQueueButId,&piscando_r,0);
        }
        if (xSemaphoreTake(xSemaphore_y, pdMS_TO_TICKS(10)) == pdTRUE){
            piscando_y = !piscando_y;
            xQueueSend(xQueueBtn2,&piscando_y,0);
        }
    }
}



int main() {
    stdio_init_all();

    xQueueButId = xQueueCreate(32, sizeof(int));
    xSemaphore_r = xSemaphoreCreateBinary();
    xQueueBtn2 = xQueueCreate(32, sizeof(int));
    xSemaphore_y = xSemaphoreCreateBinary();

    xTaskCreate(led_1_task, "LED_Task 1", 256, NULL, 1, NULL);
    xTaskCreate(btn_task, "BTN_Task 1", 256, NULL, 1, NULL);
    xTaskCreate(led_2_task, "LED_Task 2", 256, NULL, 1, NULL);

    vTaskStartScheduler();

    while(1){}

    return 0;
}