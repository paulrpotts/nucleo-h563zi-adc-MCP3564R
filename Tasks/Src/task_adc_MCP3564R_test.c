/*
 * task_adc_MCP3564R_test.c
 *
 *  Created on: Feb 12, 2026
 *      Author: paul
 */

/*
 * Peripherals
 */
#include "spi.h"
/*
 * FreeRTOS
 */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "event_groups.h"

/*
 * Application
 */
#include "adc_MCP3564R_spi_peripheral.h"
#include "adc_MCP3564R_config.h"


/*
 * Common definitions derived from the part datasheet
 */
#include "adc_MCP3564R_common_defs.h"

/*
 * Tasks
 */
#include "task_adc_MCP3564R_test.h"

/*
 * C Standard Library
 */

/*
 * Needed for memcmp()
 */
#include <string.h>

/**********************************************************************************************************************/
/* PREPROCESSOR DEFINES ***********************************************************************************************/
/**********************************************************************************************************************/

/*
 * Include externally set #defines useful for keeping our application-specific choices separate from this common
 * task code.
 */

#if (MCP3564_USE_EXT_VREF == 1)
#include "adc_MCP3564R_external_vref.h"
#else
#endif


/**********************************************************************************************************************/
/* FILE-SCOPE VARIABLES ***********************************************************************************************/
/**********************************************************************************************************************/
TaskHandle_t adc_read_test_task_h = NULL;
QueueHandle_t adc_read_test_task_can_run_q = NULL;

/*
 * The 16-value limit comes from the range of channels than can be reported in the 4-bit ADCDATA field.
 */
static QueueHandle_t adc_value_queues_a[16] = {NULL};

/*
 * We only use one of these arrays at a time so I can save a few bytes by using a union
 */
typedef union readback_buf_u_t {
    uint8_t adcdata_rb[MCP3564R_REG_ADCDATA_READ_CMD_NUM_BYTES];
    uint8_t config0_rb[MCP3564R_REG_CONFIG0_R_OR_W_CMD_NUM_BYTES];
    uint8_t config1_rb[MCP3564R_REG_CONFIG1_R_OR_W_CMD_NUM_BYTES];
    uint8_t config2_rb[MCP3564R_REG_CONFIG2_R_OR_W_CMD_NUM_BYTES];
    uint8_t config3_rb[MCP3564R_REG_CONFIG3_R_OR_W_CMD_NUM_BYTES];
    uint8_t mux_rb[MCP3564R_REG_MUX_R_OR_W_CMD_NUM_BYTES];
    uint8_t scan_rb[MCP3564R_REG_SCAN_R_OR_W_CMD_NUM_BYTES];
    uint8_t irq_rb[MCP3564R_REG_IRQ_R_OR_W_CMD_NUM_BYTES];
    uint8_t lock_rb[MCP3564R_REG_LOCK_R_OR_W_CMD_NUM_BYTES];
} readback_buf_t;

static readback_buf_t rb_bufs_u;


/*
 * A full reset command
 */
static const uint8_t full_reset[1] = {MCP3564R_FULL_RESET_CMD};

/*
 * One-shot conversion start
 */
static const uint8_t conv_start[1] = {MCP3564R_CONV_START_CMD};

/*
 * Register access commands
 */
static const uint8_t adcdata_read[MCP3564R_REG_ADCDATA_READ_CMD_NUM_BYTES] = {MCP3564_GEN_INC_READ_CMD(MCP3564_REG_ADCDATA),
                                                                              0xFF, 0xFF, 0xFF, 0xFF};

/*
 * Leave settings default when possible.
 *
 * CONFIG 0
 *
 */
#ifndef MCP3564_USE_EXT_VREF
#error "MCP3564_USE_EXT_VREF must be defined"
#endif

#if (MCP3564_USE_EXT_VREF == 1)
#define MCP3564_CONFIG0_VREF_BITS MCP3564_CONFIG0_VREF_SEL_EXT
#else
#define MCP3564_CONFIG0_VREF_BITS MCP3564_CONFIG0_VREF_SEL_INT
#endif

#ifndef MCP3564_USE_EXT_CLOCK
#error "MCP3564_USE_EXT_CLOCK must be defined"
#endif

#if (MCP3564_USE_EXT_CLOCK == 1)
#define MCP3564_CONFIG0_CLOCK_BITS MCP3546_CONFIG0_CLK_EXT
#else
#define MCP3564_CONFIG0_CLOCK_BITS MCP3564_CONFIG0_CLK_INT_PIN_OFF
/*
 * NOTE: for testing purposes, we can also use the MCP3564_CONFIG0_CLK_INT_PIN_ON value defined above which will put the clock signal
 * on the MCLK pin.
 */
#endif

static const uint8_t config0_write[MCP3564R_REG_CONFIG0_R_OR_W_CMD_NUM_BYTES] = {MCP3564_GEN_INC_WRITE_CMD(MCP3564_REG_CONFIG0),
                                                                                 MCP3564_CONFIG0_VREF_BITS  | MCP3564_CONFIG0_PART_SHUTDOWN_N |
                                                                                 MCP3564_CONFIG0_CLOCK_BITS | MCP3564_CONFIG0_ADC_MODE_STBY};
static const uint8_t config0_read[MCP3564R_REG_CONFIG0_R_OR_W_CMD_NUM_BYTES]  = {MCP3564_GEN_INC_READ_CMD(MCP3564_REG_CONFIG0), 0xFF};

/*
 * CONFIG 1
 *
 * These register settings are the same as the default, so don't normally need to be applied, but I've broken them out here so we can
 * easily experiment with changing the OSR (oversampling ratio). This makes a dramatic change to the time required to complete each
 * conversion either in MUX or SCAN mode.
 */
