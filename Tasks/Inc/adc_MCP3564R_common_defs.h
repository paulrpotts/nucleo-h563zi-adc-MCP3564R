/*
 * adc_MCP3564R_common_defs.h
 *
 *  Created on: Jul 29, 2026
 *      Author: paul
 */
#ifndef INC_FREERTOS_TASKS_ADC_MCP3564R_COMMON_DEFS_H_
#define INC_FREERTOS_TASKS_ADC_MCP3564R_COMMON_DEFS_H_

/*
 * ADC commands, registers, and bits
 *
 * Source: MCP3561_2_4R-Data-Sheet-DS200006391C.pdf
 *
 * NOTE: device address bits in CMD[7:6] vary by specific device ordered. The part I tested with responds to 0b01.
 * Address bits along with CMD[5:2] and CMD[1:0] form a command byte.
 *
 * Bits CMD[5:2] either refer to a register address or a fast command
 * Bits CMD[0:1] indicate incremental write, incremental read, static read, or fast command
 */
#define MCP3564R_ADDR_BITS               (0x1 << 6)

#define MCP3564R_ADC_CONV_START_BITS     (0xA << 2)
#define MCP3564R_ADC_STANDBY_BITS        (0xB << 2)
#define MCP3564R_ADC_SHUTDOWN_BITS       (0xC << 2)
#define MCP3564R_FULL_SHUTDOWN_BITS      (0xD << 2)
#define MCP3564R_FULL_RESET_BITS         (0xE << 2)

#define MCP3564R_FAST_CMD_BITS           (0x0)
#define MCP3564_STATIC_READ_BITS         (0x1)
#define MCP3564_INC_WRITE_BITS           (0x2)
#define MCP3564_INC_READ_BITS            (0x3)

#define MCP3564R_CONV_START_CMD          (MCP3564R_ADDR_BITS | MCP3564R_ADC_CONV_START_BITS | MCP3564R_FAST_CMD_BITS)
#define MCP3564R_STDBY_CMD               (MCP3564R_ADDR_BITS | MCP3564R_ADC_STANDBY_BITS    | MCP3564R_FAST_CMD_BITS)
#define MCP3564R_SHUTDOWN_CMD            (MCP3564R_ADDR_BITS | MCP3564R_ADC_SHUTDOWN_BITS   | MCP3564R_FAST_CMD_BITS)
#define MCP3564R_FULL_SHUTDOWN_CMD       (MCP3564R_ADDR_BITS | MCP3564R_FULL_SHUTDOWN_BITS  | MCP3564R_FAST_CMD_BITS)
#define MCP3564R_FULL_RESET_CMD          (MCP3564R_ADDR_BITS | MCP3564R_FULL_RESET_BITS     | MCP3564R_FAST_CMD_BITS)

#define MCP3564_REG_ADCDATA              (0x0) // Latest A/D conversion data, 24 or 32 bits, or modulator output stream
                                               // 4/24/32 bits depending on mode, read only
#define MCP3564_REG_CONFIG0              (0x1) // ADC operating mode, master clock mode, & input bias current source
                                               // 8 bits
#define MCP3564_REG_CONFIG1              (0x2) // Pre-scale & OSR settings
                                               // 8 bits
#define MCP3564_REG_CONFIG2              (0x3) // ADC boost & gain, auto-zeroing for analog mux, voltage ref, & ADC
                                               // 8 bits
#define MCP3564_REG_CONFIG3              (0x4) // Conversion mode, data & CRC format, CRC enable, offset, gain error
                                               // 8 bits
#define MCP3564_REG_IRQ                  (0x5) // IRQ status bits & mode settings; enable for fast cmds and conv start
                                               // 8 bits
#define MCP3564_REG_MUX                  (0x6) // Analog multiplex input when in MUX mode
                                               // 8 bits
#define MCP3564_REG_SCAN                 (0x7) // SCAN mode settings
                                               // 24 bits
#define MCP3564_REG_TIMER                (0x8) // Delay value for TIMER between SCAN cycles
                                               // 24 bits
#define MCP3564_REG_OFFSETCAL            (0x9) // ADC digital offset calibration value
                                               // 24 bits
#define MCP3564_REG_GAINCAL              (0xA) // ADC digital gain calibration value
                                               // 24 bits
#define MCP3564_REG_LOCK                 (0xD) // Password value for SPI write mode locking
                                               // 8 bits
