/*
 * adc_MCP3564R_spi_periph.h
 *
 *  Created on: Apr 24, 2026
 *      Author: paul
 */

#ifndef ADC_MCP3564R_SPI_PERIPH_H_
#define ADC_MCP3564R_SPI_PERIPH_H_

/*
 * Peripherals
 */
#include "spi.h"

/**********************************************************************************************************************/
/* PREPROCESSOR DEFINES ***********************************************************************************************/
/**********************************************************************************************************************/
/*
 * This file allows common FreeRTOS task code to access application-specific peripherals configured in STM32CubeMX.
 */
#define ADC_SPI_PERIPH_HANDLE (hspi3)

#endif /* ADC_MCP3564R_SPI_PERIPH_H_ */
