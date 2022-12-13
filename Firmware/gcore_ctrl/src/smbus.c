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
 */
#include <SI_EFM8SB2_Register_Enums.h>
#include "adc.h"
#include "config.h"
#include "nvram.h"
#include "rtc.h"
#include "run.h"
#include "smbus.h"
#include "watchdog.h"


//-----------------------------------------------------------------------------
// Variables
//-----------------------------------------------------------------------------
static volatile bool in_transfer = false;
static volatile uint8_t smb_reg_array[SMB_NUM_REG];
static volatile uint16_t smb_reg16_array[SMB_NUM_REG16];



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
}


void SMB_SetIndexedValue8(uint8_t index, uint8_t val)
{
	SMBUS_DIS_INT();
	smb_reg_array[index] = val;
	SMBUS_EN_INT();
}


uint8_t SMB_GetIndexedValue8(uint8_t index)
{
	return smb_reg_array[index];
}


void SMB_SetIndexedValue16(uint8_t index, uint16_t val)
{
	SMBUS_DIS_INT();
	smb_reg16_array[index] = val;
	SMBUS_EN_INT();
}


uint16_t SMB_GetIndexedValue16(uint8_t index)
{
	uint16_t v;

	// Must disable ADC from updating when we read from normal code
	ADC_DIS_INT();
	v = smb_reg16_array[index];
	ADC_EN_INT();

	return v;
}


void SMB_SetStatusPowerOnMask(uint8_t mask)
{
	// Set the power-on status bit
	SMBUS_DIS_INT();
	smb_reg_array[SMB_INDEX_STATUS] = (smb_reg_array[SMB_INDEX_STATUS] & ~SMB_ST_PWR_ON_RSN_MASK) | (mask & SMB_ST_PWR_ON_RSN_MASK);

	// Clear any corresponding set wakeup bit
	switch (mask) {
		case RUN_START_ALARM:
			smb_reg_array[SMB_INDEX_WK_CTRL] &= ~SMB_WK_CTRL_ALARM_MASK;
			break;

		case RUN_START_CHG:
			smb_reg_array[SMB_INDEX_WK_CTRL] &= ~(SMB_WK_CTRL_CHRG_START_MASK | SMB_WK_CTRL_CHRG_DONE_MASK);
			break;
	}
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




//-----------------------------------------------------------------------------
// SMBUS0_ISR
//-----------------------------------------------------------------------------
//  SMBUS0_IRQ requires ~1-5 uSec
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
	uint32_t d32;
	uint16_t d16;
	uint8_t d8;
	uint8_t reg_addr;

	if (reg < NVRAM_LEN) {
		d8 = NVRAM_Read(reg);
	} else {
		// Mask off upper bits
		reg_addr = reg & SMB_REG_ADDR_MASK;

		// Special cases for > 8-bit registers
		if (reg_addr == SMB_INDEX_VU_H) {
			d16 = smb_reg16_array[SMB_INDEX16_VU];
			smb_reg_array[SMB_INDEX_VU_H] = d16 >> 8;
			smb_reg_array[SMB_INDEX_VU_L] = d16 & 0xFF;
		}
		else if (reg_addr == SMB_INDEX_IU_H) {
			d16 = smb_reg16_array[SMB_INDEX16_IU];
			smb_reg_array[SMB_INDEX_IU_H] = d16 >> 8;
			smb_reg_array[SMB_INDEX_IU_L] = d16 & 0xFF;
		}
		else if (reg_addr == SMB_INDEX_VB_H) {
			d16 = smb_reg16_array[SMB_INDEX16_VB];
			smb_reg_array[SMB_INDEX_VB_H] = d16 >> 8;
			smb_reg_array[SMB_INDEX_VB_L] = d16 & 0xFF;
		}
		else if (reg_addr == SMB_INDEX_IL_H) {
			d16 = smb_reg16_array[SMB_INDEX16_IL];
			smb_reg_array[SMB_INDEX_IL_H] = d16 >> 8;
			smb_reg_array[SMB_INDEX_IL_L] = d16 & 0xFF;
		}
		else if (reg_addr == SMB_INDEX_TEMP_H) {
			d16 = smb_reg16_array[SMB_INDEX16_T];
			smb_reg_array[SMB_INDEX_TEMP_H] = d16 >> 8;
			smb_reg_array[SMB_INDEX_TEMP_L] = d16 & 0xFF;
		}
		else if (reg_addr == SMB_INDEX_TIME_H) {
			// Atomically get all 4 RTC bytes when we read the MSbyte
			d32 = RTC_GetTime();
			for (d8 = SMB_INDEX_TIME_L; d8 >= SMB_INDEX_TIME_H; d8--) {
				smb_reg_array[d8] = d32 & 0xFF;
				d32 = d32 >> 8;
			}
		}

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
	if (reg < NVRAM_LEN) {
		NVRAM_Write(reg, d);
	} else {
		// Mask off upper bits
		reg &= SMB_REG_ADDR_MASK;

		if (reg >= SMB_INDEX_RW_START) {
			smb_reg_array[reg] = d;

			// Special case for RTC Time setting
			if (reg == SMB_INDEX_TIME_L) {
				RTC_SetTimeTrigger();
			}
		}
	}
}
