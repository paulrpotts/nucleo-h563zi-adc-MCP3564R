/*
 * sfp_plus_field_defs.h
 *
 *  Created on: Jun 17, 2026
 *      Author: paul
 */
#include "FreeRTOS.h"
#include "queue.h"

#ifndef INC_SFP_PLUS_FIELD_DEFS_H_
#define INC_SFP_PLUS_FIELD_DEFS_H_

/*
 * Undefined fields cannot be written (we just discard the written data) and when read, always return zero
 */
#define SFP_PLUS_UNDEFINED                 0x0000

/*
 * Const bytes (defined in a const data structure)
 */
#define SFP_PLUS_CONST_BYTE                0x0100

/*
 * Volatile system state, one byte, not backed by the flash, read-only
 */
#define SFP_PLUS_RO_VOL_BYTE               0x4100

/*
 * Volatile Q8.8 (fixed point): bits 15:8 are 0x0A, bit 1 is the byte index within the 2-byte fixed type (0 or 1).
 */
#define SFP_PLUS_RO_VOL_FIXP_8_8           0x4200

/*
 * Volatile Q8.16 (fixed point): bits 15:8 are 0x0A, bits 1..0 is the byte index within the 2-byte fixed type (0..2).
 */
#define SFP_PLUS_RO_VOL_FIXP_8_16          0x4300

/*
 * Volatile system state, floating-point value
 */
#define SFP_PLUS_RO_VOL_FLOAT_32           0x4400

/*
 * Read-only, one-time programmable byte from flash that are NOT accessible with byte reads; we must use 16-bit or
 * 32-bit read operations or we get a precise bus fault. This is the system ID factory data.
 */
#define SFP_PLUS_RO_OTP_BYTE               0x8100


/*
 * Values from the application flash data area. This is data that can be programmed in several ways: using the
 * built-in bootloader (I think, not yet tested)), using the STM32CubeProgrammer application, or by writing values
 * using the SFP+ interface, which writes them to the flash_app_data_area_cache_a array, and then triggering a
 * flash write to copy the cached values in SRAM to the flash.
 */
#define SFP_PLUS_RW_FLASH_DATA_BYTE        0xF100

#define SFP_PLUS_RW_FLASH_DATA_FIXP_8_8    0xF200

#define SFP_PLUS_RW_FLASH_DATA_FIXP_8_16   0xF300

#define SFP_PLUS_RW_FLASH_DATA_FLOAT_32    0xF400

/*
 * The byte_p pointer is used as a pointer into writeable memory to use as a cache, a const data table in flash,
 * a factory-programable data region in flash, depending on the byte_type. I use void types _and_ a union between const
 * and non-const types so that one common structure type in my byte arrays can contain a mix of pointers to const and
 * non-const memory without warnings about discarding const qualifiers.
 */
typedef uint16_t byte_type_t;

typedef union sfp_plus_byte_u {
    void       * nonconst_byte_p;
    const void * const_byte_p;
} sfp_plus_byte_u_t;

typedef uint8_t (*read_cache_float_fp_t)(uint8_t q_idx, float * voltage_p);

typedef union sfp_plus_read_and_cache_fp_u {
    read_cache_float_fp_t r_c_float_fp;
    void *                r_c_no_fp;
} sfp_plus_read_and_cache_fp_u_t;

typedef struct sfp_plus_byte_s {
    byte_type_t                    byte_type;
    sfp_plus_byte_u_t              byte_p;
    sfp_plus_read_and_cache_fp_u_t read_and_cache_fp;
} sfp_plus_byte_t;

extern const sfp_plus_byte_t device_secondary_half_page_vendor_0xF0[128];

#endif /* INC_SFP_PLUS_FIELD_DEFS_H_ */