#define MCP3564_REG_CRCCFG               (0xF) // CRC checksum for device configuration
                                               // 16 bits

#define MCP3564_GEN_STATIC_READ_CMD(reg) (MCP3564R_ADDR_BITS | (reg << 2) | MCP3564_STATIC_READ_BITS)
#define MCP3564_GEN_INC_WRITE_CMD(reg)   (MCP3564R_ADDR_BITS | (reg << 2) | MCP3564_INC_WRITE_BITS)
#define MCP3564_GEN_INC_READ_CMD(reg)    (MCP3564R_ADDR_BITS | (reg << 2) | MCP3564_INC_READ_BITS)

/*
 * CONFIG0 field values
 */
#define MCP3564_CONFIG0_VREF_SEL_INT     (0x1 << 7) // Default
#define MCP3564_CONFIG0_VREF_SEL_EXT     (0x0 << 7)

#define MCP3564_CONFIG0_PART_SHUTDOWN_N  (0x1 << 6) // Default
#define MCP3564_CONFIG0_PART_SHUTDOWN_Y  (0x0 << 6)

#define MCP3564_CONFIG0_CLK_INT_PIN_ON   (0x3 << 4)
#define MCP3564_CONFIG0_CLK_INT_PIN_OFF  (0x2 << 4)
#define MCP3546_CONFIG0_CLK_EXT          (0x1 << 4) // Per datasheet, default value 0x0 also selects external clock

#define MCP3564_CONFIG0_CUR_BIAS_15_UA   (0x3 << 2)
#define MCP3564_CONFIG0_CUR_BIAS_3_7_UA  (0x2 << 2)
#define MCP3564_CONFIG0_CUR_BIAS_0_9_UA  (0x1 << 2)
#define MCP3564_CONFIG0_CUR_BIAS_NONE    (0x0 << 2) // Default

#define MCP3564_CONFIG0_ADC_MODE_CNV     (0x3 << 0)
#define MCP3564_CONFIG0_ADC_MODE_STBY    (0x2 << 0)
#define MCP3564_CONFIG0_ADC_MODE_SHTDWN  (0x1 << 0)
#define MCP3564_CONFIG0_ADC_MODE_SHTDWN2 (0x1 << 0) // Per datasheet, default value 0x0 also selects shutdown mode

/*
 * CONFIG1 field values
 */
#define MCP3564_CONFIG1_AMCLK_PRE_DIV_8  (0x3 << 6)
#define MCP3564_CONFIG1_AMCLK_PRE_DIV_4  (0x2 << 6)
#define MCP3564_CONFIG1_AMCLK_PRE_DIV_2  (0x1 << 6)
#define MCP3564_CONFIG1_AMCLK_PRE_DIV_1  (0x0 << 6) // Default

#define MCP3564_CONFIG1_OVERSAMP_98304   (0xF << 2) // Non-power-of-2 value is 2^13 * 12
#define MCP3564_CONFIG1_OVERSAMP_81920   (0xE << 2) // Non-power-of-2 value is 2^13 * 10
#define MCP3564_CONFIG1_OVERSAMP_49152   (0xD << 2) // Non-power-of-2 value is 2^12 * 12
#define MCP3564_CONFIG1_OVERSAMP_40960   (0xC << 2) // Non-power-of-2 value is 2^12 * 10
#define MCP3564_CONFIG1_OVERSAMP_24576   (0xB << 2) // Non-power-of-2 value is 2^11 * 12
#define MCP3564_CONFIG1_OVERSAMP_20480   (0xA << 2) // Non-power-of-2 value is 2^11 * 10
#define MCP3564_CONFIG1_OVERSAMP_16384   (0x9 << 2)
#define MCP3564_CONFIG1_OVERSAMP_8192    (0x8 << 2)
#define MCP3564_CONFIG1_OVERSAMP_4096    (0x7 << 2)
#define MCP3564_CONFIG1_OVERSAMP_2048    (0x6 << 2)
#define MCP3564_CONFIG1_OVERSAMP_1024    (0x5 << 2)
#define MCP3564_CONFIG1_OVERSAMP_512     (0x4 << 2)
#define MCP3564_CONFIG1_OVERSAMP_256     (0x3 << 2) // Default
#define MCP3564_CONFIG1_OVERSAMP_128     (0x2 << 2)
#define MCP3564_CONFIG1_OVERSAMP_64      (0x1 << 2)
#define MCP3564_CONFIG1_OVERSAMP_32      (0x0 << 2)

