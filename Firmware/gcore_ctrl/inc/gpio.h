/*
 * gpio.h
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

#ifndef INC_GPIO_H_
#define INC_GPIO_H_

#include <SI_EFM8SB2_Register_Enums.h>
#include <stdbool.h>
#include <stdint.h>



// =========================================================
// Constants
// =========================================================

//
// Charge State
//
#define CHG_STATE_OFF     0
#define CHG_STATE_ON      1
#define CHG_STATE_DONE    2
#define CHG_STATE_FAULT   3

//
// Power Enable Channels
//
#define PWR_VREF     0
#define PWR_MAIN     1



//
// GPIO Pins
//

// Power Button input
SI_SBIT(BUTTON, SFR_P0, 7);

// Charge inputs
SI_SBIT(CHG_STAT_1, SFR_P0, 5);
SI_SBIT(CHG_STAT_2, SFR_P0, 6);

// Main 3.3V Power rail enable
SI_SBIT(MAIN_PWR_EN, SFR_P1, 3);

// VREF and battery voltage divider enable
SI_SBIT(VREF_PWR_EN, SFR_P1, 6);

// SD Card Sense input
SI_SBIT(SD_SENSE, SFR_P1, 0);

// Diagnostic output for debugging (C2D)
SI_SBIT(DIAG, SFR_P2, 7);


// =========================================================
// API
// =========================================================
void GPIO_Init();
void GPIO_Eval();

void GPIO_SetPower(uint8_t ch, bool en);
bool GPIO_GetPower(uint8_t ch);

uint8_t GPIO_GetChargeState();
bool GPIO_BtnWakeupOccurred();
bool GPIO_ChgWakeupOccurred();
bool GPIO_ButtonDown();
bool GPIO_ButtonLongPress();
bool GPIO_ButtonShortPeriodExpired();


#endif /* INC_GPIO_H_ */
