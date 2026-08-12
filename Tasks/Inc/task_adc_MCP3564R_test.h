/*
 * task_ADC_MCP3564R_test.h
 *
 *  Created on: Mar 6, 2026
 *      Author: paul
 */
#ifndef INC_TASK_ADC_MCP3564R_H_
#define INC_TASK_ADC_MCP3564R_H_

#include "freertos.h"
#include "task.h"
#include "queue.h"

/*
 * Task notification flag bits so that interrupt handlers implemented in other files can signal the ADC read task
 * to indicate specific events
 */
#define ADC_TASK_NOTIFY_CONV_COMPLETE  (1 << 0x0) // Used for our external ADC hardware interrupt pin
#define ADC_TASK_NOTIFY_TX_COMPLETE    (1 << 0x1) // For HAL_SPI_Transmit_DMA() or HAL_SPI_Transmit_IT()
#define ADC_TASK_NOTIFY_TX_RX_COMPLETE (1 << 0x2) // For HAL_SPI_TransmitReceive_DMA() or HAL_SPI_TransmitReceive_IT()

#define ADC_TASK_ALL_NOTIFICATIONS     (ADC_TASK_NOTIFY_CONV_COMPLETE |\
                                        ADC_TASK_NOTIFY_TX_COMPLETE   |\
                                        ADC_TASK_NOTIFY_TX_RX_COMPLETE)

void adc_read_test_task_setup(void);
void adc_read_test_task(void *argument);

extern TaskHandle_t adc_read_test_task_h;
extern QueueHandle_t adc_read_test_task_can_run_q;

#define ADC_NUM_VOLTAGE_QUEUES 16

uint8_t adc_get_v(uint8_t chan_idx, float * voltage_p);

/*
 * An interrupt-safe version of adc_get_v that uses xQueuePeekFromISR() and does not block
 */
uint8_t adc_get_v_from_isr(uint8_t chan_idx, float * voltage_p);

#endif /* INC_TASK_ADC_MCP3564R_H_ */
