/*
 * adc_MCP3564R_gpio_exti.h
 *
 *  Created on: May 13, 2026
 *      Author: paul
 */

#ifndef ADC_MCP3564R_GPIO_EXTI_H_
#define ADC_MCP3564R_GPIO_EXTI_H_

/**********************************************************************************************************************/
/* PREPROCESSOR DEFINES ***********************************************************************************************/
/**********************************************************************************************************************/
/*
 * Allow us to easily change the EXTI number of the GPIO pin we're using for the ADC interrupt pin. Note that multiple
 * GPIO peripherals have pins that can be configured to generate the same EXTI interrupt and we can't distinguish them.
 *
 * Our interrupt pin is PA2 on the Nucleo board and production hardware board.
 */
#define ADC_GPIO_EXTI (GPIO_PIN_2)

#endif /* ADC_MCP3564R_SPI_PERIPH_H_ */