static const uint8_t config1_write[MCP3564R_REG_CONFIG1_R_OR_W_CMD_NUM_BYTES] = {MCP3564_GEN_INC_WRITE_CMD(MCP3564_REG_CONFIG1),
                                                                                 MCP3564_CONFIG1_AMCLK_PRE_DIV_1 | MCP3564_CONFIG1_OVERSAMP_256 |
                                                                                 MCP3564_CONFIG1_RESERVED_BITS};
static const uint8_t config1_read[MCP3564R_REG_CONFIG1_R_OR_W_CMD_NUM_BYTES]  = {MCP3564_GEN_INC_READ_CMD(MCP3564_REG_CONFIG1), 0xFF};

/*
 * CONFIG 2
 *
 * Controls boost, gain, auto-zero algorithm, and auto-zero reference buffer setting
 */
#if MCP3564_ENABLE_AUTOZERO_ALGO
static const uint8_t config2_write[MCP3564R_REG_CONFIG2_R_OR_W_CMD_NUM_BYTES] = {MCP3564_GEN_INC_WRITE_CMD(MCP3564_REG_CONFIG2),
                                                                                 MCP3564_CONFIG2_BIAS_CUR_1X      | MCP3564_CONFIG2_GAIN_X1       |
                                                                                 MCP3564_CONFIG2_AUTOZERO_MUX_OFF | MCP3564_CONFIG2_AUTOZERO_CHOP |
                                                                                 MCP3564_CONFIG2_RESERVED_BITS};
static const uint8_t config2_read[MCP3564R_REG_CONFIG1_R_OR_W_CMD_NUM_BYTES]  = {MCP3564_GEN_INC_READ_CMD(MCP3564_REG_CONFIG2), 0xFF};
#else
/*
 * We don't need to write this register at all since bias BOOST[0:1] 0b10, GAIN[2:0] 0b001, AZ_MUX 0, and AZ REF 0b0 are default settings
 */
#endif

/*
 * CONFIG 3
 *
 * Everything can stay default except:
 *
 * Turn on 32-bit output with channel prefix.
 *
 * If we're in MUX mode, set CONV_MODE to 0b10 which sets mode to standby at the end of the conversion
 * instead of default shutdown state.
 *
 * If we're in SCAN mode, use continuous SCAN with TIMER[23:0] delays between each cycle.
 */
#if (MCP3564_USE_MODE == MCP3564_USE_MUX_MODE)
static const uint8_t config3_write[MCP3564R_REG_CONFIG3_R_OR_W_CMD_NUM_BYTES] = {MCP3564_GEN_INC_WRITE_CMD(MCP3564_REG_CONFIG3),
                                                                                 MCP3564_CONFIG3_CNV_MODE_1STBY   | MCP3564_CONFIG3_DATA_FMT_32_CHAN |
                                                                                 MCP3564_CONFIG3_CRCCOM_DISABLED  | MCP3564_CONFIG3_OFFCAL_DISABLED  |
                                                                                 MCP3564_CONFIG3_GAINCAL_DISABLED};
#elif (MCP3564_USE_MODE == MCP3564_USE_SCAN_MODE)
static const uint8_t config3_write[MCP3564R_REG_CONFIG3_R_OR_W_CMD_NUM_BYTES] = {MCP3564_GEN_INC_WRITE_CMD(MCP3564_REG_CONFIG3),
                                                                                 MCP3564_CONFIG3_CNV_MODE_CONT    | MCP3564_CONFIG3_DATA_FMT_32_CHAN |
                                                                                 MCP3564_CONFIG3_CRCCOM_DISABLED  | MCP3564_CONFIG3_OFFCAL_DISABLED  |
                                                                                 MCP3564_CONFIG3_GAINCAL_DISABLED};
#else
#error "Unexpected MCP3564_USE_MODE"
#endif

static const uint8_t config3_read[MCP3564R_REG_CONFIG3_R_OR_W_CMD_NUM_BYTES] = {MCP3564_GEN_INC_READ_CMD(MCP3564_REG_CONFIG3), 0xFF};

#if (MCP3564_USE_MODE == MCP3564_USE_MUX_MODE)
/*
 * MUX
 */

#if (MCP3564_INPUTS_TYPE == MCP3564_INPUTS_SE)
/*
* For single-ended readings, use VIN_MINUS_AGND in the MUX_VIN- bits. We have 8 predefined commands for our 8 single-ended inputs.
*/
#define MCP3564_MUX_TEST_NUM_INPUTS 8

static const uint8_t mux_write_a[MCP3564_MUX_TEST_NUM_INPUTS][MCP3564R_REG_MUX_R_OR_W_CMD_NUM_BYTES] = {
    {MCP3564_GEN_INC_WRITE_CMD(MCP3564_REG_MUX), VIN_PLUS_CH0 | VIN_MINUS_AGND},
    {MCP3564_GEN_INC_WRITE_CMD(MCP3564_REG_MUX), VIN_PLUS_CH1 | VIN_MINUS_AGND},
    {MCP3564_GEN_INC_WRITE_CMD(MCP3564_REG_MUX), VIN_PLUS_CH2 | VIN_MINUS_AGND},
    {MCP3564_GEN_INC_WRITE_CMD(MCP3564_REG_MUX), VIN_PLUS_CH3 | VIN_MINUS_AGND},
    {MCP3564_GEN_INC_WRITE_CMD(MCP3564_REG_MUX), VIN_PLUS_CH4 | VIN_MINUS_AGND},
    {MCP3564_GEN_INC_WRITE_CMD(MCP3564_REG_MUX), VIN_PLUS_CH5 | VIN_MINUS_AGND},
    {MCP3564_GEN_INC_WRITE_CMD(MCP3564_REG_MUX), VIN_PLUS_CH6 | VIN_MINUS_AGND},
    {MCP3564_GEN_INC_WRITE_CMD(MCP3564_REG_MUX), VIN_PLUS_CH7 | VIN_MINUS_AGND}
};
#elif (MCP3564_INPUTS_TYPE == MCP3564_INPUTS_DE)
/*
* For double-ended readings, pair even inputs with odd inputs (see datasheet p. 62). We have 4 predefined commands for our 4 double-
* ended inputs.
*/
#define MCP3564_MUX_TEST_NUM_INPUTS 4

