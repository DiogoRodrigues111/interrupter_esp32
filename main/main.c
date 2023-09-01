/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

/*
* ISR Program

* Main Programmer: Diogo Rodrigues Roessler
*/

#include <stdio.h>
#include <inttypes.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_chip_info.h"
#include "esp_flash.h"
#include "driver/gpio.h"
#include "esp_intr_alloc.h"

typedef enum {
    FALSE = 0,
    TRUE = 1
} BOOLEAN ;

/* GPIOs Port */

#define PUSH_BUTTON         (GPIO_NUM_12)
#define LED                 (GPIO_NUM_26)

void StartProgram(void);
void StopProgram(void* pvParameter);
void yield(void);

/* Function for wait the interruption */
void StopProgram(void* pvParameter)
{
    while (1) {
        /* Wait a interruption occurs ( maybe can used semph or queue ) */
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        /* Get process interruption after here */
    }
}

/* Handler ISR - Stop the original program with ISR*/
void IRAM_ATTR ISR_StopProgram(void *arg)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    TaskHandle_t tskStopProgram = (void *)&StopProgram;
    vTaskNotifyGiveFromISR(tskStopProgram, &xHigherPriorityTaskWoken);
    if (xHigherPriorityTaskWoken == pdTRUE) {
        yield();
    }
    /* Loop of RTOS */
    vTaskStartScheduler();
}

/* Normal Program running */
void StartProgram(void)
{
    /* START PROGRAM */

    gpio_set_level(LED, 1);
    vTaskDelay(1000/portTICK_PERIOD_MS);
    gpio_set_level(LED, 0);
    vTaskDelay(1000/portTICK_PERIOD_MS);
}

/* Call this abstraction for used later in ISR handler */
void yield(void)
{
    /* ISR occurs now here */

    portYIELD_FROM_ISR();
}

void app_main(void)
{
    /* CONFIGURE Led */
    gpio_set_direction(LED, GPIO_MODE_OUTPUT);
    
    /* esp_rom_gpio_pad_select_gpio(PUSH_BUTTON); */
    
    /* GPIO install handler for ISR */
    gpio_install_isr_service(0);

    /* CONFIGURE Push Button Input Mode */
    gpio_config_t push_gpio = {
        .intr_type = GPIO_INTR_NEGEDGE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << PUSH_BUTTON),
        .pull_up_en = GPIO_PULLUP_ENABLE
    };
    gpio_config(&push_gpio);

    /* Get value of PushButton */
    BOOLEAN volatile ReadPushButton = (gpio_get_level(PUSH_BUTTON) == 1);
    TaskHandle_t isr_stopprogram_handler = NULL;

    while (1) {
        if (ReadPushButton)
        {
            /* Create a task for serialize StopProgram to notification for RTOS */
            xTaskCreate(StopProgram, "StopProgram", configMINIMAL_STACK_SIZE, NULL, 5, &isr_stopprogram_handler);  
            if (isr_stopprogram_handler != NULL)
            {
                /* Call the Interruption */
                gpio_isr_handler_add(PUSH_BUTTON, ISR_StopProgram, NULL);
            }
        }
        else {
            vTaskEndScheduler();
        }
        
        /* Always start the program */
        StartProgram();
    }
}
