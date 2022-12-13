/*
 * rtc.h
 *
 * Copyright (c) 2021 danjuliodesigns, LLC.  All rights reserved.
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
uint32_t RTC_GetTime();
void RTC_SetTimeTrigger();
bool RTC_SawAlarm();
void RTC_ClearAlarm();

#endif /* INC_RTC_H_ */
