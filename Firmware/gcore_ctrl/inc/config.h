/*
 * config.h
 *
 *  Global default system configuration parameters
 *
 * Copyright (c) 2021-2023 danjuliodesigns, LLC.  All rights reserved.
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

#ifndef INC_CONFIG_H_
#define INC_CONFIG_H_

#include <SI_EFM8SB2_Register_Enums.h>

// =========================================================
// Firmware version
//  FW_ID should be limited to 8-bits
//  FW_VER fields should be limited to 4-bits
#define FW_ID               1
#define FW_VER_MAJOR        1
#define FW_VER_MINOR        2



// =========================================================
// Hardware configuration
//

//
// External battery resistor divider scale factor (x100)
//
#define BATT_MULT_FACTOR    502



// =========================================================
// Default hard-coded operating parameters
//

//
// Battery Voltage thresholds
//
#define BATT_VREF_MV        1250
#define BATT_CRIT_MV        3300
#define BATT_TURN_ON_MIN_MV 3500


//
// Evaluation intervals
//
#define EVAL_BATT_MSEC  100
#define EVAL_GPIO_MSEC  10
#define EVAL_LED_MSEC   10


//
// Critical battery shutdown interval
//
#define BATT_CRIT_TO_MSEC 10000


//
// Default Short button press interval
//
//   BTN_PRESS_SHORT_MSEC / EVAL_GPIO_MSEC must fit in a uint8_t
//
#define BTN_PRESS_SHORT_MSEC 100


//
// Long button press shutdown interval
//   BTN_PRESS_LONG_MSEC / EVAL_GPIO_MSEC must fit in a uint16_t
//
#define BTN_PRESS_LONG_MSEC 5000


//
// Default backlight PWM brightness
//
#define BL_PWM_DEFAULT      128


#endif /* INC_CONFIG_H_ */
