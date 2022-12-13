/*
 * watchdog.c
 *
 * Main system watchdog timer control module. WD_Reset() must be called more
 * frequently than the minimum watchdog timeout or the module will reset the
 * processor.
 *
 *
 *
 */
#include <SI_EFM8SB2_Register_Enums.h>
#include "watchdog.h"



//-----------------------------------------------------------------------------
// Variables
//-----------------------------------------------------------------------------



//-----------------------------------------------------------------------------
// API routines
//-----------------------------------------------------------------------------
void WD_Disable()
{
	// Disable the watchdog - this routine should be run when interrupts are disabled
	PCA0MD &= ~PCA0MD_WDTE__BMASK;
}


void WD_Init()
{
	// Disable the watchdog
	PCA0MD &= ~PCA0MD_WDTE__BMASK;

	// Configure our watchdog timeout
	PCA0CPL5 = PCA_PRELOAD;

	// Enable the watchdog
	PCA0MD |= PCA0MD_WDTE__BMASK;

	// Reset the watchdog timer
	PCA0CPH5 = 0;
}


void WD_Reset()
{
	// Reset the watchdog timer
	PCA0CPH5 = 0;
}
