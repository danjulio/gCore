/*
 * led.c
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
#include "led.h"
#include "smbus.h"
#include "timer.h"



// =========================================================
// Private Constants
// =========================================================
// Charge States for display
#define LED_CHG_OFF      0
#define LED_CHG_ON       1
#define LED_CHG_FADE_ON  2
#define LED_CHG_FADE_OFF 3
#define LED_CHG_BLNK_ON  4
#define LED_CHG_BLNK_OFF 5



// =========================================================
// Variables
// =========================================================
static bit led_en_backlight;

static uint8_t led_charge_state;
static uint8_t led_prev_chg;
static uint8_t led_timer;
static uint8_t led_anim_count;
static uint8_t led_chg_pwm_val;

static uint8_t led_bl_val;
static uint8_t led_chg_val;




// =========================================================
// API
// =========================================================
void LED_Init()
{
	led_en_backlight = 0;
	led_charge_state = LED_CHG_OFF;
	led_prev_chg = CHG_STATE_OFF;
	led_timer = EVAL_LED_MSEC;
	led_anim_count = LED_BLINK_COUNT;

	LED_SetPWM(LED_BACKLIGHT, 0);
	LED_SetPWM(LED_CHARGE, 0);
}


void LED_Eval()
{
	uint8_t cur_val;

	if (--led_timer == 0) {
		led_timer = EVAL_LED_MSEC;

		// Check for updated Backlight PWM value
		cur_val = SMB_GetIndexedValue8(SMB_INDEX_BL_PWM);
		if ((cur_val != led_bl_val) && led_en_backlight) {
			LED_SetPWM(LED_BACKLIGHT, cur_val);
		}

		// Check for updated charge state
		cur_val = GPIO_GetChargeState();
		if (cur_val != led_prev_chg) {
			LED_SetChargeState(cur_val);
			led_prev_chg = cur_val;
		}

		// Evaluate Charge LED dynamic states
		switch (led_charge_state) {
		case LED_CHG_FADE_ON:
			led_chg_pwm_val += LED_CHG_PWM_STEP;
			LED_SetPWM(LED_CHARGE, led_chg_pwm_val);
			if (--led_anim_count == 0) {
				led_anim_count = LED_CHG_NUM_STEPS;
				led_charge_state = LED_CHG_FADE_OFF;
			}
			break;

		case LED_CHG_FADE_OFF:
			led_chg_pwm_val -= LED_CHG_PWM_STEP;
			LED_SetPWM(LED_CHARGE, led_chg_pwm_val);
			if (--led_anim_count == 0) {
				led_anim_count = LED_CHG_NUM_STEPS;
				led_charge_state = LED_CHG_FADE_ON;
			}
			break;

		case LED_CHG_BLNK_ON:
			if (--led_anim_count == 0) {
				led_charge_state = LED_CHG_BLNK_OFF;
				led_anim_count = LED_BLINK_COUNT;
				LED_SetPWM(LED_CHARGE, 0);
			}
			break;

		case LED_CHG_BLNK_OFF:
			if (--led_anim_count == 0) {
				led_charge_state = LED_CHG_BLNK_ON;
				led_anim_count = LED_BLINK_COUNT;
				LED_SetPWM(LED_CHARGE, LED_CHG_BLNK_PWM);
			}
			break;
		}
	}
}


void LED_EnableBacklight(bool en)
{
	led_en_backlight = en;
	if (en) {
		led_bl_val = SMB_GetIndexedValue8(SMB_INDEX_BL_PWM);
	} else {
		led_bl_val = 0;
	}
	LED_SetPWM(LED_BACKLIGHT, led_bl_val);
}


void LED_SetPWM(uint8_t ch, uint8_t val)
{
	uint8_t pwm_val = 255 - val;

	switch (ch) {
	case LED_CHARGE:
		led_chg_val = val;
		PCA0CPH0 = pwm_val;
		if (val == 0) {
			PCA0CPM0 &= ~PCA0CPM0_ECOM__BMASK;         // Clear ECOM0 to disable output
		} else {
			if ((PCA0CPM0 & PCA0CPM0_ECOM__BMASK) != PCA0CPM0_ECOM__BMASK) {
				PCA0CPM0 |= PCA0CPM0_ECOM__BMASK;      // Set ECOM0 when output is not 0
			}
		}
		break;

	case LED_BACKLIGHT:
		led_bl_val = val;
		PCA0CPH1 = pwm_val;
		if (val == 0) {
			PCA0CPM1 &= ~PCA0CPM1_ECOM__BMASK;
		} else {
			if ((PCA0CPM1 & PCA0CPM1_ECOM__BMASK) != PCA0CPM1_ECOM__BMASK) {
				PCA0CPM1 |= PCA0CPM1_ECOM__BMASK;
			}
		}
		break;
	}
}


uint8_t LED_GetPWM(uint8_t ch)
{
	uint8_t val = 0;

	switch (ch) {
	case LED_CHARGE:
		val = led_chg_val;
		break;

	case LED_BACKLIGHT:
		val = led_bl_val;
		break;
	}

	return val;
}


void LED_SetChargeState(uint8_t chg_state)
{
	switch (chg_state) {
	case CHG_STATE_OFF:
		// Switch LED off
		led_charge_state = LED_CHG_OFF;
		LED_SetPWM(LED_CHARGE, 0);
		break;

	case CHG_STATE_ON:
		// Start fade animation at lowest intensity
		led_charge_state = LED_CHG_FADE_ON;
		led_anim_count = LED_CHG_NUM_STEPS;
		led_chg_pwm_val = LED_CHG_OFF_PWM;
		LED_SetPWM(LED_CHARGE, led_chg_pwm_val);
		break;

	case CHG_STATE_DONE:
		// Switch LED Solid on
		led_charge_state = LED_CHG_ON;
		LED_SetPWM(LED_CHARGE, LED_CHG_DONE_PWM);
		break;

	case CHG_STATE_FAULT:
		// Start blink animation with LED ON
		led_charge_state = LED_CHG_BLNK_ON;
		led_anim_count = LED_BLINK_COUNT;
		LED_SetPWM(LED_CHARGE, LED_CHG_BLNK_PWM);
		break;
	}
}
