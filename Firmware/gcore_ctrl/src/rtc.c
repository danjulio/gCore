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
 * Copyright (c) 2021, 2023 danjuliodesigns, LLC.  All rights reserved.
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
#include "rtc.h"
#include "smbus.h"


// =========================================================
// Variables
// =========================================================
static volatile uint32_t rtc_time;
static volatile uint32_t rtc_prev_time;
static volatile uint32_t rtc_alarm;
static volatile int32_t rtc_correction;
static volatile int32_t rtc_correction_count;
static volatile bit rtc_set_time;
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
	rtc_prev_time = 0;
	rtc_alarm = 0;
	rtc_set_time = 0;
	rtc_alarm_enabled = 0;
	rtc_saw_alarm = 0;
	rtc_correction_enabled = 0;
}


void RTC_Eval()
{
	bool alarm_enabled;
	int32_t corr_val;
	uint32_t t;

	//
	// Check for set new time, otherwise check if it's time to update SMBus
	//
	if (rtc_set_time == 1) {
		rtc_set_time = 0;

		// Get the new time from SMBUS registers
		t = SMB_GetTime();
		_RTC_Disable_Interrupt();
		rtc_time = t;
		_RTC_Enable_Interrupt();
	} else {
		_RTC_Disable_Interrupt();
		t = rtc_time;
		_RTC_Enable_Interrupt();

		if (t != rtc_prev_time) {
			SMB_SetTime(t);
			rtc_prev_time = t;
		}
	}

	//
	// Check for Alarm enable/disable
	//
	alarm_enabled = (SMB_GetIndexedValue8(SMB_INDEX_WK_CTRL) & SMB_WK_CTRL_ALARM_MASK == SMB_WK_CTRL_ALARM_MASK);
	if (alarm_enabled && (rtc_alarm_enabled == 0)) {
		// Get the wakeup alarm time
		rtc_alarm = SMB_GetAlarm();

		// Enable the wakeup alarm after setting rtc_alarm
		rtc_alarm_enabled = 1;
	} else if (!alarm_enabled && (rtc_alarm_enabled == 1)) {
		rtc_alarm_enabled = 0;
	}

	//
	// Check for correction enable/disable/change
	//
	corr_val = SMB_GetCorrect();
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


void RTC_SetTimeTrigger()
{
	rtc_set_time = 1;
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
	// Disable ADC interrupts to prevent a condition where a SMBus access
	// routine called by the ADC ISR blocks during a long transaction
	// after RTC interrupts are disabled which causes a RTC interrupt to
	// be missed.
	ADC_DIS_INT();

	EIE1 &= ~EIE1_ERTC0A__ENABLED;
}


static void _RTC_Enable_Interrupt()
{
	EIE1 |= EIE1_ERTC0A__ENABLED;

	// Re-enable ADC interrupts
	ADC_EN_INT();
}



// =========================================================
// RTC ISR
// =========================================================
// ISR requires about 2-7 uSec
//
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
