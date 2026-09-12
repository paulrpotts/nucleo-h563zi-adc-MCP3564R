/*
 * task_uart_menu.c
 *
 *  Created on: Apr 29, 2026
 *      Author: paul
 */

/*
 * Peripherals
 */
#include "usart.h"

/*
 * FreeRTOS
 */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

/*
 * Application
 */
#include "debugging_uart_peripheral.h"

/*
 * Tasks
 */
#include "task_adc_MCP3564R_test.h"
#include "task_uart_menu.h"

/*
 * C Standard Library
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

/**********************************************************************************************************************/
/* PREPROCESSOR DEFINES ***********************************************************************************************/
/**********************************************************************************************************************/
#define UART_INPUT_BUF_LEN 32

/**********************************************************************************************************************/
/* FILE-SCOPE VARIABLES ***********************************************************************************************/
/**********************************************************************************************************************/
static uint8_t in_char;

TaskHandle_t uart_menu_task_h = NULL;

/**********************************************************************************************************************/
/* PRIVATE FUNCTION PROTOTYPES*****************************************************************************************/
/**********************************************************************************************************************/
static void adc_menu();

/**********************************************************************************************************************/
/* PUBLIC FUNCTIONS ***************************************************************************************************/
/**********************************************************************************************************************/
void uart_menu_task_setup(void)
{
    /*
     * Nothing currently needed here
     */
}

void uart_menu_task(void *pvParameters)
{
    uint32_t notify_bits;

    printf("\033[2J\033[H"); // Clear the screen and home the cursor

    while (1)
    {
        printf("--- STM32H5 debug options ---\r\n");
        printf("[A] - ADC\r\n");
        printf("Press a key: ");
        fflush(stdout);

        HAL_UART_Receive_IT(&DEBUG_UART_PERIPH_HANDLE, &in_char, 1);

        (void)xTaskNotifyWait(0x0, UART_MENU_TASK_NOTIFY_RX_COMPLETE, &notify_bits, portMAX_DELAY);
        assert(notify_bits & UART_MENU_TASK_NOTIFY_RX_COMPLETE);

        printf("%c\r\n", in_char);

        switch (in_char)
        {
            case 'A':
            case 'a':
                adc_menu();
                break;

            default:
                printf("Invalid option\r\n");
                break;
        }
    }
}

/**********************************************************************************************************************/
/* PRIVATE FUNCTIONS **************************************************************************************************/
/**********************************************************************************************************************/
static void adc_menu()
{
    uint32_t notify_bits;
    BaseType_t wait_ret_val;
    BaseType_t run_menu = pdTRUE;

    printf("\033[2J\033[H"); // Clear the screen and home the cursor

    while (pdTRUE == run_menu)
    {
        printf("--- ADC OPTIONS ---\r\n");
        printf("[G] - go - allow ADC task to run\r\n");
        printf("[S] - stop - pause ADC task\r\n");
        printf("[R] - show most recently queued values\r\n");
        printf("[X] - exit to main menu\r\n");
        printf("Press a key: ");
        fflush(stdout);

        HAL_UART_Receive_IT(&DEBUG_UART_PERIPH_HANDLE, &in_char, 1);

        wait_ret_val = xTaskNotifyWait(0x0, UART_MENU_TASK_NOTIFY_RX_COMPLETE, &notify_bits, portMAX_DELAY);
        assert ((pdTRUE == wait_ret_val) && (notify_bits & UART_MENU_TASK_NOTIFY_RX_COMPLETE));

        printf("%c\r\n", in_char);

        switch (in_char)
        {
            case 'G':
            case 'g':
                {
                    /*
                     * The actual value is irrelevant; putting any value in the queue allows the sweep
                     * task to run.
                     */
                    BaseType_t val = pdTRUE;
                    xQueueOverwrite(adc_read_test_task_can_run_q, &val);
                }
                break;

            case 'S':
            case 's':
                xQueueReset(adc_read_test_task_can_run_q);
                break;

            case 'R':
            case 'r':
                {
                    float voltage;
                    uint8_t voltage_idx;

                    for (voltage_idx = 0; voltage_idx < ADC_NUM_VOLTAGE_QUEUES; voltage_idx++)
                    {
                        if (1 == adc_get_v(voltage_idx, &voltage))
                        {
                            printf("Voltage index %d: %fV\r\n", voltage_idx, voltage);
                        }
                        else
                        {
                            printf("Voltage value at index %d not found in queue, skipping\r\n", voltage_idx);
                        }
                    }
                }
                break;

            case 'X':
            case 'x':
                printf("\033[2J\033[H");
                run_menu = pdFALSE;
                break;

            default:
                printf("Invalid option\r\n");
                break;
        }
    }
}
