/*
 * watchdog.h
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

#ifndef INC_WATCHDOG_H_
#define INC_WATCHDOG_H_

#include <stdint.h>



// =========================================================
// Constants
// =========================================================

// Timeout value controlled by PCA0CPL5
//   Min Timeout = 256 * PCA0CPL5 * PCA_CLOCK_PERIOD
//   PCA_CLOCK_PERIOD = 1/6.125 MHz = 163.265 nSec
//
//   We choose the maximum by setting PCA0CPL5 = 255 =>
//   Min Timeout = 10.658 mSec
#define PCA_PRELOAD 255



// =========================================================
// API
// =========================================================
void WD_Disable();
void WD_Init();
void WD_Reset();

#endif /* INC_WATCHDOG_H_ */
