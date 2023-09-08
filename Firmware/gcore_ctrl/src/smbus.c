/*
 * smbus.c
 *
 * SMBus access module
 *  Implements an I2C-compatible interface at 7-bit address 0x12.  Main logic
 *  is interrupt driven by the SMBus peripheral with action taken based on
 *  current I2C access state.
 *
 * Copyright (c) 2021-2022 danjuliodesigns, LLC.  All rights reserved.
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
#include "adc.h"
#include "config.h"
#include "nvram.h"
#include "rtc.h"
#include "smbus.h"


//-----------------------------------------------------------------------------
// Variables
//-----------------------------------------------------------------------------
static volatile bool in_transfer = false;
static volatile uint8_t SI_SEG_IDATA smb_reg_array[SMB_NUM_REG];



//-----------------------------------------------------------------------------
// Internal Routine forward declarations
//-----------------------------------------------------------------------------
static uint8_t _SMB_ReadRegister(uint16_t reg);
static void _SMB_WriteRegister(uint16_t reg, uint8_t d);



//-----------------------------------------------------------------------------
// API routines
//-----------------------------------------------------------------------------
void SMB_Init()
{
	uint8_t i;

	for (i=0; i<SMB_NUM_REG; i++) {
		smb_reg_array[i] = 0;
	}

	// Update non-zero values
	smb_reg_array[SMB_INDEX_ID] = FW_ID;
	smb_reg_array[SMB_INDEX_VERSION] = (FW_VER_MAJOR << 4) | FW_VER_MINOR;
	smb_reg_array[SMB_INDEX_BL_PWM] = BL_PWM_DEFAULT;
	smb_reg_array[SMB_INDEX_PWR_TM] = SMB_PWR_BTN_TO_DEFAULT;
}


void SMB_Suspend()
{
	// Disable SMBus interrupts
	SMBUS_DIS_INT();

	// Disable TIMER 3
	EIE1 &= ~EIE1_ET3__BMASK;          // Disable TIMER 3 interrupts
	TMR3CN0 &= ~TMR3CN0_TR3__BMASK;    // TIMER 3 disabled

	// Clear ACK
	SMB0CN0_ACK = 0;

	// Clear out any interrupted transfers
	in_transfer = false;
}


void SMB_Resume()
{
	// Clear any bogus state from a previously partially completed cycle (e.g when power went down)
	in_transfer = false;

	// Make sure EXTHOLD = 1 for correct operation of bug work-around
	SMB0CF |= SMB0CF_EXTHOLD__BMASK;

	// Restart TIMER 3 (use constants from InitDevice.c)
	TMR3RLH = (0xF9 << TMR3RLH_TMR3RLH__SHIFT);
	TMR3RLL = (0x4D << TMR3RLL_TMR3RLL__SHIFT);
	TMR3CN0 &= ~(TMR3CN0_TF3L__BMASK | TMR3CN0_TF3H__BMASK);
	TMR3CN0 |= TMR3CN0_TR3__RUN;
	EIE1 |= EIE1_ET3__ENABLED;

	// Reset communication
	SMB0CF &= ~0x80;
	SMB0CF |= 0x80;
	SMB0CN0_STA = 0;
	SMB0CN0_STO = 0;
	SMB0CN0_ACK = 0;

	// Re-enable SMBus interrupts
	SMBUS_EN_INT();
}


void SMB_ShutDown()
{
	// Disable the SMBus peripheral and make sure any ongoing
	// activity as the system powers off doesn't hang the EFM8
	// Stop our ISR from running
	SMBUS_DIS_INT();

	// Disable SMbus
	SMB0CF &= ~(SMB0CF_ENSMB__BMASK | SMB0CF_EXTHOLD__BMASK);
	SMB0CF |= SMB0CF_INH__SLAVE_DISABLED;

	// Clear ACK
	SMB0CN0_ACK = 0;

	// Clear out any interrupted transfers
	in_transfer = false;
}


// Routines for use by main code to atomically update SMBus values
// Note: The following mechanisms are used to prevent data corruption
//       and code lock-up:
//         1. Multi-byte access routines wait until the SMBus interface
//            is not busy by polling in_transfer.  Then they immediately
//            disable SMBus interrupts while accessing the data.  There
//            is a chance that a SMBus interrupt could occur immediately
//            after the poll.  However this will occur at the end of the
//            I2C chip address phase and access of any data is at least
//            80 uSec away.
//         2. Multi-byte access routines also disable the ADC ISR to
//            prevent two things:
//               a. A possible lock-up condition where the ADC ISR executes
//                  part-way through the access of the multiple bytes and
//                  then calls its own SMBus access routine which will then
//                  hang on the in_transfer poll.
//               b. An additional delay between the end of an in_transfer
//                  poll and the disabling of the SMBus ISR due to an ADC
//                  ISR execution.  This should ensure the Multi-byte transfer
//                  executes and re-enables the SMBus ISR before the next
//                  byte is transfered on I2C and the SMBus ISR called again.
//
void SMB_SetIndexedValue8(uint8_t index, uint8_t val)
{
	smb_reg_array[index] = val;
}


uint8_t SMB_GetIndexedValue8(uint8_t index)
{
	return smb_reg_array[index];
}


void SMB_SetTime(uint32_t val)
{
	ADC_DIS_INT();

	while (in_transfer == 1) {};

	SMBUS_DIS_INT();
	// Fast array-to-array copy while ISRs disabled
	smb_reg_array[SMB_INDEX_TIME_H] = *(((uint8_t*) &val) + 0);
	smb_reg_array[SMB_INDEX_TIME_2] = *(((uint8_t*) &val) + 1);
	smb_reg_array[SMB_INDEX_TIME_1] = *(((uint8_t*) &val) + 2);
	smb_reg_array[SMB_INDEX_TIME_L] = *(((uint8_t*) &val) + 3);
	SMBUS_EN_INT();
	ADC_EN_INT();
}


int16_t SMB_GetVB()
{
	uint16_t v;

	ADC_DIS_INT();

	while (in_transfer == 1) {};

	SMBUS_DIS_INT();
	v = (smb_reg_array[SMB_INDEX_VB_H] << 8) | smb_reg_array[SMB_INDEX_VB_L];
	SMBUS_EN_INT();
	ADC_EN_INT();

	return v;
}


uint32_t SMB_GetTime()
{
	uint8_t v[4];

	ADC_DIS_INT();

	while (in_transfer == 1) {};

	SMBUS_DIS_INT();
	// Array to array copy fastest while ISRs disabled
	v[0] = smb_reg_array[SMB_INDEX_TIME_H];
	v[1] = smb_reg_array[SMB_INDEX_TIME_2];
	v[2] = smb_reg_array[SMB_INDEX_TIME_1];
	v[3] = smb_reg_array[SMB_INDEX_TIME_L];
	SMBUS_EN_INT();
	ADC_EN_INT();

	// Get 4-bytes of big-endian data to return
	return *((uint32_t*) &v[0]);
}


uint32_t SMB_GetAlarm()
{
	uint8_t v[4];

	ADC_DIS_INT();

	while (in_transfer == 1) {};

	SMBUS_DIS_INT();
	// Array to array copy fastest while ISRs disabled
	v[0] = smb_reg_array[SMB_INDEX_ALRM_H];
	v[1] = smb_reg_array[SMB_INDEX_ALRM_2];
	v[2] = smb_reg_array[SMB_INDEX_ALRM_1];
	v[3] = smb_reg_array[SMB_INDEX_ALRM_L];
	SMBUS_EN_INT();
	ADC_EN_INT();

	// Get 4-bytes of big-endian data to return
	return *((uint32_t*) &v[0]);
}


int32_t SMB_GetCorrect()
{
	uint8_t v[4];

	ADC_DIS_INT();

	while (in_transfer == 1) {};

	SMBUS_DIS_INT();
	// Array to array copy fastest while ISRs disabled
	v[0] = smb_reg_array[SMB_INDEX_CORR_H];
	v[1] = smb_reg_array[SMB_INDEX_CORR_2];
	v[2] = smb_reg_array[SMB_INDEX_CORR_1];
	v[3] = smb_reg_array[SMB_INDEX_CORR_L];
	SMBUS_EN_INT();
	ADC_EN_INT();

	// Get 4-bytes of big-endian data to return
	return *((uint32_t*) &v[0]);
}


void SMB_SetStatusPowerOnMask(uint8_t mask)
{
	// Set the power-on status bit
	SMBUS_DIS_INT();
	smb_reg_array[SMB_INDEX_STATUS] = (smb_reg_array[SMB_INDEX_STATUS] & ~SMB_ST_PWR_ON_RSN_MASK) | (mask & SMB_ST_PWR_ON_RSN_MASK);
	SMBUS_EN_INT();
}


void SMB_SetStatusBit(uint8_t mask, bool val)
{
	SMBUS_DIS_INT();
	smb_reg_array[SMB_INDEX_STATUS] = smb_reg_array[SMB_INDEX_STATUS] & ~mask;
	if (val) {
		smb_reg_array[SMB_INDEX_STATUS] |= mask;
	}
	SMBUS_EN_INT();
}


// Routines for use by ADC ISR to atomically update SMBus values
void SMB_SetVU(int16_t val)
{
	while (in_transfer == 1) {};

	SMBUS_DIS_INT();
	smb_reg_array[SMB_INDEX_VU_H] = val >> 8;
	smb_reg_array[SMB_INDEX_VU_L] = val & 0xFF;
	SMBUS_EN_INT();
}


void SMB_SetIU(int16_t val)
{
	while (in_transfer == 1) {};

	SMBUS_DIS_INT();
	smb_reg_array[SMB_INDEX_IU_H] = val >> 8;
	smb_reg_array[SMB_INDEX_IU_L] = val & 0xFF;
	SMBUS_EN_INT();
}


void SMB_SetVB(int16_t val)
{
	while (in_transfer == 1) {};

	SMBUS_DIS_INT();
	smb_reg_array[SMB_INDEX_VB_H] = val >> 8;
	smb_reg_array[SMB_INDEX_VB_L] = val & 0xFF;
	SMBUS_EN_INT();
}


void SMB_SetIL(int16_t val)
{
	while (in_transfer == 1) {};

	SMBUS_DIS_INT();
	smb_reg_array[SMB_INDEX_IL_H] = val >> 8;
	smb_reg_array[SMB_INDEX_IL_L] = val & 0xFF;
	SMBUS_EN_INT();
}


void SMB_SetTemp(int16_t val)
{
	while (in_transfer == 1) {};

	SMBUS_DIS_INT();
	smb_reg_array[SMB_INDEX_TEMP_H] = val >> 8;
	smb_reg_array[SMB_INDEX_TEMP_L] = val & 0xFF;
	SMBUS_EN_INT();
}



//-----------------------------------------------------------------------------
// SMBUS0_ISR
//-----------------------------------------------------------------------------
//  SMBUS0_IRQ requires ~2-6 uSec
//
SI_INTERRUPT (SMBUS0_ISR, SMBUS0_IRQn)
{
	static bool first_byte;
	static bool second_byte;
	static uint16_t SI_SEG_DATA smb_reg;
	static uint16_t SI_SEG_DATA smb_data;

	/* EMF8SB2 SMBus I2C Bug work-around described in the section "Multi-Slave Behavior" of the reference manual
	 *  1. Set EXTHOLD=1 to work around interrupt execution on every I2C transaction (and subsequent bus hold)
	 *     - EXTHOLD set by initialization code
	 *     - EXTHOLD cleared during transfer and re-enabled at the end to support repeated start
	 *  2. Use flag "in_transfer" instead of STA bit to detect address phase of I2C cycle
	 *  3. Use TIMER3 to automatically clear ACK every 70 uSec (supporting max 100 kHz SCL) while not in a transfer
	 *     to prevent a potential lock-up condition
	 */
	if (!in_transfer) {
		// Assume Start+address received
		in_transfer = true;
		SMB0CF &= ~SMB0CF_EXTHOLD__BMASK;  // EXTHOLD = 0 to allow detection of repeated start
		EIE1 &= ~EIE1_ET3__BMASK;          // Disable TIMER 3 interrupts
		TMR3CN0 &= ~TMR3CN0_TR3__BMASK;    // TIMER 3 disabled
		SMB0CN0_STA = 1;
	}

	if (SMB0CN0_ARBLOST == 0) {
		switch (SMB0CN0 & 0xF0) {       // Decode the SMBus status vector
			// Slave Receiver: Start+Address received
			case  SMB_SRADD:
				SMB0CN0_STA = 0;                   // Clear SMB0CN0_STA bit

				if ((SMB0DAT & SMB0ADM & 0xFE) == (SMB0ADR & SMB0ADM & 0xFE)) {
					first_byte = true;
					second_byte = false;

					if ((SMB0DAT&0x01) == SMB_READ) { // Prepare outgoing byte if the transfer is a master READ
						SMB0DAT = _SMB_ReadRegister(smb_reg);
						smb_reg++;
					} else {
						SMB0CN0_ACK = 1;             // Auto ACK the next master WRITE
					}
				}
				break;

			// Slave Receiver: Data received
			case  SMB_SRDB:
				if (first_byte) {
					// Set the register high address
					first_byte = false;
					second_byte = true;
					smb_reg = SMB0DAT;
					smb_data = 0;
				} else if (second_byte) {
					// Set the register low address
					second_byte = false;
					smb_reg = (smb_reg << 8) | SMB0DAT;
				} else {
					// Writing data
					_SMB_WriteRegister(smb_reg, SMB0DAT);
					smb_reg++;
				}
				SMB0CN0_ACK = 1;                // SMB0CN0_ACK received data
				break;

				// Slave Receiver: Stop received while either a Slave Receiver or
				// Slave Transmitter
			case  SMB_SRSTO:
				in_transfer = false;
				SMB0CN0_STO = 0;                // SMB0CN0_STO must be cleared by software when
				                                // a STOP is detected as a slave
				break;

				// Slave Transmitter: Data byte transmitted
			case  SMB_STDB:
				if (SMB0CN0_ACK == 1) {         // If Master SMB0CN0_ACK's, send the next byte
					SMB0DAT = _SMB_ReadRegister(smb_reg);
					smb_reg++;
				}                               // Otherwise, do nothing
				break;

			// Slave Transmitter: Arbitration lost, Stop detected
			//
			// This state will only be entered on a bus error condition.
			// In normal operation, the slave is no longer sending data or has
			// data pending when a STOP is received from the master, so the SMB0CN0_TXMODE
			// bit is cleared and the slave goes to the SRSTO state.
			case  SMB_STSTO:
				in_transfer = false;
				SMB0CN0_STO = 0;                 // SMB0CN0_STO must be cleared by software when
				                                 // a STOP is detected as a slave
				break;

				// Default: all other cases undefined
			default:
				SMB0CF &= ~0x80;           // Reset communication
				SMB0CF |= 0x80;
				SMB0CN0_STA = 0;
				SMB0CN0_STO = 0;
				SMB0CN0_ACK = 0;
				in_transfer = false;
				break;
		}
	}
	// SMB0CN0_ARBLOST = 1, Abort failed transfer
	else {
		SMB0CN0_STA = 0;
		SMB0CN0_STO = 0;
		SMB0CN0_ACK = 0;
		in_transfer = false;
	}

	// Re-enable bug-fix functionality after transfer ends
	if (!in_transfer) {
		SMB0CF |= SMB0CF_EXTHOLD__BMASK;  // EXTHOLD = 1

		// Restart TIMER 3 (use constants from InitDevice.c)
		TMR3RLH = (0xF9 << TMR3RLH_TMR3RLH__SHIFT);
		TMR3RLL = (0x4D << TMR3RLL_TMR3RLL__SHIFT);
		TMR3CN0 &= ~(TMR3CN0_TF3L__BMASK | TMR3CN0_TF3H__BMASK);
		TMR3CN0 |= TMR3CN0_TR3__RUN;
		EIE1 |= EIE1_ET3__ENABLED;
	}

	SMB0CN0_SI = 0;                             // Clear SMBus interrupt flag
}


