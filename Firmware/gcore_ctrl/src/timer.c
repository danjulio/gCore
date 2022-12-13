/*
 * timer.c
 *
 * Copyright (c) 2021 danjuliodesigns, LLC.  All rights reserved.
 */
#include <SI_EFM8SB2_Register_Enums.h>
#include "timer.h"
#include "watchdog.h"



// =========================================================
// Variables
// =========================================================
volatile bit timer_tick;
volatile uint8_t timer_phase_tick[TIMER_NUM_PHASES];
volatile uint8_t timer_phase;
volatile uint32_t timer_msec;



// =========================================================
// API
// =========================================================
void TIMER_Init()
{
	// Disable TIMER2 interrupts
	IE_ET2 = 0;

	// Clear tick flags and reset phase
	timer_tick = 0;
	for (timer_phase=0; timer_phase<TIMER_NUM_PHASES; timer_phase++) {
		timer_phase_tick[timer_phase] = 0;
	}
	timer_phase = 0;
	timer_msec = 0;

	// Enable TIMER2 interrupts
	IE_ET2 = 1;
}


// For use before timer evaluation activities
void TIMER_DelayMsec(uint8_t delay)
{
	uint16_t count = delay * TIMER_NUM_PHASES;

	while (count != 0) {
		if (timer_tick) {
			timer_tick = 0;
			count--;
			WD_Reset();
		}
	}
}


// Must only be called once per evaluation
bool TIMER_FastTick()
{
	if (timer_tick == 1) {
		timer_tick = 0;
		return true;
	}

	return false;
}


// Must only be called once per evaluation per phase
bool TIMER_MsecTick(uint8_t phase)
{
	bool ret = false;

	if (phase < TIMER_NUM_PHASES) {
		IE_ET2 = 0;
		if (timer_phase_tick[phase] != 0) {
			timer_phase_tick[phase] = 0;
			ret = true;
		}
		IE_ET2 = 1;
	}

	return ret;
}


uint32_t TIMER_GetMsec()
{
	uint32_t t;

	IE_ET2 = 0;
	t = timer_msec;
	IE_ET2 = 1;

	return t;
}


void TIMER_SetMsec(uint32_t t)
{
	IE_ET2 = 0;
	timer_msec = t;
	IE_ET2 = 1;
}



// =========================================================
// TIMER2 ISR
// =========================================================
SI_INTERRUPT (TIMER2_ISR, TIMER2_IRQn)
{
	// Tick!
	timer_tick = 1;

	// Set phase tick and increment phase for next time
	timer_phase_tick[timer_phase] = 1;
	if (++timer_phase == TIMER_NUM_PHASES) {
		timer_phase = 0;
		timer_msec++;
	}

	// Clear overflow interrupt bit
	TMR2CN0 &= ~(TMR2CN0_TF2L__BMASK | TMR2CN0_TF2H__BMASK);
}