/*
 * Bits 1:0 are reserved and should always be set to 0
 */
#define MCP3564_CONFIG1_RESERVED_BITS    (0x0 << 0)

/*
 * CONFIG2 field values
 */
#define MCP3564_CONFIG2_BIAS_CUR_2X      (0x3 << 6)
#define MCP3564_CONFIG2_BIAS_CUR_1X      (0x2 << 6) // Default
#define MCP3564_CONFIG2_BIAS_CUR_0_66X   (0x1 << 6)
#define MCP3564_CONFIG2_BIAS_CUR_0_5X    (0x0 << 6)

#define MCP3564_CONFIG2_GAIN_X64         (0x7 << 3)
#define MCP3564_CONFIG2_GAIN_X32         (0x6 << 3)
#define MCP3564_CONFIG2_GAIN_X16         (0x5 << 3)
#define MCP3564_CONFIG2_GAIN_X8          (0x4 << 3)
#define MCP3564_CONFIG2_GAIN_X4          (0x3 << 3)
#define MCP3564_CONFIG2_GAIN_X2          (0x2 << 3)
#define MCP3564_CONFIG2_GAIN_X1          (0x1 << 3) // Default
#define MCP3564_CONFIG2_GAIN_X_ONE_THIRD (0x0 << 3)

#define MCP3564_CONFIG2_AUTOZERO_MUX_ON  (0x1 << 2)
#define MCP3564_CONFIG2_AUTOZERO_MUX_OFF (0x0 << 2) // Default

#define MCP3564_CONFIG2_AUTOZERO_CHOP    (0x1 << 1) // Default, has no effect when external VREF is enabled
#define MCP3564_CONFIG2_AUTOZERO_NOCHOP  (0x0 << 1)

/*
 * Bit 0 is reserved and should always be set to 0
 */
#define MCP3564_CONFIG2_RESERVED_BITS    (0x1 << 0)

/*
 * CONFIG3 field values
 */
#define MCP3564_CONFIG3_CNV_MODE_CONT    (0x3 << 6)
#define MCP3564_CONFIG3_CNV_MODE_1STBY   (0x2 << 6) // Sets mode to standby at end of conversion or cycle in SCAN mode
#define MCP3564_CONFIG3_CNV_MODE_1SHTDWN (0x1 << 6) // Default, sets mode to shutdown after
// NOTE: per datasheet, one-shot conversion or one-shot cycle followed by shutdown is 0b0x; value of bit 6 is unspecified

#define MCP3564_CONFIG3_DATA_FMT_32_CHAN (0x3 << 4) // 4 channel ID bits, 4-bit sign ext, 24-bit right-justified data
                                                    // Sign extension allows over-range indication
#define MCP3564_CONFIG3_DATA_FMT_32_RJ   (0x2 << 4) // 8-bit sign extension, right-justified 24-bit value
                                                    // Sign extension allows over-range indication
#define MCP3564_CONFIG3_DATA_FMT_32_LJ   (0x1 << 4) // 24-bit left-justified data, LSB is zero
#define MCP3564_CONFIG3_DATA_FMT_24      (0x0 << 4) // Default, 24-bit data

#define MCP3564_CONFIG3_CRC_FMT_32       (0x1 << 3) // 16-bit CRC followed by 16 zero bits
#define MCP3564_CONFIG3_CRC_FMT_16       (0x0 << 3) // Default, 16-bit CRC

#define MCP3564_CONFIG3_CRCCOM_ENABLED   (0x1 << 2) // CRC on communications enabled; does not affect CRCCFG calculations
#define MCP3564_CONFIG3_CRCCOM_DISABLED  (0x0 << 2) // Default, CRC on communications disabled

#define MCP3564_CONFIG3_OFFCAL_ENABLED   (0x1 << 1) // Digital offset calibration enabled
#define MCP3564_CONFIG3_OFFCAL_DISABLED  (0x0 << 1) // Default, digital offset calibration disabled

#define MCP3564_CONFIG3_GAINCAL_ENABLED  (0x1 << 0) // Digital gain calibration enabled
#define MCP3564_CONFIG3_GAINCAL_DISABLED (0x0 << 0) // Default, digital gain calibration disabled

/*
 * IRQ Register field values
 */
