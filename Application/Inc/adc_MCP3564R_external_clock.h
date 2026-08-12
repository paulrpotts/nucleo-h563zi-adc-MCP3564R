/*
 * adc_MCP3564R_External_clock.c
 *
 *  Created on: May 13, 2026
 *      Author: paul
 */

#ifndef ADC_MCP3564R_EXTERNAL_CLOCK_H_
#define ADC_MCP3564R_EXTERNAL_CLOCK_H_

/**********************************************************************************************************************/
/* PREPROCESSOR DEFINES ***********************************************************************************************/
/**********************************************************************************************************************/
/*
 * #include this .c file directly in main.c to start the configured timer generating the external ADC clock.
 */
#define ADC_MCP3564R_START_EXT_CLOCK() do { HAL_TIM_OC_Start(&htim4, TIM_CHANNEL_3); } while(0)

#endif /* ADC_MCP3564R_EXTERNAL_CLOCK_H_ */
