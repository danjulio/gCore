/*
 * nvram.h
 *
 * Copyright (c) 2021 danjuliodesigns, LLC.  All rights reserved.
 */

#ifndef INC_NVRAM_H_
#define INC_NVRAM_H_
#include <stdint.h>


// =========================================================
// Constants
// =========================================================

// Scratchpad FLASH size
#define NVRAM_FLASH_LEN   1024

// NVRAM size
#define NVRAM_LEN         4096

// NVRAM 16-bit address mask (must match size)
#define NVRAM_ADDR_MASK   0x0FFF

// Number of writes to perform in a millisecond eval
//  - writes take < ~72 uSec max
//  - leave overhead to be interrupted by other activity (ADC, I2C)
//  - Must be evenly divisible into NVRAM_FLASH_LEN
#define NVRAM_WRITES_EVAL 8



// =========================================================
// API
// =========================================================
void NVRAM_Init();  // Designed to be called only at power-on
void NVRAM_Eval();
void NVRAM_Write(uint16_t a, uint8_t d);
uint8_t NVRAM_Read(uint16_t a);

#endif /* INC_NVRAM_H_ */