// Bit 7 of the IRQ register is unimplemented and will be read as zero
#define MCP3564_IRQ_REG_DR_NO            (0x1 << 6) // ADCDATA has not been updated since reset or last read
#define MCP3564_IRQ_REG_DR_YES           (0x0 << 6) // New ADCDATA is ready for reading

#define MCP3564_IRQ_REG_CRCCFG_NO        (0x1 << 5) // CRC error has not occurred for configuration registers
#define MCP3564_IRQ_REG_CRCCFG_YES       (0x0 << 5) // CRC error has not occurred

#define MCP3564_IRQ_REG_POR_NO           (0x1 << 4) // POR has not occurred since last reading
#define MCP3564_IRQ_REG_POR_YES          (0x0 << 4) // POR has occurred

#define MCP3564_IRQ_REG_IRQ_MD_MDAT      (0x1 << 3) // MDAT is on the !IRQ/MDAT pin, with POR and CRC interrupts
                                                    // which take priority over the MDAT output
#define MCP3564_IRQ_REG_IRQ_MD_INTS      (0x0 << 3) // All interrupts can appear on !IRQ/MDAT pin

#define MCP3564_IRQ_REG_IRQ_MD_IDLE_HI   (0x1 << 2) // Inactive state is high; no pull-up resistor required
#define MCP3564_IRQ_REG_IRQ_MD_IDLE_HIZ  (0x0 << 2) // Default: inactive state high-Z, requires pull-up to DVDD

#define MCP3564_IRQ_REG_EN_FASTCMD       (0x1 << 1) // Default: enable fast commands
#define MCP3564_IRQ_REG_DIS_FASTCMD      (0x0 << 1) // Disable fast commands

#define MCP3564_IRQ_REG_EN_STP           (0x1 << 0) // Default: conversion start interrupt output enabled
#define MCP3564_IRQ_REG_DIS_STP          (0x0 << 0) // Conversion start interrupt output disabled

/*
 * Multiplexer Register field values
 */
#define VIN_PLUS_VCM                     (0xF << 4) // Internal VCM
#define VIN_PLUS_TEMP_M                  (0xE << 4) // Internal temperature sensor diode M
#define VIN_PLUS_TEMP_P                  (0xD << 4) // Internal temperature sensor diode P
#define VIN_PLUS_REFINM                  (0xC << 4) // REFIN-
#define VIN_PLUS_REFINP                  (0xB << 4) // REFIN+/OUT
// 0xA << 4 is reserved; do not use
#define VIN_PLUS_AVDD                    (0x9 << 4) // AVDD
#define VIN_PLUS_AGND                    (0x8 << 4) // AGND
#define VIN_PLUS_CH7                     (0x7 << 4) // CH7
#define VIN_PLUS_CH6                     (0x6 << 4) // CH6
#define VIN_PLUS_CH5                     (0x5 << 4) // CH5
#define VIN_PLUS_CH4                     (0x4 << 4) // CH4
#define VIN_PLUS_CH3                     (0x3 << 4) // CH3
#define VIN_PLUS_CH2                     (0x2 << 4) // CH2
#define VIN_PLUS_CH1                     (0x1 << 4) // CH1
#define VIN_PLUS_CH0                     (0x0 << 4) // Default: CH0

#define VIN_MINUS_VCM                    (0xF << 0) // Internal VCM
#define VIN_MINUS_TEMP_M                 (0xE << 0) // Internal temperature sensor diode M
#define VIN_MINUS_TEMP_P                 (0xD << 0) // Internal temperature sensor diode P
#define VIN_MINUS_REFINM                 (0xC << 0) // REFIN-
#define VIN_MINUS_REFINP                 (0xB << 0) // REFIN+/OUT
// 0xA << 4 is reserved; do not use
#define VIN_MINUS_AVDD                   (0x9 << 0) // AVDD
#define VIN_MINUS_AGND                   (0x8 << 0) // AGND
#define VIN_MINUS_CH7                    (0x7 << 0) // CH7
#define VIN_MINUS_CH6                    (0x6 << 0) // CH6
#define VIN_MINUS_CH5                    (0x5 << 0) // CH5
#define VIN_MINUS_CH4                    (0x4 << 0) // CH4
#define VIN_MINUS_CH3                    (0x3 << 0) // CH3
#define VIN_MINUS_CH2                    (0x2 << 0) // CH2
#define VIN_MINUS_CH1                    (0x1 << 0) // CH1
#define VIN_MINUS_CH0                    (0x0 << 0) // Default: CH0
// See datasheet notes p. 96 about temperature sensor diode readings and floating inputs

