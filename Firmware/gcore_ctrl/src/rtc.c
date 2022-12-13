/*
 * rtc.c
 *
 * RTC with alarm and calibration function.
 *
 * Alarm can be set to wake the device up.
 *
 * Calibration value adjusts time forward or backward one second when the number
 * of calibration seconds has transpired.  A positive calibration value is used
 * for a slow clock and increments the second count.  A negative calibration value
 * is used for a fast clock and decrements the second count.
 *
 * Copyright (c) 2021 danjuliodesigns, LLC.  All rights reserved.
 */
#include <SI_EFM8SB2_Register_Enums.h>
#include "rtc.h"
#include "gpio.h"
#include "smbus.h"


// =========================================================
// Variables
// =========================================================
static volatile uint32_t rtc_time;
static volatile uint32_t rtc_alarm;
static volatile int32_t rtc_correction;
static volatile int32_t rtc_correction_count;
static volatile bit rtc_alarm_enabled;
static volatile bit rtc_saw_alarm;
static volatile bit rtc_correction_enabled;
static volatile bit rtc_correction_up;            // Set when correction_count increments toward zero (from neg cal)


// =========================================================
// Forward Declarations
// =========================================================
static void _RTC_Disable_Interrupt();
static void _RTC_Enable_Interrupt();


// =========================================================
// API
// =========================================================
void RTC_Init()
{
	rtc_time = 0;
	rtc_alarm = 0;
	rtc_alarm_enabled = 0;
	rtc_saw_alarm = 0;
	rtc_correction_enabled = 0;
}


void RTC_Eval()
{
	bool alarm_enabled;
	uint8_t i;
	int32_t corr_val;

	//
	// Check for Alarm enable/disable
	//
	alarm_enabled = SMB_GetIndexedValue8(SMB_INDEX_WK_CTRL) & SMB_WK_CTRL_ALARM_MASK == SMB_WK_CTRL_ALARM_MASK;
	if (alarm_enabled && (rtc_alarm_enabled == 0)) {
		// Get the wakeup alarm time
		rtc_alarm = 0;
		for (i=0; i<4; i++) {
			rtc_alarm = rtc_alarm << 8;
			rtc_alarm |= SMB_GetIndexedValue8(SMB_INDEX_ALRM_H + i);
		}

		// Enable the wakeup alarm after setting rtc_alarm
		rtc_alarm_enabled = 1;
	} else if (!alarm_enabled && (rtc_alarm_enabled == 1)) {
		rtc_alarm_enabled = 0;
	}

	//
	// Check for correction enable/disable/change
	//
	corr_val = 0;
	for (i=0; i<4; i++) {
		corr_val = corr_val << 8;
		corr_val |= SMB_GetIndexedValue8(SMB_INDEX_CORR_H + i);
	}
	if (corr_val != rtc_correction) {
		// Disable while we update values
		rtc_correction_enabled = 0;

		// Update values
		rtc_correction_up = (corr_val < 0) ? 1 : 0;
		rtc_correction_count = corr_val;
		rtc_correction = corr_val;

		// Finally (re)enable if necessary
		rtc_correction_enabled = (corr_val != 0) ? 1 : 0;
	}
}


uint32_t RTC_GetTime()
{
	uint32_t t;

	_RTC_Disable_Interrupt();
	t = rtc_time ;
	_RTC_Enable_Interrupt();

	return t;
}


void RTC_SetTimeTrigger()
{
	uint32_t t = 0;
	uint8_t i;

	// Get the new time from SMBUS registers
	for (i=0; i<4; i++) {
		t = t << 8;
		t |= SMB_GetIndexedValue8(SMB_INDEX_TIME_H + i);
	}

	_RTC_Disable_Interrupt();
	rtc_time = t;
	_RTC_Enable_Interrupt();
}


bool RTC_SawAlarm()
{
	if (rtc_saw_alarm == 1) {
		rtc_saw_alarm = 0;
		return true;
	} else {
		return false;
	}
}


void RTC_ClearAlarm()
{
	rtc_saw_alarm = 0;
}



// =========================================================
// Internal functions
// =========================================================
static void _RTC_Disable_Interrupt()
{
	EIE1 &= ~EIE1_ERTC0A__ENABLED;
}


static void _RTC_Enable_Interrupt()
{
	EIE1 |= EIE1_ERTC0A__ENABLED;
}



// =========================================================
// RTC ISR
// =========================================================
SI_INTERRUPT (RTC0ALARM_ISR, RTC0ALARM_IRQn)
{
	// Update time
	rtc_time++;

	// Apply any necessary correction
	if (rtc_correction_enabled == 1) {
		if (rtc_correction_up == 1) {
			// Fast clock
			if (++rtc_correction_count == 0) {
				// Decrement time
				rtc_time--;

				// Reset correction counter
				rtc_correction_count = rtc_correction;
			}
		} else {
			// Slow clock
			if (--rtc_correction_count == 0) {
				// Increment time
				rtc_time++;

				// Reset correction counter
				rtc_correction_count = rtc_correction;
			}
		}
	}

	// Check for an alarm condition
	if (rtc_alarm_enabled) {
		if (rtc_time >= rtc_alarm) {
			rtc_saw_alarm = 1;
			rtc_alarm_enabled = 0;
			rtc_alarm = 0;
		}
	}

	// Finally clear RTC0CN0::ALRM (RTC Alarm Event Flag and Auto Reset Enable)
	RTC0ADR = RTC0ADR_ADDR__RTC0CN0 | RTC0ADR_SHORT__BMASK;
	RTC0DAT = RTC0CN0_RTC0EN__ENABLED | RTC0CN0_RTC0TR__RUN
			| RTC0CN0_MCLKEN__DISABLED | RTC0CN0_RTC0AEN__DISABLED
			| RTC0CN0_ALRM__SET | RTC0CN0_RTC0CAP__NOT_SET
			| RTC0CN0_RTC0SET__NOT_SET;
	NOP();
	RTC0ADR = RTC0ADR_ADDR__RTC0CN0 | RTC0ADR_SHORT__BMASK;
	RTC0DAT = RTC0CN0_RTC0EN__ENABLED | RTC0CN0_RTC0TR__RUN
			| RTC0CN0_MCLKEN__DISABLED | RTC0CN0_RTC0AEN__ENABLED
			| RTC0CN0_ALRM__SET | RTC0CN0_RTC0CAP__NOT_SET
			| RTC0CN0_RTC0SET__NOT_SET;
}
