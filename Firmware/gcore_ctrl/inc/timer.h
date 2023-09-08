/*
 * timer.h
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

#ifndef INC_TIMER_H_
#define INC_TIMER_H_

#include <stdint.h>



// =========================================================
// Constants
// =========================================================

// Timer basic tick interval
#define TIMER_TICK_USEC  200

// Number of sub-intervals (phases) in a mSec
#define TIMER_NUM_PHASES (1000 / TIMER_TICK_USEC)

// Evaluation phases
#define TIMER_EVAL_GPIO_PHASE  0
#define TIMER_EVAL_LED_PHASE   1
#define TIMER_EVAL_NVRAM_PHASE 2
#define TIMER_EVAL_RTC_PHASE   3
#define TIMER_EVAL_RUN_PHASE   4



// =========================================================
// API
// =========================================================
void TIMER_Init();
void TIMER_DelayMsec(uint8_t delay);
bool TIMER_FastTick();
bool TIMER_MsecTick(uint8_t phase);
uint32_t TIMER_GetMsec();
void TIMER_SetMsec(uint32_t t);


#endif /* INC_TIMER_H_ */