static const uint8_t mux_write_a[MCP3564_MUX_TEST_NUM_INPUTS][MCP3564R_REG_MUX_R_OR_W_CMD_NUM_BYTES] = {
    {MCP3564_GEN_INC_WRITE_CMD(MCP3564_REG_MUX), VIN_PLUS_CH0 | VIN_PLUS_CH1},
    {MCP3564_GEN_INC_WRITE_CMD(MCP3564_REG_MUX), VIN_PLUS_CH2 | VIN_PLUS_CH3},
    {MCP3564_GEN_INC_WRITE_CMD(MCP3564_REG_MUX), VIN_PLUS_CH4 | VIN_PLUS_CH5},
    {MCP3564_GEN_INC_WRITE_CMD(MCP3564_REG_MUX), VIN_PLUS_CH6 | VIN_PLUS_CH7}
};
#else
#error "Unexpected MCP3564_INPUTS_TYPE"
#endif
#elif (MCP3564_USE_MODE == MCP3564_USE_SCAN_MODE)
/*
 * SCAN
 */
static const uint8_t scan_read[MCP3564R_REG_SCAN_R_OR_W_CMD_NUM_BYTES] = {MCP3564_GEN_INC_READ_CMD(MCP3564_REG_SCAN), 0xFF, 0xFF, 0xFF};

#if (MCP3564_INPUTS_TYPE == MCP3564_INPUTS_SE)
/*
* For single-ended readings, we use all 8 specified in SCAN register bits 7:0.
*/
#define MCP3564_SCAN_TEST_NUM_INPUTS 8

static const uint8_t scan_write[MCP3564R_REG_SCAN_R_OR_W_CMD_NUM_BYTES] = {MCP3564_GEN_INC_WRITE_CMD(MCP3564_REG_SCAN), 0x00, 0x00, 0xFF};
#elif (MCP3564_INPUTS_TYPE == MCP3564_INPUTS_DE)
/*
* For double-ended readings, we use the four even/odd differential channels numbered 0xA..0xD that we can specify in bits 11:8
*/
#define MCP3564_SCAN_TEST_NUM_INPUTS 4

static const uint8_t scan_write[MCP3564R_REG_SCAN_R_OR_W_CMD_NUM_BYTES] = {MCP3564_GEN_INC_WRITE_CMD(MCP3564_REG_SCAN), 0x00, 0x0F, 0x00};
#else
#error "Unexpected MCP3564_INPUTS_TYPE"
#endif
#else
#error "Unexpected MCP3564_USE_MODE"
#endif

#if MCP3564_USE_CONV_COMPLETE_INT
/*
 * IRQ
 */
static const uint8_t irq_write[MCP3564R_REG_IRQ_R_OR_W_CMD_NUM_BYTES] = {MCP3564_GEN_INC_WRITE_CMD(MCP3564_REG_IRQ),
                                                                         MCP3564_IRQ_REG_IRQ_MD_INTS | MCP3564_IRQ_REG_IRQ_MD_IDLE_HI |
                                                                         MCP3564_IRQ_REG_EN_FASTCMD  | MCP3564_IRQ_REG_DIS_STP};
static const uint8_t irq_read[MCP3564R_REG_IRQ_R_OR_W_CMD_NUM_BYTES]  = {MCP3564_GEN_INC_READ_CMD(MCP3564_REG_IRQ), 0xFF};
#else
#error "MCP3564_USE_CONV_COMPLETE_INT should be set to 1. This code doesn't currently support polling for conversion completion."
#endif

/*
 * Writing 0xA5 to the lock register lets us write to registers. Writing any other value locks us out. Break this into separate unlock
 * and lock commands to match the way we want to use it.
 */
static const uint8_t lock_write_unlock[MCP3564R_REG_LOCK_R_OR_W_CMD_NUM_BYTES] = {MCP3564_GEN_INC_WRITE_CMD(MCP3564_REG_LOCK), 0xA5};

#if (MCP3564_USE_MODE == MCP3564_USE_SCAN_MODE)
    static const uint8_t lock_write_lock[MCP3564R_REG_LOCK_R_OR_W_CMD_NUM_BYTES] = {MCP3564_GEN_INC_WRITE_CMD(MCP3564_REG_LOCK), 0xFF};
#else
    /*
     * If we're going to use MUX instead of SCAN, we don't want to actually lock the registers, because we're going to need to write a
     * new configuration to the MUX register for each conversion. Enforce this by not defining lock_write_lock so we get a build error
     * if the code tries it.
     */
#endif

static const uint8_t lock_read[MCP3564R_REG_LOCK_R_OR_W_CMD_NUM_BYTES] = {MCP3564_GEN_INC_READ_CMD(MCP3564_REG_LOCK), 0xFF};

#if MCP3564_DBG_COUNT_CHAN_READS
/*
 * Define counters for keeping track of how many times each channel has been successfully read.
 */
uint32_t scan_chan_read_counts_a[ADC_NUM_VOLTAGE_QUEUES] = {0};
#endif

/*
 * For rolling averages
 */
#define ROLLING_AVG_NUM_VALUES 8

static float avg_bufs_a[ADC_NUM_VOLTAGE_QUEUES][ROLLING_AVG_NUM_VALUES] = {{0.0F}};
static uint8_t avg_buf_indices_a[ADC_NUM_VOLTAGE_QUEUES] = {0};
static uint8_t avg_buf_counts_a[ADC_NUM_VOLTAGE_QUEUES]  = {0};

