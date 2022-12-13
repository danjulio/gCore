/*
 * timer.h
 *
 * Copyright (c) 2021 danjuliodesigns, LLC.  All rights reserved.
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
