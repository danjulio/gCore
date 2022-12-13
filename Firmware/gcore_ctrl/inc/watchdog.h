/*
 * watchdog.h
 *
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