/**********************************************************************************************************************/
/* PRIVATE FUNCTION PROTOTYPES*****************************************************************************************/
/**********************************************************************************************************************/
static uint8_t write_register(const uint8_t * reg_write_cmd_p, uint8_t num_bytes);
static uint8_t write_register_and_verify(const uint8_t * reg_write_cmd_p, const uint8_t * const reg_read_cmd_p,
                                         uint8_t * const readback_buf_p, uint8_t num_bytes);
static uint8_t write_irq_reg_and_verify(const uint8_t * reg_write_cmd_p, const uint8_t * const reg_read_cmd_p,
                                        uint8_t * const readback_buf_p, uint8_t num_bytes);
static void adc_configure(void);
static float get_rolling_average(uint8_t chan_idx, int32_t adc_val_i, float gain);

/**********************************************************************************************************************/
/* PRIVATE FUNCTIONS **************************************************************************************************/
/**********************************************************************************************************************/
static uint8_t write_register(const uint8_t * reg_write_cmd_p, uint8_t num_bytes)
{
    HAL_StatusTypeDef hal_status;
    BaseType_t wait_ret_val;
    uint32_t notify_bits;
    uint8_t ret_val = 0; /* Failure */

#if (MCP3564_HAL_SPI_MODE == MCP3564_USE_HAL_SPI_DMA_MODE)
    hal_status = HAL_SPI_Transmit_DMA(&ADC_SPI_PERIPH_HANDLE, reg_write_cmd_p, num_bytes);
#elif (MCP3564_HAL_SPI_MODE == MCP3564_USE_HAL_SPI_INT_MODE)
    hal_status = HAL_SPI_Transmit_IT(&ADC_SPI_PERIPH_HANDLE, reg_write_cmd_p, num_bytes);
#else
#error "unexpected MCP3564_HAL_SPI_MODE"
#endif
    if (HAL_OK == hal_status)
    {
        wait_ret_val = xTaskNotifyWait(0x0, ADC_TASK_NOTIFY_TX_COMPLETE, &notify_bits,
                                       pdMS_TO_TICKS(ADC_HAL_SPI_TIMEOUT_MSEC));
        if ((pdTRUE == wait_ret_val) && (notify_bits & ADC_TASK_NOTIFY_TX_COMPLETE))
        {
            ret_val = 1;
        }
        else
        {
            ret_val = 0;
        }
    }

    return ret_val;
}

static uint8_t write_register_and_verify(const uint8_t * reg_write_cmd_p, const uint8_t * const reg_read_cmd_p,
                                         uint8_t * const readback_buf_p, uint8_t num_bytes)
{
    HAL_StatusTypeDef hal_status;
    BaseType_t wait_ret_val;
    uint32_t notify_bits;
    uint8_t ret_val = 0; /* Failure */

    if (1 == write_register(reg_write_cmd_p, num_bytes))
    {
        /*
         * The register write succeeded, so read it back
         */
#if (MCP3564_HAL_SPI_MODE == MCP3564_USE_HAL_SPI_DMA_MODE)
        hal_status = HAL_SPI_TransmitReceive_DMA(&ADC_SPI_PERIPH_HANDLE, reg_read_cmd_p, readback_buf_p, num_bytes);
#elif (MCP3564_HAL_SPI_MODE == MCP3564_USE_HAL_SPI_INT_MODE)
        hal_status = HAL_SPI_TransmitReceive_IT(&ADC_SPI_PERIPH_HANDLE, reg_read_cmd_p, readback_buf_p, num_bytes);
#else
#error "unexpected MCP3564_HAL_SPI_MODE"
#endif
        if (HAL_OK == hal_status)
        {
            /*
             * Wait for DMA completion
             */
            wait_ret_val = xTaskNotifyWait(0x0, ADC_TASK_NOTIFY_TX_RX_COMPLETE, &notify_bits,
                                           pdMS_TO_TICKS(ADC_HAL_SPI_TIMEOUT_MSEC));
            if ((pdTRUE == wait_ret_val) && (notify_bits & ADC_TASK_NOTIFY_TX_RX_COMPLETE))
            {
                /*
                 * The first byte will not match, because when we send any command, the first byte on MISO
                 * contains a status byte, for polling applications. Our implementation does not use this;
                 * we use the interrupt pin instead.
                 */
                if (0 == memcmp((const void *)(reg_write_cmd_p + 1), (const void *)(readback_buf_p + 1),
                                num_bytes - 1))
                {
                    ret_val = 1; /* Success */
                }
                else
                {
                    ret_val = 0;
                }
            }
            else
            {
                ret_val = 0;
            }
        }
    }

    return ret_val;
}

/*
 * The IRQ register contains status flags in bits 6:4 which are readable only. Therefore, I do not set these in the command to initialize
 * the IRQ register, but we do see them set (indicating the flags are inactive) when we read the register back. This means I can't use my
 * standard write_register_and_verify() function above, or we'll get a failure to match when we compare the read-back value with the write
 * command. To work around this, here is a version of the function that only checks bits 3:0 of the register value.
 */
static uint8_t write_irq_reg_and_verify(const uint8_t * reg_write_cmd_p, const uint8_t * const reg_read_cmd_p,
                                        uint8_t * const readback_buf_p, uint8_t num_bytes)
{
    HAL_StatusTypeDef hal_status;
    BaseType_t wait_ret_val;
    uint32_t notify_bits;
    uint8_t ret_val = 0; /* Failure */

    if (1 == write_register(reg_write_cmd_p, num_bytes))
    {
#if (MCP3564_HAL_SPI_MODE == MCP3564_USE_HAL_SPI_DMA_MODE)
        hal_status = HAL_SPI_TransmitReceive_DMA(&ADC_SPI_PERIPH_HANDLE, reg_read_cmd_p, readback_buf_p, num_bytes);
#elif (MCP3564_HAL_SPI_MODE == MCP3564_USE_HAL_SPI_INT_MODE)
        hal_status = HAL_SPI_TransmitReceive_IT(&ADC_SPI_PERIPH_HANDLE, reg_read_cmd_p, readback_buf_p, num_bytes);
#else
#error "unexpected MCP3564_HAL_SPI_MODE"
#endif
        if (HAL_OK == hal_status)
        {
            wait_ret_val = xTaskNotifyWait(0x0, ADC_TASK_NOTIFY_TX_RX_COMPLETE, &notify_bits,
                                           pdMS_TO_TICKS(ADC_HAL_SPI_TIMEOUT_MSEC));
            if ((pdTRUE == wait_ret_val) && (notify_bits & ADC_TASK_NOTIFY_TX_RX_COMPLETE))
            {
                if ((*(reg_write_cmd_p + 1) & 0xF) == (*(readback_buf_p + 1) & 0xF))
                {
                    ret_val = 1;
                }
            }
        }
    }

    return ret_val;
}

