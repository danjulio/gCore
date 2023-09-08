/*
 * rtc.h
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

#ifndef INC_RTC_H_
#define INC_RTC_H_

#include <stdbool.h>
#include <stdint.h>



// =========================================================
// Constants
// =========================================================



// =========================================================
// API
// =========================================================
void RTC_Init(); // Only called at power-on
void RTC_Eval();
void RTC_SetTimeTrigger();
bool RTC_SawAlarm();
void RTC_ClearAlarm();

#endif /* INC_RTC_H_ */
