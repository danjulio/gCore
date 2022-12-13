/*
 * smbus.h
 *
 * Header for SMBus access module
 *
 * Copyright (c) 2021-2022 danjuliodesigns, LLC.  All rights reserved.
 *
 */

#ifndef SMBUS_H_
#define SMBUS_H_

#include <stdint.h>
#include "config.h"


//-----------------------------------------------------------------------------
// Constants
//-----------------------------------------------------------------------------

// Externally accessible Register access indices
//  To simplify the interrupt-driven SMB logic, we maintain a set of registers
//  in this module that other modules can update or access.  The first registers
// are RO.
//
// 8-bit register indices (index into local array)
#define SMB_INDEX_ID       0
#define SMB_INDEX_VERSION  1
#define SMB_INDEX_STATUS   2
#define SMB_INDEX_GPIO     3
#define SMB_INDEX_VU_H     4
#define SMB_INDEX_VU_L     5
#define SMB_INDEX_IU_H     6
#define SMB_INDEX_IU_L     7
#define SMB_INDEX_VB_H     8
#define SMB_INDEX_VB_L     9
#define SMB_INDEX_IL_H     10
#define SMB_INDEX_IL_L     11
#define SMB_INDEX_TEMP_H   12
#define SMB_INDEX_TEMP_L   13
#define SMB_INDEX_BL_PWM   14
#define SMB_INDEX_WK_CTRL  15
#define SMB_INDEX_SHDOWN   16
#define SMB_INDEX_PWR_TM   17
#define SMB_INDEX_NV_CTRL  18
#define SMB_INDEX_TIME_H   19
#define SMB_INDEX_TIME_2   20
#define SMB_INDEX_TIME_1   21
#define SMB_INDEX_TIME_L   22
#define SMB_INDEX_ALRM_H   23
#define SMB_INDEX_ALRM_2   24
#define SMB_INDEX_ALRM_1   25
#define SMB_INDEX_ALRM_L   26
#define SMB_INDEX_CORR_H   27
#define SMB_INDEX_CORR_2   28
#define SMB_INDEX_CORR_1   29
#define SMB_INDEX_CORR_L   30

#define SMB_INDEX_RW_START SMB_INDEX_BL_PWM

// Number and mask for registers (must be power-of-two and match)
#define SMB_NUM_REG        32
#define SMB_REG_ADDR_MASK  0x1F

// Internal 16-bit register backing array to allow atomic access
//
// Indices
#define SMB_INDEX16_VU     0
#define SMB_INDEX16_IU     1
#define SMB_INDEX16_VB     2
#define SMB_INDEX16_IL     3
#define SMB_INDEX16_T      4

// Number of 16-bit backing registers
#define SMB_NUM_REG16      5


// Register masks
//
// GPIO register masks
#define SMB_GPIO_SD_CARD_MASK  0x08
#define SMB_GPIO_PWR_BTN_MASK  0x04
#define SMB_GPIO_CHG_1_MASK    0x02
#define SMB_GPIO_CHG_0_MASK    0x01

// Status register masks
#define SMB_ST_CRIT_BATT_MASK  0x80
#define SMB_ST_PB_PRESS_MASK   0x10
#define SMB_ST_PWR_ON_RSN_MASK 0x07

// NVRAM Flash register trigger values
#define SMB_NVRAM_WR_TRIG      'W'
#define SMB_NVRAM_RD_TRIG      'R'

// NVRAM Flash register busy mask
#define SMB_NVRAM_BUSY_MASK    0x01
#define SMB_NVRAM_IDLE_MASK    0x00

// Shutdown register trigger
#define SMB_SHUTDOWN_TRIG_MASK 0x0F

// Wakeup Control register masks
#define SMB_WK_CTRL_ALARM_MASK      0x01
#define SMB_WK_CTRL_CHRG_START_MASK 0x02
#define SMB_WK_CTRL_CHRG_DONE_MASK  0x04



//
// Internal SMBus operation constants
//

// I2C Address LSB
#define  SMB_WRITE      0x00           // SMBus WRITE command
#define  SMB_READ       0x01           // SMBus READ command

// Status vector - top 4 bits only
#define  SMB_SRADD      0x20           // (SR) slave address received
                                       //    (also could be a lost
                                       //    arbitration)
#define  SMB_SRSTO      0x10           // (SR) STOP detected while SR or ST,
                                       //    or lost arbitration
#define  SMB_SRDB       0x00           // (SR) data byte received, or
                                       //    lost arbitration
#define  SMB_STDB       0x40           // (ST) data byte transmitted
#define  SMB_STSTO      0x50           // (ST) STOP detected during a
                                       //    transaction; bus error


// Default Power Button Press timeout (units of 10 mSec)
#define SMB_PWR_BTN_TO_DEFAULT  (BTN_PRESS_SHORT_MSEC / 10)


//-----------------------------------------------------------------------------
// API Routines
//-----------------------------------------------------------------------------
void SMB_Init();      // Designed to be called only at power-up
void SMB_Suspend();   // Designed to halt SMBus activity when going from Power to Charge run states
void SMB_Resume();    // Designed to be called when starting/resuming SMBus to clear out any partial cycles left over
void SMB_ShutDown();  // Designed to be called before sleep to prevent a bus hang

// Routines for use by main code to atomically update SMBus values
void SMB_SetIndexedValue8(uint8_t index, uint8_t val);
uint8_t SMB_GetIndexedValue8(uint8_t index);
void SMB_SetIndexedValue16(uint8_t index, uint16_t val);
uint16_t SMB_GetIndexedValue16(uint8_t index);
void SMB_SetStatusPowerOnMask(uint8_t mask);
void SMB_SetStatusBit(uint8_t mask, bool val);

// Interrupt macros
#define SMBUS_DIS_INT() EIE1 &= ~EIE1_ESMB0__BMASK
#define SMBUS_EN_INT()  EIE1 |= EIE1_ESMB0__BMASK


#endif /* SMBUS_H_ */
