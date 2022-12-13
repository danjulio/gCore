/*
 * run.h
 *
 * Header for main run logic.
 *
 * Copyright (c) 2021 danjuliodesigns, LLC.  All rights reserved.
 *
 *
 */

#ifndef INC_RUN_H_
#define INC_RUN_H_

#include <stdint.h>


// =========================================================
// Constants
// =========================================================

// Start-up reason bit-mask
#define RUN_START_BTN      0x01
#define RUN_START_ALARM    0x02
#define RUN_START_CHG      0x04



// =========================================================
// API
// =========================================================
void RUN_Task(uint8_t startup_mask);

#endif /* INC_RUN_H_ */