//-----------------------------------------------------------------------------
// TIMER3_ISR
//-----------------------------------------------------------------------------
//
// TIMER3 ISR Content goes here. Remember to clear flag bits:
// TMR3CN0::TF3H (Timer # High Byte Overflow Flag)
// TMR3CN0::TF3L (Timer # Low Byte Overflow Flag)
//
//-----------------------------------------------------------------------------
SI_INTERRUPT (TIMER3_ISR, TIMER3_IRQn)
{
	// Clear ACK
	SMB0CN0_ACK = 0;

	// Clear Overflow flags
	TMR3CN0 &= ~(TMR3CN0_TF3L__BMASK | TMR3CN0_TF3H__BMASK);
}


//-----------------------------------------------------------------------------
// Internal Routines
//-----------------------------------------------------------------------------
uint8_t _SMB_ReadRegister(uint16_t reg)
{
	uint8_t d8;
	uint8_t reg_addr;

	if (reg < NVRAM_LEN) {
		d8 = NVRAM_Read(reg);
	} else {
		// Mask off upper bits
		reg_addr = reg & SMB_REG_ADDR_MASK;

		d8 = smb_reg_array[reg_addr];

		// Special case for Power Button Press Detected bit in the STATUS register.
		if (reg_addr == SMB_INDEX_STATUS) {
			// Clear the bit after reading
			smb_reg_array[SMB_INDEX_STATUS] &= ~(SMB_ST_PB_PRESS_MASK);
		}
	}

	return (d8);
}


void _SMB_WriteRegister(uint16_t reg, uint8_t d)
{
	uint8_t reg_addr;

	if (reg < NVRAM_LEN) {
		NVRAM_Write(reg, d);
	} else {
		// Mask off upper bits
		reg_addr = reg & SMB_REG_ADDR_MASK;

		if (reg_addr >= SMB_INDEX_RW_START) {
			smb_reg_array[reg_addr] = d;

			// Special case for RTC Time setting
			if (reg_addr == SMB_INDEX_TIME_L) {
				RTC_SetTimeTrigger();
			}
		}
	}
}