/*
 * Note that this function does not return an error code, but triggers an assertion failure if communication with the ADC is not
 * working reliably.
 */
void adc_configure(void)
{
    uint8_t try_count;
    uint8_t configuration_verified = 0;

    /*
     * Sometimes we have a pending conversion complete notification. Clear all our notifications before we start our initial
     * configuration of the ADC.
     */
    (void)ulTaskNotifyValueClear(NULL, ADC_TASK_ALL_NOTIFICATIONS);

    /*
     * Try the initialization sequence up to three times. If we get a HAL error or fail to read back what we wrote to a register,
     * restart the sequence from the top. If we manage to get through the whole sequence, set our success flag and break out of
     * the loop.
     */
    for (try_count = 1; try_count <= 3; try_count += 1)
    {
        /*
         * Unlock registers and read back the lock register
         */
        if (0 == write_register_and_verify(lock_write_unlock, lock_read, rb_bufs_u.lock_rb, MCP3564R_REG_LOCK_R_OR_W_CMD_NUM_BYTES))
        {
            continue;
        }
        /*
         * Issue the fast command to reset the part
         */
        if (0 == write_register(full_reset, MCP3564R_FAST_CMD_NUM_BYTES))
        {
            continue;
        }
        /*
         * Write and read back CONFIG registers
         */
        if (0 == write_register_and_verify(config0_write, config0_read, rb_bufs_u.config0_rb, MCP3564R_REG_CONFIG0_R_OR_W_CMD_NUM_BYTES))
        {
            continue;
        }

        if (0 == write_register_and_verify(config1_write, config1_read, rb_bufs_u.config1_rb, MCP3564R_REG_CONFIG1_R_OR_W_CMD_NUM_BYTES))
        {
            continue;
        }

#if MCP3564_ENABLE_AUTOZERO_ALGO
        if (0 == write_register_and_verify(config2_write, config2_read, rb_bufs_u.config2_rb, MCP3564R_REG_CONFIG2_R_OR_W_CMD_NUM_BYTES))
        {
            continue;
        }
#endif

        if (0 == write_register_and_verify(config3_write, config3_read, rb_bufs_u.config3_rb, MCP3564R_REG_CONFIG3_R_OR_W_CMD_NUM_BYTES))
        {
            continue;
        }

        /*
         * Due to the read-only flags in the interrupt register, we can't use our regular write_register_and_verify() function.
         * Use a separate function that compares only the the bits we expect to see changed.
         */
        if (0  == write_irq_reg_and_verify(irq_write, irq_read, rb_bufs_u.irq_rb, MCP3564R_REG_IRQ_R_OR_W_CMD_NUM_BYTES))
        {
            continue;
        }

        /*
         * Sometimes the conversion complete interrupt has fired before we actually start conversion, possibly when we write to the IRQ
         * register. Clear this bit for good measure so we don't get a premature exit when we wait for the bit to be set upon completion
         * of each conversion.
         */
        (void)ulTaskNotifyValueClear(NULL, ADC_TASK_NOTIFY_CONV_COMPLETE);

#if (MCP3564_USE_MODE == MCP3564_USE_MUX_MODE)
        /*
         * For MUX mode, we don't write the MUX register here. It is written for each conversion.
         */
#elif (MCP3564_USE_MODE == MCP3564_USE_SCAN_MODE)
        if (0 == write_register_and_verify(scan_write, scan_read, rb_bufs_u.scan_rb, MCP3564R_REG_SCAN_R_OR_W_CMD_NUM_BYTES))
        {
            continue;
        }

        /*
         * In scan mode, lock the registers, since we won't need to write to the MUX register to configure each conversion.
         */
        if (0 == write_register_and_verify(lock_write_lock, lock_read, rb_bufs_u.lock_rb, MCP3564R_REG_LOCK_R_OR_W_CMD_NUM_BYTES))
        {
            continue;
        }
#else
#error "Unexpected MCP3564_USE_MODE"
#endif

        configuration_verified = 1;
        break;
    }

    /*
     * If we fell out of the loop without having set our configuration_verified flag, that means we didn't successfully
     * write and read back all the registers we need, so there's a serious communication problem and we don't want the
     * code to continue.
     */
    assert(1 == configuration_verified);
}

