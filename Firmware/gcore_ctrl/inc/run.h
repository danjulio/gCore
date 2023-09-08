/*
 * run.h
 *
 * Header for main run logic.
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