/*
 * SCAN Register field values
 * Byte 2
 * Note that bit 4 is reserved and should be set to zero, and bits 0:3 are unimplemented and
 * will read as zero.
 */
#define MCP3564_SCAN_REG_DLY_512         (0x7 << 5) // 512 * DMCLK
#define MCP3564_SCAN_REG_DLY_256         (0x6 << 5) // 256 * DMCLK
#define MCP3564_SCAN_REG_DLY_128         (0x5 << 5) // 128 * DMCLK
#define MCP3564_SCAN_REG_DLY_64          (0x4 << 5) // 64 * DMCLK
#define MCP3564_SCAN_REG_DLY_32          (0x3 << 5) // 32 * DMCLK
#define MCP3564_SCAN_REG_DLY_16          (0x2 << 5) // 16 * DMCLK
#define MCP3564_SCAN_REG_DLY_8           (0x1 << 5) // 8 * DMCLK
#define MCP3564_SCAN_REG_DLY_0           (0x0 << 5) // Default: no delay

/*
 * Byte 1
 */
#define MCP3564_SCAN_REG_OFFSET_MASK     (0x1 << 7)
#define MCP3564_SCAN_REG_VCM_MASK        (0x1 << 6)
#define MCP3564_SCAN_REG_AVDD_MASK       (0x1 << 5)
#define MCP3564_SCAN_REG_TEMP_MASK       (0x1 << 4)
#define MCP3564_SCAN_REG_DIFF_CH_MASK    (0xF << 3)

/*
 * Byte 0
 */
#define MCP3564_SCAN_REG_SCAN_SE_CH_MASK (0xFF)

/*
 * TIMER Register
 *
 * The TIMER register holds a 24-bit TIMER_SCAN value between two consecutive SCAN cycles
 * when CONV_MODE[1:0] = 0b11. The value is in DMCLK periods. Default value is zero.
 */

/*
 * OFFSETCAL Register
 *
 * The OFFSETCAL register holds a 24-bit OFFSETCAL value in two's complement form.
 */

/*
 * GAINCAL Register
 *
 * The GAINCAL register holds a 24-bit unsigned gain value. The default value 0x800000
 * provides a gain of 1x.
 */

/*
 * LOCK Register
 *
 * The LOCK register will lock out write access to any register except itself when set
 * to a value other than 0xAF. See datasheet for notes on how CRC works with this register.
 */

/*
 * CRCCFG Register
 *
 * The CRCCFG register holds a checksum that is continuously calculated internally based
 * on the register map configuration settings when the device is locked (LOCK[0..7] != 0xA5).
 */

/*
 * RESERVED Registers
 *
 * The 24-bit register at address 0xB is reserved and should be set to 0x900000 (which is
 * default). We just leave it alone.
 *
 * The 8-bit register at address 0xC is reserved and should be set to 0x30 (which is default).
 * We just leave it alone.
 *
 * The 8-bit register at address 0xE is reserved and should be set to:
 * 0xC for MCP3561R
 * 0xD for MCP3562R
 * 0xF for MCP3564R
 * These are default values. We just leave this register alone.
 */

#define MCP3564R_FAST_CMD_NUM_BYTES               1
/*
 * Byte counts for different types of register write commands
 */
#define MCP3564R_REG_ADCDATA_READ_CMD_NUM_BYTES   5
#define MCP3564R_REG_CONFIG0_R_OR_W_CMD_NUM_BYTES 2
#define MCP3564R_REG_CONFIG1_R_OR_W_CMD_NUM_BYTES 2
#define MCP3564R_REG_CONFIG2_R_OR_W_CMD_NUM_BYTES 2
#define MCP3564R_REG_CONFIG3_R_OR_W_CMD_NUM_BYTES 2
#define MCP3564R_REG_IRQ_R_OR_W_CMD_NUM_BYTES     2
#define MCP3564R_REG_MUX_R_OR_W_CMD_NUM_BYTES     2
#define MCP3564R_REG_SCAN_R_OR_W_CMD_NUM_BYTES    4
#define MCP3564R_REG_LOCK_R_OR_W_CMD_NUM_BYTES    2

#define MCP3564_INT_VREF_VOLTAGE_F 2.4F

#endif /* INC_FREERTOS_TASKS_ADC_MCP3564R_COMMON_DEFS_H_ */
