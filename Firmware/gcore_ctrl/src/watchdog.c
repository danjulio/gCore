/*
 * watchdog.c
 *
 * Main system watchdog timer control module. WD_Reset() must be called more
 * frequently than the minimum watchdog timeout or the module will reset the
 * processor.
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
