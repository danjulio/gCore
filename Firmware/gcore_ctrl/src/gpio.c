/*
 * gpio.c
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
#include "config.h"
#include "gpio.h"
#include "smbus.h"



// =========================================================
// Variables
// =========================================================

// Button state
bit button_prev;
bit button_down;
bit button_released;
bit button_long_press;

// Charge state
bit chg_state_1_prev;
bit chg_state_2_prev;
bit chg_state_1;  // Charge in progress
bit chg_state_2;  // Charge connected; complete

// Device power state
bit gpio_pwr_main;
bit gpio_pwr_vref;

// Other GPIO state
bit gpio_sd_prev;
bit gpio_sd_debounced;

// Evaluation timer
static uint8_t SI_SEG_IDATA gpio_timer;

// Button press timer (EVAL_GPIO_MSEC intervals)
static uint16_t SI_SEG_IDATA gpio_button_timer;

// Button short press detection value (EVAL_GPIO_MSEC intervals)
static uint8_t SI_SEG_IDATA gpio_button_short_val;



// =========================================================
// Forward Declarations
// =========================================================
static void _gpio_sample_inputs();


// =========================================================
// API
// =========================================================
void GPIO_Init()
{
	button_prev = 0;
	button_down = 0;
	button_released = 0;
	button_long_press = 0;

	chg_state_1_prev = 0;
	chg_state_2_prev = 0;
	chg_state_1 = 0;
	chg_state_2 = 0;

	gpio_pwr_main = 0;
	gpio_pwr_vref = 0;

	gpio_sd_prev = 0;
	gpio_sd_debounced = 0;

	gpio_timer = EVAL_GPIO_MSEC;
	gpio_button_timer = 0;
	gpio_button_short_val = SMB_GetIndexedValue8(SMB_INDEX_PWR_TM);

	// Default power off
	MAIN_PWR_EN = 0;
	VREF_PWR_EN = 0;

	// Get initial input states (debounced)
	_gpio_sample_inputs();
	_gpio_sample_inputs();
}


void GPIO_Eval()
{
	uint8_t temp_reg;

	if (--gpio_timer == 0) {
		gpio_timer = EVAL_GPIO_MSEC;

		// Check for updated short-press timeout
		temp_reg = SMB_GetIndexedValue8(SMB_INDEX_PWR_TM);
		if (temp_reg != gpio_button_short_val) {
			gpio_button_short_val = temp_reg;
		}

		_gpio_sample_inputs();

		// Button press evaluation
		if (button_down) {
			// Look for long press detection
			if (gpio_button_timer < (BTN_PRESS_LONG_MSEC / EVAL_GPIO_MSEC)) {
				// Increment the button down timer
				gpio_button_timer += 1;

				if (gpio_button_timer == (BTN_PRESS_LONG_MSEC / EVAL_GPIO_MSEC)) {
					// Long press detected - will cause power down and sleep
					button_long_press = 1;
				}
			}
		} else if (button_released) {
			if ((gpio_button_timer >= gpio_button_short_val) &&
			    (gpio_button_timer < (BTN_PRESS_LONG_MSEC / EVAL_GPIO_MSEC))) {
				// Short press detected
				SMB_SetStatusBit(SMB_ST_PB_PRESS_MASK, true);
			}
		}

		if (!button_down) {
			// Hold timer in reset
			gpio_button_timer = 0;
		}

		// Build up current GPIO register value and load SMBUS
		temp_reg =
				((chg_state_1 == 1) ? SMB_GPIO_CHG_0_MASK : 0x00) |
				((chg_state_2 == 1) ? SMB_GPIO_CHG_1_MASK : 0x00) |
				((button_down == 1) ? SMB_GPIO_PWR_BTN_MASK : 0x00) |
				((gpio_sd_debounced == 1) ? SMB_GPIO_SD_CARD_MASK : 0x00);

		SMB_SetIndexedValue8(SMB_INDEX_GPIO, temp_reg);
	}
}


void GPIO_SetPower(uint8_t ch, bool en)
{
	switch (ch) {
	case PWR_MAIN:
		gpio_pwr_main = en ? 1 : 0;
		MAIN_PWR_EN = gpio_pwr_main;
		break;

	case PWR_VREF:
		gpio_pwr_vref = en ? 1 : 0;
		VREF_PWR_EN = gpio_pwr_vref;
		break;
	}
}


bool GPIO_GetPower(uint8_t ch)
{
	bool en = false;

	switch (ch) {
	case PWR_MAIN:
		en = gpio_pwr_main;
		break;

	case PWR_VREF:
		en = gpio_pwr_vref;
		break;
	}

	return en;
}


uint8_t GPIO_GetChargeState()
{
	uint8_t st;

	st = (chg_state_1 == 1) ? 0x01 : 0x00;
	st |= (chg_state_2 == 1) ? 0x02 : 0x00;

	switch (st) {
	case 0:
		st = CHG_STATE_OFF;
		break;
	case 1:
		st = CHG_STATE_ON;
		break;
	case 2:
		st = CHG_STATE_DONE;
		break;
	default:
		st = CHG_STATE_FAULT;
	}

	return st;
}


bool GPIO_BtnWakeupOccurred()
{
	return (BUTTON == 0);
}


bool GPIO_ChgWakeupOccurred()
{
	return ((CHG_STAT_1 == 0) || (CHG_STAT_2 == 0));
}


bool GPIO_ButtonDown()
{
	return (button_down == 1);
}


bool GPIO_ButtonLongPress()
{
	if (button_long_press == 1) {
		button_long_press = 0;
		gpio_button_timer = 0;
		return true;
	}

	return false;
}


bool GPIO_ButtonShortPeriodExpired()
{
	return (gpio_button_timer > gpio_button_short_val);
}




// =========================================================
// Internal functions
// =========================================================

// Called on GPIO sample interval
void _gpio_sample_inputs()
{
	bool cur_input;

	// Button - only evaluate state and set released here
	cur_input = (BUTTON == 0) ? true : false;
	button_released = 0;
	if ((button_down == 1) && (button_prev == 1) && !cur_input) {
		button_released = 1;
	}
	button_down = ((button_prev == 1) && cur_input) ? 1 : 0;
	button_prev = cur_input ? 1 : 0;

	// Charge state
	cur_input = (CHG_STAT_1 == 0) ? true : false;
	chg_state_1 = ((chg_state_1_prev == 1) && cur_input) ? 1 : 0;
	chg_state_1_prev = cur_input ? 1 : 0;

	cur_input = (CHG_STAT_2 == 0) ? true : false;
	chg_state_2 = ((chg_state_2_prev == 1) && cur_input) ? 1 : 0;
	chg_state_2_prev = cur_input ? 1 : 0;

	// SD Card input
	cur_input = (SD_SENSE == 0) ? true : false;
	gpio_sd_debounced = ((gpio_sd_prev == 1) && cur_input) ? 1 : 0;
	gpio_sd_prev = cur_input ? 1 : 0;
}