float get_rolling_average(uint8_t chan_idx, int32_t adc_val_i, float gain)
{
    float sum_f = 0;
    float avg_val_f;
    uint8_t val_idx;

    avg_bufs_a[chan_idx][avg_buf_indices_a[chan_idx]] = (float)adc_val_i;
    avg_buf_indices_a[chan_idx] = (avg_buf_indices_a[chan_idx] + 1) % ROLLING_AVG_NUM_VALUES;

    if (avg_buf_counts_a[chan_idx] < ROLLING_AVG_NUM_VALUES)
    {
        avg_buf_counts_a[chan_idx] += 1;
    }

    for (val_idx = 0; val_idx < avg_buf_counts_a[chan_idx]; val_idx += 1)
    {
        sum_f += avg_bufs_a[chan_idx][val_idx];
    }

    avg_val_f = sum_f / (float)avg_buf_counts_a[chan_idx];

    /*
     * Scale our averaged integer value such that 2^23 maps to our nominal VREF, applying the gain. The gain should
     * match the ADC gain setting that was in effect when the reading was taken.
     *
     * See equation 5-5 in the datasheet:
     *
     * ADC_OUTPUT(LSb) = ( (Vin+ - Vin-) / (Vref+ - Vref-) * 8,388,608 * GAIN
     *
     * Where ADC output is in LSb units.
     *
     * Solving for (Vin+ - Vin-) we get (Vin+ - Vin-) = ADC_OUTPUT(LSb) * (Vref+ - Vref-) / 8388608.0F / gain
     *
     * See the datasheet for details on the 24-bit and 25-bit encodings. When we decode the ADC output into an int32,
     * it should always include sign extension, so this math should work with either encoding.
     */
#if (MCP3564_USE_EXT_VREF == 1)
    return avg_val_f * MCP3564_EXT_VREF_VOLTAGE_F / 8388608.0F / gain;
#else
    return avg_val_f * MCP3564_INT_VREF_VOLTAGE_F / 8388608.0F / gain;
#endif
}

/**********************************************************************************************************************/
/* PUBLIC FUNCTIONS ***************************************************************************************************/
/**********************************************************************************************************************/
void adc_read_test_task_setup(void)
{
    uint8_t q_idx;
    BaseType_t base_q_val = 0;

    /*
     * Set up single-element queues for consumers of our voltage values. For testing purposes, we use 8 single-ended
     * readings or 4 double-ended readings. I have seen odd things happen, like in SCAN mode where only channels with
     * indices 0x8 through 0xB should be reported, on the first conversion, we get index 0. We also may want to test
     * the special TEMP, AVDD, VCM, and OFFSET channel later, so just make queues for every reportable source.
     */
    for (q_idx = 0; q_idx < ADC_NUM_VOLTAGE_QUEUES; q_idx += 1)
    {
        QueueHandle_t adc_value_q = xQueueCreate(1, sizeof(float));
        assert (NULL != adc_value_q);
        adc_value_queues_a[q_idx] = adc_value_q;
    }

    adc_read_test_task_can_run_q = xQueueCreate(1, sizeof(BaseType_t));
    assert(NULL != adc_read_test_task_can_run_q);

    /*
     * The ADC task will run unless stopped
     */
    xQueueOverwrite(adc_read_test_task_can_run_q, &base_q_val);
}

