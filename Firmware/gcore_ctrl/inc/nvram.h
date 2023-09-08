/*
 * nvram.h
 *
 * Copyright (c) 2021 danjuliodesigns, LLC.  All rights reserved.
 *
 * This is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * It is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this software.  If not, see <https://www.gnu.org/licenses/>.
 *
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
