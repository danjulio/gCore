/*
 * nvram.c
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
#include <SI_EFM8SB2_Register_Enums.h>
#include "nvram.h"
#include "smbus.h"


// =========================================================
// Constants
// =========================================================



// =========================================================
// Variables
// =========================================================
static bit nv_ram_flash_in_progress;
static uint8_t SI_SEG_XDATA nvram_array[NVRAM_LEN];
static uint16_t SI_SEG_IDATA nvram_array_index;



// =========================================================
// Forward Declarations
// =========================================================
static void _scratchpad_erase();
static void _scratchpad_write(uint16_t addr, uint8_t dat);
static uint8_t _scratchpad_read(uint16_t addr);



// =========================================================
// API
// =========================================================
void NVRAM_Init()
{
	// Initialize first NVRAM_FLASH_LEN bytes from Scratchpad flash
	// Other bytes initialized by SILABS_STARTUP.A51
	for (nvram_array_index=0; nvram_array_index<NVRAM_FLASH_LEN; nvram_array_index++) {
		nvram_array[nvram_array_index] = _scratchpad_read(nvram_array_index);
	}
	nvram_array_index = 0;
	nv_ram_flash_in_progress = 0;
}


void NVRAM_Eval()
{
	uint8_t reg_data;

	if (nv_ram_flash_in_progress == 1) {
		// Write a number of bytes to flash during this interval
		for (reg_data=0; reg_data<NVRAM_WRITES_EVAL; reg_data++) {
			_scratchpad_write(nvram_array_index, nvram_array[nvram_array_index]);
			nvram_array_index += 1;
		}

		// Check if we are done
		if (nvram_array_index >= NVRAM_FLASH_LEN) {
			nv_ram_flash_in_progress = 0;
			SMB_SetIndexedValue8(SMB_INDEX_NV_CTRL, SMB_NVRAM_IDLE_MASK);
		}
	} else {
		reg_data = SMB_GetIndexedValue8(SMB_INDEX_NV_CTRL);

		// Look to see if we've been triggered to store NVRAM to flash
		if (reg_data == SMB_NVRAM_WR_TRIG) {
			// Setup to write the first NVRAM_FLASH_LEN bytes to flash
			SMB_SetIndexedValue8(SMB_INDEX_NV_CTRL, SMB_NVRAM_BUSY_MASK);
			nv_ram_flash_in_progress = 1;
			nvram_array_index = 0;

			// Erase the flash page (this is slow and will block execution so external SW must not start polling too quickly)
			_scratchpad_erase();
		}

		// Else look to see if we've been triggered to update from flash
		else if (reg_data == SMB_NVRAM_RD_TRIG) {
			// Read all data here
			SMB_SetIndexedValue8(SMB_INDEX_NV_CTRL, SMB_NVRAM_BUSY_MASK);
			nvram_array_index = 0;
			for (nvram_array_index=0; nvram_array_index<NVRAM_FLASH_LEN; nvram_array_index++) {
				nvram_array[nvram_array_index] = _scratchpad_read(nvram_array_index);
			}
			SMB_SetIndexedValue8(SMB_INDEX_NV_CTRL, SMB_NVRAM_IDLE_MASK);
		}

		// All other values are ignored and the register reloaded with IDLE
		else if (reg_data != SMB_NVRAM_IDLE_MASK) {
			SMB_SetIndexedValue8(SMB_INDEX_NV_CTRL, SMB_NVRAM_IDLE_MASK);
		}
	}
}


void NVRAM_Write(uint16_t a, uint8_t d)
{
	nvram_array[a & NVRAM_ADDR_MASK] = d;
}


uint8_t NVRAM_Read(uint16_t a)
{
	return nvram_array[a & NVRAM_ADDR_MASK];
}



// =========================================================
// Internal Functions
// =========================================================
// Requires 28-36 mSec
static void _scratchpad_erase()
{
	uint8_t SI_SEG_XDATA * SI_SEG_DATA pageP = 0;

	IE_EA = 0;                          // Disable Interrupts
	FLKEY  = 0xA5;                      // Key Sequence 1
	FLKEY  = 0xF1;                      // Key Sequence 2
	PSCTL = PSCTL_PSWE__WRITE_ENABLED | PSCTL_PSEE__ERASE_ENABLED | PSCTL_SFLE__SCRATCHPAD_ENABLED;
	*pageP = 0;                         // Write a byte to scratchpad to trigger erase
	PSCTL = 0;
	IE_EA = 1;                          // Re-enable Interrupts
}


// Requires 57-71 uSec
static void _scratchpad_write(uint16_t addr, uint8_t dat)
{
	uint8_t SI_SEG_XDATA * SI_SEG_DATA pageP = (uint8_t SI_SEG_XDATA *) addr;

	IE_EA = 0;                          // Disable Interrupts
	FLKEY  = 0xA5;                      // Key Sequence 1
	FLKEY  = 0xF1;                      // Key Sequence 2
	PSCTL = PSCTL_PSWE__WRITE_ENABLED | PSCTL_SFLE__SCRATCHPAD_ENABLED;
	*pageP = dat;                       // Write a byte to scratchpad to trigger erase
	PSCTL = 0;
	IE_EA = 1;                          // Re-enable Interrupts
}


static uint8_t _scratchpad_read(uint16_t addr)
{
	uint8_t SI_SEG_CODE * SI_SEG_DATA pageP = (uint8_t SI_SEG_CODE *) addr;
	uint8_t dat = 0;

	IE_EA = 0;                          // Disable Interrupts
	PSCTL = PSCTL_SFLE__SCRATCHPAD_ENABLED;
	dat = *pageP;
	PSCTL = 0;
	IE_EA = 1;                          // Re-enable Interrupts

	return dat;
}