void adc_read_test_task(void *argument)
{
    BaseType_t wait_ret_val;
    BaseType_t task_can_run_q_val;
    uint32_t notify_bits;
    HAL_StatusTypeDef hal_status;

    /*
     * I've seen cases where pending notifications are a problem, possibly due to the interrupt pin from the ADC
     * going low and triggering our external interrupt handler. Clear everything so the task starts fresh.
     */
    xTaskNotifyStateClear(NULL);
    ulTaskNotifyValueClear(NULL, 0xFFFFFFFF);


    /*
     * Optionally enforce a minimum task delay, if we need to slow down the task, rather than just waiting on HAL
     * SPI API calls and waits for conversion to slow us down.
     */
#if 0
    const TickType_t freq_ticks = pdMS_TO_TICKS(ADC_TASK_DELAY_MSEC);
#endif

    /*
     * This call may throw an assert!
     */
    adc_configure();

/*
 * Scan mode and mux mode represent two ways to drive the ADC for testing: in mux mode, we do individual one-shot
 * conversions. In scan mode we program a series of conversions and let the ADC run, and get an interrupt via a
 * GPIO pin every time a conversion result is ready.
 */
#if (MCP3564_USE_MODE == MCP3564_USE_MUX_MODE)
    while (1)
    {
        /*
         * Clear this queue to block the task. Write to it to allow the task to run.
         */
        (void)xQueuePeek(adc_read_test_task_can_run_q, &task_can_run_q_val, portMAX_DELAY);

        /*
         * In MUX mode we write to the MUX register, trigger a conversion, sleep waiting for the data ready interrupt,
         * then read the ADCData register to get the conversion results. Do this round-robin with our predefined MUX
         * register values to update all voltages.
         */
        uint8_t mux_a_idx;

        for (mux_a_idx = 0; mux_a_idx < MCP3564_MUX_TEST_NUM_INPUTS; mux_a_idx += 1)
        {
#if 0
            TickType_t last_wake_time = xTaskGetTickCount();
#endif

            /*
             * Set up the MUX register
             */
#if (MCP3564_HAL_SPI_MODE == MCP3564_USE_HAL_SPI_DMA_MODE)
            hal_status = HAL_SPI_Transmit_DMA(&ADC_SPI_PERIPH_HANDLE, mux_write_a[mux_a_idx], MCP3564R_REG_MUX_R_OR_W_CMD_NUM_BYTES);
#elif (MCP3564_HAL_SPI_MODE == MCP3564_USE_HAL_SPI_INT_MODE)
            hal_status = HAL_SPI_Transmit_IT(&ADC_SPI_PERIPH_HANDLE, mux_write_a[mux_a_idx], MCP3564R_REG_MUX_R_OR_W_CMD_NUM_BYTES);
#else
#error "unexpected MCP3564_HAL_SPI_MODE"
#endif
            assert(HAL_OK == hal_status);

            /*
             * Wait for DMA completion
             */
            wait_ret_val = xTaskNotifyWait(0x0, ADC_TASK_NOTIFY_TX_COMPLETE, &notify_bits,
                                           pdMS_TO_TICKS(ADC_HAL_SPI_TIMEOUT_MSEC));
            assert ((pdTRUE == wait_ret_val) && (notify_bits & ADC_TASK_NOTIFY_TX_COMPLETE));

            /*
             * Send the start conversion fast command
             */
#if (MCP3564_HAL_SPI_MODE == MCP3564_USE_HAL_SPI_DMA_MODE)
            hal_status = HAL_SPI_Transmit_DMA(&ADC_SPI_PERIPH_HANDLE, conv_start, MCP3564R_FAST_CMD_NUM_BYTES);
#elif (MCP3564_HAL_SPI_MODE == MCP3564_USE_HAL_SPI_INT_MODE)
            hal_status = HAL_SPI_Transmit_IT(&ADC_SPI_PERIPH_HANDLE, conv_start, MCP3564R_FAST_CMD_NUM_BYTES);
#else
#error "unexpected MCP3564_HAL_SPI_MODE"
#endif
            assert(HAL_OK == hal_status);

            /*
             * Wait for both the SPI TX complete and conversion complete interrupts. Event groups have a more
             * convenient way to use "and" logic to wait on both bits, but we have had performance problems
             * with event groups, which require the timer task to complete the notifications, so this will have to do.
             */
            uint32_t collected_notify_bits = 0; /* For waiting on more than one notification */

            do
            {
                /*
                 * Both bits should be received very quickly, so a timeout condition indicates failure
                 */
                wait_ret_val = xTaskNotifyWait(0x0, (ADC_TASK_NOTIFY_TX_COMPLETE | ADC_TASK_NOTIFY_CONV_COMPLETE),
                                               &notify_bits, pdMS_TO_TICKS(ADC_CONV_COMPLETE_TIMEOUT_MSEC));
                assert(pdTRUE == wait_ret_val);
                collected_notify_bits |= notify_bits;
            } while (collected_notify_bits != (ADC_TASK_NOTIFY_TX_COMPLETE | ADC_TASK_NOTIFY_CONV_COMPLETE));

            /*
             * Do our SPI write-to-read to retrieve the conversions results. Note that
             */
#if (MCP3564_HAL_SPI_MODE == MCP3564_USE_HAL_SPI_DMA_MODE)
            hal_status = HAL_SPI_TransmitReceive_DMA(&ADC_SPI_PERIPH_HANDLE, adcdata_read, rb_bufs_u.adcdata_rb,
                                                     MCP3564R_REG_ADCDATA_READ_CMD_NUM_BYTES);
#elif (MCP3564_HAL_SPI_MODE == MCP3564_USE_HAL_SPI_INT_MODE)
            hal_status = HAL_SPI_TransmitReceive_IT(&ADC_SPI_PERIPH_HANDLE, adcdata_read, rb_bufs_u.adcdata_rb,
                                                    MCP3564R_REG_ADCDATA_READ_CMD_NUM_BYTES);
#else
#error "unexpected MCP3564_HAL_SPI_MODE"
#endif
            assert(HAL_OK == hal_status);

            wait_ret_val = xTaskNotifyWait(0x0, ADC_TASK_NOTIFY_TX_RX_COMPLETE, &notify_bits,
                                           pdMS_TO_TICKS(ADC_HAL_SPI_TIMEOUT_MSEC));
            assert ((pdTRUE == wait_ret_val) && (notify_bits & ADC_TASK_NOTIFY_TX_RX_COMPLETE));

            /*
             * See datasheet MCP3561_2_4R-Data-Sheet-DS200006391C.pdf pp. 48-49 to understand the ADC reading representation.
             * The high 4 bits of the MSB of the 4-byte field contain channel index and the low 4 bits contain the sign bit
             * repeated.
             *
             * Following table 5-8, create a 25-bit value including one bit from the sign field as MSb, and then shift this
             * right (it's signed, so sign is maintained) to get a raw signed integer in LSb units.
             */
            int32_t adc_val_i = ((rb_bufs_u.adcdata_rb[1] << 31) |
                                 (rb_bufs_u.adcdata_rb[2] << 23) |
                                 (rb_bufs_u.adcdata_rb[3] << 15) |
                                 (rb_bufs_u.adcdata_rb[4] << 7)    ) >> 7;

            /*
             * Note that in this test mode gain is always 1.0 because that is the default. The gain should match the gain
             * setting that was in effect when the ADC took the reading.
             */
            float average_chan_voltage = get_rolling_average(mux_a_idx, adc_val_i, 1.0F);

            xQueueOverwrite(adc_value_queues_a[mux_a_idx], &average_chan_voltage);

            /*
             * Optional minimal delay; in testing, this is generally not resulting in a wait as we have exceeded the
             * time slice by a wide margin (conversions seem to take about 1.45ms in one-shot mode))
             */
#if 0
            vTaskDelayUntil(&last_wake_time, freq_ticks);
#endif
        } /* Loop on channels */
    } /* Loop forever */
#elif (MCP3564_USE_MODE == MCP3564_USE_SCAN_MODE)
    /*
     * In SCAN mode we wrote once to the SCAN register during the setup process, above. Now we kick off the conversions
     * and we should get a data ready interrupt when each one is complete.
     *
     * Send the start conversion fast command.
    */
#if (MCP3564_HAL_SPI_MODE == MCP3564_USE_HAL_SPI_DMA_MODE)
    hal_status = HAL_SPI_Transmit_DMA(&ADC_SPI_PERIPH_HANDLE, conv_start, MCP3564R_FAST_CMD_NUM_BYTES);
#elif (MCP3564_HAL_SPI_MODE == MCP3564_USE_HAL_SPI_INT_MODE)
    hal_status = HAL_SPI_Transmit_IT(&ADC_SPI_PERIPH_HANDLE, conv_start, MCP3564R_FAST_CMD_NUM_BYTES);
#else
#error "unexpected MCP3564_HAL_SPI_MODE"
#endif
    assert(HAL_OK == hal_status);

    /*
     * Wait until the conversion start command is sent
     */
    wait_ret_val = xTaskNotifyWait(0x0, ADC_TASK_NOTIFY_TX_COMPLETE, &notify_bits,
                                   pdMS_TO_TICKS(ADC_HAL_SPI_TIMEOUT_MSEC));
    assert ((pdTRUE == wait_ret_val) && (notify_bits & ADC_TASK_NOTIFY_TX_COMPLETE));

    /*
    * Now just wait for the interrupts and read the values. Use the channel identifier encoded in the ADC data register.
    */
    while (1)
    {
        /*
         * Clear this queue to block the task. Write to it to allow the task to run.
         */
        (void)xQueuePeek(adc_read_test_task_can_run_q, &task_can_run_q_val, portMAX_DELAY);

        wait_ret_val = xTaskNotifyWait(0x0, ADC_TASK_NOTIFY_CONV_COMPLETE, &notify_bits,
                                       pdMS_TO_TICKS(ADC_CONV_COMPLETE_TIMEOUT_MSEC));
        assert((pdTRUE == wait_ret_val) && (notify_bits & ADC_TASK_NOTIFY_CONV_COMPLETE));

        /*
         * Do our SPI write-to-read to retrieve the conversions results
         */
#if (MCP3564_HAL_SPI_MODE == MCP3564_USE_HAL_SPI_DMA_MODE)
        hal_status = HAL_SPI_TransmitReceive_DMA(&ADC_SPI_PERIPH_HANDLE, adcdata_read, rb_bufs_u.adcdata_rb,
                                                 MCP3564R_REG_ADCDATA_READ_CMD_NUM_BYTES);
#elif (MCP3564_HAL_SPI_MODE == MCP3564_USE_HAL_SPI_INT_MODE)
        hal_status = HAL_SPI_TransmitReceive_IT(&ADC_SPI_PERIPH_HANDLE, adcdata_read, rb_bufs_u.adcdata_rb,
                                                MCP3564R_REG_ADCDATA_READ_CMD_NUM_BYTES);
#else
#error "unexpected MCP3564_HAL_SPI_MODE"
#endif
        assert(HAL_OK == hal_status);

        wait_ret_val = xTaskNotifyWait(0x0, ADC_TASK_NOTIFY_TX_RX_COMPLETE, &notify_bits,
                                       pdMS_TO_TICKS(ADC_HAL_SPI_TIMEOUT_MSEC));
        assert ((pdTRUE == wait_ret_val) && (notify_bits & ADC_TASK_NOTIFY_TX_RX_COMPLETE));

        /*
         * NOTE: if we are failing to catch our expected notification bit but we do see a conversion complete
         * bit, it is likely because this task is being preempted so long that the last SPI transmit didn't
         * go out to the hardware, but the ADC completed another conversion and pulled the interrupt pin low
         * again. To fix this, make sure the ADC task priority is higher than the other tasks.
         */

        /*
         * See datasheet MCP3561_2_4R-Data-Sheet-DS200006391C.pdf pp. 48-49 to understand the ADC reading representation.
         * The high 4 bits of the MSB of the 4-byte field contain channel index and the low 4 bits contain the sign bit
         * repeated. We use this representation (DATA_FORMAT[1:0] = 0b11) so we can easily identify the channel that
         * each reading belongs to.
         *
         * Following table 5-8, create a 25-bit value including one bit from the sign field as MSb, and then shift this
         * right to get a raw signed integer in LSb units.
         */
        uint8_t chan_idx = rb_bufs_u.adcdata_rb[1] >> 4;
        int32_t adc_val_i = ((rb_bufs_u.adcdata_rb[1] << 31) |
                             (rb_bufs_u.adcdata_rb[2] << 23) |
                             (rb_bufs_u.adcdata_rb[3] << 15) |
                             (rb_bufs_u.adcdata_rb[4] << 7)    ) >> 7;

        /*
         * The single-ended channel indices are reported in ACDATA as 0..7, following the indices of the bits that
         * enable each channel in the SCAN register. Note that for the differential channels the indices are reported
         * as 0x8..0xB.
         *
         * Note that in the FIRST value returned after we start running in SCAN mode, we can get an index field of zero
         * and an ADC conversion value of zero. This may be a bug in the ADC hardware and it may have to do with a
         * spurious early interrupt that fires when we write the INT register, even though we have not yet triggered a
         * conversion.
         *
         * Note that our standard gain structure just uses the default 1x gain. I've made this a parameter here so you
         * can easily adapt it to handle a different gain. The get_rolling_average() function takes the VREF setting
         * into account.
         */
        float average_chan_voltage = get_rolling_average(chan_idx, adc_val_i, 1.0F);

        xQueueOverwrite(adc_value_queues_a[chan_idx], &average_chan_voltage);

#if MCP3564_DBG_COUNT_CHAN_READS
        scan_chan_read_counts_a[chan_idx] += 1;
#endif

#if 0
        vTaskDelayUntil(&last_wake_time, freq_ticks);
#endif
    } /* Loop forever */
#else
#error "Unexpected MCP3564_USE_MODE"
#endif
}

uint8_t adc_get_v(uint8_t chan_idx, float *voltage_p)
{
    uint8_t ret_val = 0;

    /*
     * Pass zero for xTicksToWait since we don't want to wait at all. Once the ADC task has completed an
     * initial round of conversions, the single-element voltage value queues should always have values in
     * them, overwritten periodically with fresh data.
     */
    if (pdPASS == xQueuePeek(adc_value_queues_a[chan_idx], voltage_p, 0))
    {
        /*
         * Data was found in the single-element queue
         */
        ret_val = 1;
    }

    return ret_val;
}

uint8_t adc_get_v_from_isr(uint8_t chan_idx, float *voltage_p)
{
    uint8_t ret_val = 0;

    /*
     * Read from the queue without blocking.
     */
    if (pdPASS == xQueuePeekFromISR(adc_value_queues_a[chan_idx], voltage_p))
    {
        ret_val = 1;
    }

    return ret_val;
}

/**********************************************************************************************************************/
/* PRIVATE FUNCTIONS **************************************************************************************************/
/**********************************************************************************************************************/

