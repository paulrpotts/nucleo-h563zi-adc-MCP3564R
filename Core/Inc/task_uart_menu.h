/*
 * task_uart_menu.h
 *
 *  Created on: Apr 29, 2026
 *      Author: paul
 */

#ifndef INC_TASK_UART_MENU_H_
#define INC_TASK_UART_MENU_H_

#include "freertos.h"
#include "task.h"

/*
 * Task notification flag bits so that interrupt handlers implemented in other files can signal the UART menu task to
 * indicate certain events
 */
#define UART_MENU_TASK_NOTIFY_RX_COMPLETE (1 << 0x0)

void uart_menu_task_setup(void);
void uart_menu_task(void *pvParameters);

extern TaskHandle_t uart_menu_task_h;

#endif /* INC_TASK_UART_MENU_H_ */
