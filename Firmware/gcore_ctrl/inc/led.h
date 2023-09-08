/*
 * led.h
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

#ifndef INC_LED_H_
#define INC_LED_H_

#include <stdint.h>


// =========================================================
// Constants
// =========================================================

// PWM Brightness channels
//  Charge LED : Channel 0
//  Backlight  : Channel 1
#define LED_CHARGE    1
#define LED_BACKLIGHT 0

// Blinking Charge LED PWM brightness
#define LED_CHG_BLNK_PWM      192

// Blink period (in units of LED module evaluation intervals)
#define LED_BLINK_COUNT       20

// Solid on Charge LED PWM brightness
#define LED_CHG_DONE_PWM      128

// Pulsed Charge LED PWM brightness parameters :
//    - LED_CHG_ON_PWM sets the maximum brightness of the pulse
//    - LED_CHG_PWM_STEP sets the increment or decrement value between each PWM value during
//      the pulse animation.  It should be less than or equal to LED_CHG_ON_PWM/LED_CHG_NUM_STEPS.
//    - LED_CHG_NUM_STEPS sets the number of increments or decrements between minimum and
//      maximum brightness (number of eval cycles for each half of the pulse).
#define LED_CHG_ON_PWM         250
#define LED_CHG_PWM_STEP       2
#define LED_CHG_NUM_STEPS      120
#define LED_CHG_OFF_PWM        (LED_CHG_ON_PWM - LED_CHG_PWM_STEP*LED_CHG_NUM_STEPS)



// =========================================================
// API
// =========================================================
void LED_Init();
void LED_Eval();
void LED_EnableBacklight(bool en);
void LED_SetPWM(uint8_t ch, uint8_t val);
uint8_t LED_GetPWM(uint8_t ch);
void LED_SetChargeState(uint8_t chg_state);


#endif /* INC_LED_H_ */
