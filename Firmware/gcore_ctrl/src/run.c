/*
 * run.c
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
#include "adc.h"
#include "config.h"
#include "gpio.h"
#include "led.h"
#include "nvram.h"
#include "rtc.h"
#include "run.h"
#include "smbus.h"
#include "timer.h"
#include "watchdog.h"


// =========================================================
// Constants
// =========================================================
#define RUN_ST_WAKE   0
#define RUN_ST_CHARGE 1
#define RUN_ST_WAIT   2
#define RUN_ST_POWER  3
#define RUN_ST_POFF   4
#define RUN_ST_SLEEP  5



// =========================================================
// Variables
// =========================================================
static uint8_t run_state;
static uint16_t crit_batt_timer;



// =========================================================
// Internal Routine forward declarations
// =========================================================
static void RUN_Eval(uint8_t startup_mask);
static void RUN_SetState(uint8_t s);



// =========================================================
// API
// =========================================================
void RUN_Task(uint8_t startup_mask)
{
	// Start up in WAKE state
	RUN_SetState(RUN_ST_WAKE);

	// First, check battery voltage
	ADC_Init();
	if ((SMB_GetVB() < BATT_TURN_ON_MIN_MV) &&
		(GPIO_GetChargeState() != CHG_STATE_ON)) {

		// Battery voltage too low for turn-on and not charging: return to sleep
		// (If charging we will enter CHARGE state - and hope we're getting enough
		//  current to actually charge the battery)
		RUN_SetState(RUN_ST_SLEEP);
	}

//	if (run_state != RUN_ST_SLEEP) {
//		WD_Init();
//	}

	// System evaluation - falls out when we're going back to sleep
	while (run_state != RUN_ST_SLEEP) {
//		WD_Reset();

		if (TIMER_MsecTick(TIMER_EVAL_GPIO_PHASE)) {
			GPIO_Eval();
		}
		if (TIMER_MsecTick(TIMER_EVAL_LED_PHASE)) {
			LED_Eval();
		}
		if (TIMER_MsecTick(TIMER_EVAL_NVRAM_PHASE)) {
			NVRAM_Eval();
		}
		if (TIMER_MsecTick(TIMER_EVAL_RTC_PHASE)) {
			RTC_Eval();
		}
		if (TIMER_MsecTick(TIMER_EVAL_RUN_PHASE)) {
			RUN_Eval(startup_mask);
		}
	}
}



//-----------------------------------------------------------------------------
// Internal Routines
//-----------------------------------------------------------------------------

static void RUN_Eval(uint8_t startup_mask)
{
	bool batt_ok;
	uint8_t wakeup_reg;

	batt_ok = (SMB_GetVB() >= BATT_TURN_ON_MIN_MV);
	wakeup_reg = SMB_GetIndexedValue8(SMB_INDEX_WK_CTRL);

	switch (run_state) {
	case RUN_ST_WAKE:
		// Look for conditions to immediately power up
		if (((startup_mask & RUN_START_ALARM) != 0) && ((wakeup_reg & SMB_WK_CTRL_ALARM_MASK) != 0) && batt_ok) {

			SMB_SetStatusPowerOnMask(RUN_START_ALARM);
			SMB_SetIndexedValue8(SMB_INDEX_WK_CTRL, 0);
			RTC_ClearAlarm();
			RUN_SetState(RUN_ST_POWER);
		}
		else if ((startup_mask & RUN_START_CHG) != 0) {
			if (((wakeup_reg & SMB_WK_CTRL_CHRG_START_MASK) != 0) && (GPIO_GetChargeState() == CHG_STATE_ON) && batt_ok) {
				SMB_SetStatusPowerOnMask(RUN_START_CHG);
				SMB_SetIndexedValue8(SMB_INDEX_WK_CTRL, 0);
				RUN_SetState(RUN_ST_POWER);
			} else if (((wakeup_reg & SMB_WK_CTRL_CHRG_DONE_MASK) != 0) && (GPIO_GetChargeState() == CHG_STATE_DONE) && batt_ok) {
				SMB_SetStatusPowerOnMask(RUN_START_CHG);
				SMB_SetIndexedValue8(SMB_INDEX_WK_CTRL, 0);
				RUN_SetState(RUN_ST_POWER);
			} else if (GPIO_GetChargeState() != CHG_STATE_OFF) {
				RUN_SetState(RUN_ST_CHARGE);
			} else {
				RUN_SetState(RUN_ST_SLEEP);
			}
		}

		// Otherwise look for the button timer to expire so we can see if we're waking from a button press or sleeping
		else if (GPIO_ButtonShortPeriodExpired() && batt_ok) {
			RUN_SetState(RUN_ST_WAIT);
		}
		else if (!GPIO_ButtonDown()) {
			// Can sleep when the user lets go of the button
			RUN_SetState(RUN_ST_SLEEP);
		}
		break;

	case RUN_ST_CHARGE:
		// Look for conditions to power on fully
		if (batt_ok) {
			if (RTC_SawAlarm() && ((wakeup_reg & SMB_WK_CTRL_ALARM_MASK) != 0)) {
				SMB_SetStatusPowerOnMask(RUN_START_ALARM);
				SMB_SetIndexedValue8(SMB_INDEX_WK_CTRL, 0);
				RTC_ClearAlarm();
				RUN_SetState(RUN_ST_POWER);
			} else if (GPIO_ButtonShortPeriodExpired()) {
				RUN_SetState(RUN_ST_WAIT);
			} else if ((GPIO_GetChargeState() == CHG_STATE_DONE) && ((wakeup_reg & SMB_WK_CTRL_CHRG_DONE_MASK) != 0)) {
				SMB_SetStatusPowerOnMask(RUN_START_CHG);
				SMB_SetIndexedValue8(SMB_INDEX_WK_CTRL, 0);
				RUN_SetState(RUN_ST_POWER);
			}

			// Otherwise look for exit from charge to sleep
			else if (GPIO_GetChargeState() == CHG_STATE_OFF) {
				RUN_SetState(RUN_ST_SLEEP);
			}
		}
		else if (GPIO_GetChargeState() == CHG_STATE_OFF) {
			RUN_SetState(RUN_ST_SLEEP);
		}
		break;

	case RUN_ST_WAIT:
		// Wait for button to be released
		if (!GPIO_ButtonDown()) {
			SMB_SetStatusPowerOnMask(RUN_START_BTN);
			SMB_SetIndexedValue8(SMB_INDEX_WK_CTRL, 0);
			RUN_SetState(RUN_ST_POWER);

			// Clear any indication of a long press in case they held the button down for a long time
			(void) GPIO_ButtonLongPress();

			// Clear status register bit so device software that might be sampling it in order to
			// shut down won't mis-interpret it
			SMB_SetStatusBit(SMB_ST_PB_PRESS_MASK, false);
		}
		break;

	case RUN_ST_POWER:
		// Evaluate critical battery detection
		if (SMB_GetVB() < BATT_CRIT_MV) {
			if (crit_batt_timer == BATT_CRIT_TO_MSEC) {
				SMB_SetStatusBit(SMB_ST_CRIT_BATT_MASK, true);
			}
			if (crit_batt_timer != 0) {
				crit_batt_timer -= 1;
			}
		} else {
			// Hold timer in reset
			crit_batt_timer = BATT_CRIT_TO_MSEC;

			// Clear Critical battery mask if necessary
			if ((SMB_GetIndexedValue8(SMB_INDEX_STATUS) & SMB_ST_CRIT_BATT_MASK) == SMB_ST_CRIT_BATT_MASK) {
				SMB_SetStatusBit(SMB_ST_CRIT_BATT_MASK, false);
			}
		}

		// Look for conditions to sleep
		if (GPIO_ButtonLongPress()) {
			RUN_SetState(RUN_ST_POFF);
		} else if ((crit_batt_timer == 0) ||
		           (SMB_GetIndexedValue8(SMB_INDEX_SHDOWN) == SMB_SHUTDOWN_TRIG_MASK)) {
			if (GPIO_GetChargeState() == CHG_STATE_OFF) {
				RUN_SetState(RUN_ST_SLEEP);
			} else {
				RUN_SetState(RUN_ST_CHARGE);
			}
		}
		break;

	case RUN_ST_POFF:
		// Wait for power button to be released
		if (!GPIO_ButtonDown()) {
			if (GPIO_GetChargeState() == CHG_STATE_OFF) {
				RUN_SetState(RUN_ST_SLEEP);
			} else {
				RUN_SetState(RUN_ST_CHARGE);
			}

			// Clear indication of a long press
			(void) GPIO_ButtonLongPress();
		}
		break;

	case RUN_ST_SLEEP:
		break;
	}
}


static void RUN_SetState(uint8_t s)
{
	switch (s) {
	case RUN_ST_WAKE:
		GPIO_Init();
		LED_Init();
		TIMER_Init();
		GPIO_SetPower(PWR_VREF, true);
		break;

	case RUN_ST_CHARGE:
		// Switch off all outward facing signs we're alive in case we're coming from ST_POWER
		GPIO_SetPower(PWR_MAIN, false);
		LED_EnableBacklight(false);
		SMB_Suspend();

		// Clear any previous shutdown command
		SMB_SetIndexedValue8(SMB_INDEX_SHDOWN, 0);
		break;

	case RUN_ST_WAIT:
		// Switch on main power and setup for operation so they let go of the button
		GPIO_SetPower(PWR_MAIN, true);
		LED_EnableBacklight(true);
		break;

	case RUN_ST_POWER:
		// Switch on main power and setup for operation
		GPIO_SetPower(PWR_MAIN, true);
		LED_EnableBacklight(true);
		SMB_Resume();
		crit_batt_timer = BATT_CRIT_TO_MSEC;
		break;

	case RUN_ST_POFF:
		// Switch off all outward facing signs we're alive
		GPIO_SetPower(PWR_MAIN, false);
		LED_EnableBacklight(false);
		break;

	case RUN_ST_SLEEP:
		// Clear any previous shutdown command
		SMB_SetIndexedValue8(SMB_INDEX_SHDOWN, 0);

		// Make sure everything is off and potential triggering conditions disabled
		SMB_ShutDown();
		GPIO_SetPower(PWR_MAIN, false);
		GPIO_SetPower(PWR_VREF, false);
		LED_SetChargeState(CHG_STATE_OFF);
		LED_EnableBacklight(false);
		RTC_ClearAlarm();  // Clear any alarms that occurred while we were running
//		WD_Disable();
		break;
	}

	run_state = s;
}
