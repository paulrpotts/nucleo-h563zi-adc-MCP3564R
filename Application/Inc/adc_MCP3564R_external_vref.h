/*
 * adc_MCP3564R_External_vref.c
 *
 *  Created on: May 13, 2026
 *      Author: paul
 */

#ifndef ADC_MCP3564R_EXTERNAL_VREF_H_
#define ADC_MCP3564R_EXTERNAL_VREF_H_

/**********************************************************************************************************************/
/* PREPROCESSOR DEFINES ***********************************************************************************************/
/**********************************************************************************************************************/
/*
 * #include this .c file directly in the task_ADC_MCP3564R.c when configuring the ADC to use an external voltage
 * reference. This allows us to easily change the voltage reference without modifying the common task code.
 *
 * This value is for an Analog Devices MAX6071 part. Please see the datasheet for external voltage reference
 * requirements.
 */
#define MCP3564_EXT_VREF_VOLTAGE_F (2.5F)

#endif /* ADC_MCP3564R_EXTERNAL_VREF_H_ */
