/*
 * gCore control library.  Provides access to the EFM8 co-processor for power control,
 * monitoring, Backlight control, RTC and NVRAM.
 *
 * Written by Dan Julio
 *
 * Copyright 2021 danjuliodesigns, LLC
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this
 * software and associated documentation files (the "Software"), to deal in the Software
 * without restriction, including without limitation the rights to use, copy, modify,
 * merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies
 * or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
 * PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
 * OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */
#ifndef _GCORE_
#define _GCORE_

#include <Arduino.h>
#include "gCore.h"
#include <sys/time.h>



/*
 * gCore Constants
*/

// I2C address
#define GCORE_I2C_ADDR    0x12

// gCore pins
#define GCORE_I2C_SDA     21
#define GCORE_I2C_SCL     22

// gCore Max frequency
#define GCORE_I2C_FREQ    100000

// Register address offsets
#define GCORE_NVRAM_BASE  0x0000
#define GCORE_REG_BASE    0x1000

// Region sizes
#define GCORE_NVRAM_FULL_LEN 0x1000
#define GCORE_NVRAM_BCKD_LEN 0x0400
#define GCORE_REG_LEN        0x1F

// 8-bit Register offsets
#define GCORE_REG_ID      0x00
#define GCORE_REG_VER     0x01
#define GCORE_REG_STATUS  0x02
#define GCORE_REG_GPIO    0x03
#define GCORE_REG_VU      0x04
#define GCORE_REG_IU      0x06
#define GCORE_REG_VB      0x08
#define GCORE_REG_IL      0x0A
#define GCORE_REG_TEMP    0x0C
#define GCORE_REG_BL      0x0E
#define GCORE_REG_WK_CTRL 0x0F
#define GCORE_REG_SHDOWN  0x10
#define GCORE_REG_PWR_TM  0x11
#define GCORE_REG_NV_CTRL 0x12
#define GCORE_REG_TIME    0x13
#define GCORE_REG_ALARM   0x17
#define GCORE_REG_CORR    0x1B


//
// gCore FW ID
//
#define GCORE_FW_ID       0x01


//
// Register bitmasks
//

// GPIO register masks
#define GCORE_GPIO_SD_CARD_MASK  0x08
#define GCORE_GPIO_PWR_BTN_MASK  0x04
#define GCORE_GPIO_CHG_MASK      0x03
#define GCORE_GPIO_CHG_1_MASK    0x02
#define GCORE_GPIO_CHG_0_MASK    0x01

// Status register masks
#define GCORE_ST_CRIT_BATT_MASK  0x80
#define GCORE_ST_PB_PRESS_MASK   0x10
#define GCORE_ST_PWR_ON_RSN_MASK 0x07

// Status power-on reason bit masks
#define GCORE_PWR_ON_CHG_MASK    0x04
#define GCORE_PWR_ON_ALARM_MASK  0x02
#define GCORE_PWR_ON_BTN_MASK    0x01

// NVRAM Flash register busy mask (RO)
#define GCORE_NVRAM_BUSY_MASK    0x01
#define GCORE_NVRAM_IDLE_MASK    0x00

// Wakeup Control register masks
#define GCORE_WK_CHRG_DONE_MASK  0x04
#define GCORE_WK_CHRG_START_MASK 0x02
#define GCORE_WK_ALARM_MASK      0x01


//
// Register special trigger values
//

// NVRAM Flash Register triggers (WO)
#define GCORE_NVRAM_WR_TRIG      'W'
#define GCORE_NVRAM_RD_TRIG      'R'

// Shutdown Register trigger (WO)
#define GCORE_SHUTDOWN_TRIG       0x0F


//
// Charge status bit values
//
#define GCORE_CHG_IDLE            0
#define GCORE_CHG_ACTIVE          1
#define GCORE_CHG_DONE            2
#define GCORE_CHG_FAULT           3


/*
 * RTC constants
*/

// Convenience macros to convert to and from tm years 
#define  tmYearToCalendar(Y) ((Y) + 1970)  // full four digit year 
#define  CalendarYrToTm(Y)   ((Y) - 1970)
#define  tmYearToY2k(Y)      ((Y) - 30)    // offset is from 2000
#define  y2kYearToTm(Y)      ((Y) + 30)


/*
 * Time structures
*/
typedef struct  {
	uint8_t Second;       // 0 - 59 
	uint8_t Minute;       // 0 - 59
	uint8_t Hour;         // 0 - 23
	uint8_t Wday;         // 1 - 7; day of week, sunday is day 1
	uint8_t Day;          // 1 - 31
	uint8_t Month;        // 1 - 12
	uint8_t Year;         // offset from 1970; 
} tmElements_t;



/*
 * Class definition
 */
class gCore {
	public:
		gCore();
		
		void begin();
		
		// gCore low level API
		bool gcore_get_reg8(uint8_t offset, uint8_t* dat);
		bool gcore_set_reg8(uint8_t offset, uint8_t dat);
		bool gcore_get_reg16(uint8_t offset, uint16_t* dat);
		bool gcore_set_reg16(uint8_t offset, uint16_t dat);
		bool gcore_get_reg32(uint8_t offset, uint32_t* dat);
		bool gcore_set_reg32(uint8_t offset, uint32_t dat);
		
		bool gcore_set_wakeup_bit(uint8_t mask, bool en);
		
		bool gcore_get_nvram_byte(uint16_t offset, uint8_t* dat);
		bool gcore_set_nvram_byte(uint16_t offset, uint8_t dat);
		bool gcore_get_nvram_bytes(uint16_t offset, uint8_t* dat, uint16_t len);
		bool gcore_set_nvram_bytes(uint16_t offset, uint8_t* dat, uint16_t len);
		
		bool gcore_get_time_secs(uint32_t* s);
		bool gcore_set_time_secs(uint32_t s);
		bool gcore_get_alarm_secs(uint32_t* s);
		bool gcore_set_alarm_secs(uint32_t s);
		bool gcore_get_corr_secs(uint32_t* s);
		bool gcore_set_corr_secs(uint32_t s);
		
		// Power management API
		void power_set_brightness(int percent);
		void power_set_button_short_press_msec(int msec);
		bool power_button_pressed();
		bool power_get_sdcard_present();
		void power_off();

		// RTC support API
		time_t rtc_get_time_secs();
		bool rtc_set_time_secs(time_t t);
		void rtc_read_time(tmElements_t* tm);
		bool rtc_write_time(const tmElements_t tm);
		
		time_t rtc_get_alarm_secs();
		bool rtc_set_alarm_secs(time_t t);
		void rtc_read_alarm(tmElements_t* tm);
		bool rtc_write_alarm(const tmElements_t tm);
		
		bool rtc_enable_alarm(bool en);
		bool rtc_get_alarm_enable(bool* en);
		
		void rtc_breakTime(time_t timeInput, tmElements_t* tm);
		time_t rtc_makeTime(const tmElements_t tm);
};

#endif /* _GCORE_ */