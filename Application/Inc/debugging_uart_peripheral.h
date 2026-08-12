/*
 * debugging_uart_peripheral.h
 *
 *  Created on: Apr 29, 2026
 *      Author: paul
 */

#ifndef DEBUGGING_UART_PERIPHERAL_H_
#define DEBUGGING_UART_PERIPHERAL_H_

#include "usart.h"

/**********************************************************************************************************************/
/* PREPROCESSOR DEFINES ***********************************************************************************************/
/**********************************************************************************************************************/
/*
 * This file allows common FreeRTOS task code to access application-specific peripherals configured in STM32CubeMX.
 */
#define DEBUG_UART_PERIPH_HANDLE (huart5)

#endif /* DEBUGGING_UART_PERIPHERAL_H_ */
