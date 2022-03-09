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
#include "gCore.h"
#include <Wire.h>


/*
 * Private constants
 */

// Leap year calulator expects year argument as years offset from 1970
#define LEAP_YEAR(Y)     ( ((1970+(Y))>0) && !((1970+(Y))%4) && ( ((1970+(Y))%100) || !((1970+(Y))%400) ) )

// Useful time constants
#define SECS_PER_MIN  ((time_t)(60UL))
#define SECS_PER_HOUR ((time_t)(3600UL))
#define SECS_PER_DAY  ((time_t)(SECS_PER_HOUR * 24UL))
#define DAYS_PER_WEEK ((time_t)(7UL))
#define SECS_PER_WEEK ((time_t)(SECS_PER_DAY * DAYS_PER_WEEK))
#define SECS_PER_YEAR ((time_t)(SECS_PER_DAY * 365UL)) // TODO: ought to handle leap years
#define SECS_YR_2000  ((time_t)(946684800UL)) // the time at the start of y2k


static const uint8_t monthDays[]={31,28,31,30,31,30,31,31,30,31,30,31}; // API starts months from 1, this array starts from 0


/*
 * Class Implementation
 */
gCore::gCore()
{
}


void gCore::begin()
{
	Wire.begin(GCORE_I2C_SDA, GCORE_I2C_SCL, GCORE_I2C_FREQ);
}


bool gCore::gcore_get_reg8(uint8_t offset, uint8_t* dat)
{
	uint8_t ret;
	uint16_t reg;
	
	if (offset >= GCORE_REG_LEN) {
		return false;
	}
	
	reg = GCORE_REG_BASE + offset;
	
	Wire.beginTransmission(GCORE_I2C_ADDR);
	Wire.write((reg>>8) & 0xFF);
	Wire.write(reg & 0xFF);
	if (Wire.endTransmission() != 0) {
		return false;
	}
	
	ret = Wire.requestFrom(GCORE_I2C_ADDR, 1);
	*dat = Wire.read();
  
	return (ret == 1);
}


bool gCore::gcore_set_reg8(uint8_t offset, uint8_t dat)
{
	uint16_t reg;
	
	if (offset >= GCORE_REG_LEN) {
		return false;
	}
	
	reg = GCORE_REG_BASE + offset;
	
	Wire.beginTransmission(GCORE_I2C_ADDR);
	Wire.write((reg>>8) & 0xFF);
	Wire.write(reg & 0xFF);
	Wire.write(dat);
	if (Wire.endTransmission() != 0) {
		return false;
	}
}


bool gCore::gcore_get_reg16(uint8_t offset, uint16_t* dat)
{
	uint8_t ret;
	uint16_t reg;
	
	if (offset >= GCORE_REG_LEN) {
		return false;
	}
	
	reg = GCORE_REG_BASE + offset;
	
	Wire.beginTransmission(GCORE_I2C_ADDR);
	Wire.write((reg>>8) & 0xFF);
	Wire.write(reg & 0xFF);
	if (Wire.endTransmission() != 0) {
		return false;
	}
	
	ret = Wire.requestFrom(GCORE_I2C_ADDR, 2);
	*dat = Wire.read() << 8;
	*dat |= Wire.read();
  
	return (ret == 2);
}


bool gCore::gcore_set_reg16(uint8_t offset, uint16_t dat)
{
	uint16_t reg;
	
	if (offset >= GCORE_REG_LEN) {
		return false;
	}
	
	reg = GCORE_REG_BASE + offset;
	
	Wire.beginTransmission(GCORE_I2C_ADDR);
	Wire.write((reg>>8) & 0xFF);
	Wire.write(reg & 0xFF);
	Wire.write(dat >> 8);
	Wire.write(dat & 0xFF);
	if (Wire.endTransmission() != 0) {
		return false;
	}
}


bool gCore::gcore_get_reg32(uint8_t offset, uint32_t* dat)
{
	uint8_t ret;
	uint16_t reg;
	
	if (offset >= GCORE_REG_LEN) {
		return false;
	}
	
	reg = GCORE_REG_BASE + offset;
	
	Wire.beginTransmission(GCORE_I2C_ADDR);
	Wire.write((reg>>8) & 0xFF);
	Wire.write(reg & 0xFF);
	if (Wire.endTransmission() != 0) {
		return false;
	}
	
	ret = Wire.requestFrom(GCORE_I2C_ADDR, 4);
	*dat = Wire.read() << 24;
	*dat |= Wire.read() << 16;
	*dat |= Wire.read() << 8;
	*dat |= Wire.read();
  
	return (ret == 4);
}


bool gCore::gcore_set_reg32(uint8_t offset, uint32_t dat)
{
	uint16_t reg;
	
	if (offset >= GCORE_REG_LEN) {
		return false;
	}
	
	reg = GCORE_REG_BASE + offset;
	
	Wire.beginTransmission(GCORE_I2C_ADDR);
	Wire.write((reg>>8) & 0xFF);
	Wire.write(reg & 0xFF);
	Wire.write(dat >> 24);
	Wire.write((dat >> 16) & 0xFF);
	Wire.write((dat >> 8) & 0xFF);
	Wire.write(dat & 0xFF);
	if (Wire.endTransmission() != 0) {
		return false;
	}
}


bool gCore::gcore_set_wakeup_bit(uint8_t mask, bool en)
{
	uint8_t val;
	
	// Perform a read-modify-write operation
	if (gcore_get_reg8(GCORE_REG_WK_CTRL, &val)) {
		if (en) {
			val | mask;
		} else {
			val &= ~mask;
		}
		return gcore_set_reg8(GCORE_REG_WK_CTRL, val);
	}
	
	return false;
}


bool gCore::gcore_get_nvram_byte(uint16_t offset, uint8_t* dat)
{
	uint8_t ret;
	
	if (offset >= GCORE_NVRAM_FULL_LEN) {
		return false;
	}
	
	Wire.beginTransmission(GCORE_I2C_ADDR);
	Wire.write((offset>>8) & 0xFF);
	Wire.write(offset & 0xFF);
	if (Wire.endTransmission() != 0) {
		return false;
	}
	
	ret = Wire.requestFrom(GCORE_I2C_ADDR, 1);
	*dat = Wire.read();
  
	return (ret == 1);
}


bool gCore::gcore_set_nvram_byte(uint16_t offset, uint8_t dat)
{
	if (offset >= GCORE_NVRAM_FULL_LEN) {
		return false;
	}
	
	Wire.beginTransmission(GCORE_I2C_ADDR);
	Wire.write((offset>>8) & 0xFF);
	Wire.write(offset & 0xFF);
	Wire.write(dat);
	if (Wire.endTransmission() != 0) {
		return false;
	}
}


bool gCore::gcore_get_nvram_bytes(uint16_t offset, uint8_t* dat, uint16_t len)
{
	uint8_t read_len;
	uint8_t ret;
	
	if ((offset+len) > GCORE_NVRAM_FULL_LEN) {
		return false;
	}
	
	while (len) {
		if (len > I2C_BUFFER_LENGTH) {
			read_len = I2C_BUFFER_LENGTH;
		} else {
			read_len = len;
		}
		
		Wire.beginTransmission(GCORE_I2C_ADDR);
		Wire.write((offset>>8) & 0xFF);
		Wire.write(offset & 0xFF);
		if (Wire.endTransmission() != 0) {
			return false;
		}
		
		ret = Wire.requestFrom((uint16_t) GCORE_I2C_ADDR, read_len);
		if (ret != read_len) {
			return false;
		}
		len -= read_len;
		while (read_len--) {
			*dat++ = Wire.read();
		}
	}
	
	return true;
}


bool gCore::gcore_set_nvram_bytes(uint16_t offset, uint8_t* dat, uint16_t len)
{
	uint8_t read_len;
	uint8_t ret;
	
	if ((offset+len) > GCORE_NVRAM_FULL_LEN) {
		return false;
	}
	
	while (len) {
		if (len > I2C_BUFFER_LENGTH) {
			read_len = I2C_BUFFER_LENGTH;
		} else {
			read_len = len;
		}
		
		Wire.beginTransmission(GCORE_I2C_ADDR);
		Wire.write((offset>>8) & 0xFF);
		Wire.write(offset & 0xFF);
		len -= read_len;
		while (read_len--) {
			Wire.write(*dat++);
		}
		if (Wire.endTransmission() != 0) {
			return false;
		}
	}
	
	return true;
}


bool gCore::gcore_get_time_secs(uint32_t* s)
{
	return gcore_get_reg32(GCORE_REG_TIME, s);
}


bool gCore::gcore_set_time_secs(uint32_t s)
{
	return gcore_set_reg32(GCORE_REG_TIME, s);
}


bool gCore::gcore_get_alarm_secs(uint32_t* s)
{
	return gcore_get_reg32(GCORE_REG_ALARM, s);
}


bool gCore::gcore_set_alarm_secs(uint32_t s)
{
	return gcore_set_reg32(GCORE_REG_ALARM, s);
}


bool gCore::gcore_get_corr_secs(uint32_t* s)
{
	return gcore_get_reg32(GCORE_REG_CORR, s);
}


bool gCore::gcore_set_corr_secs(uint32_t s)
{
	return gcore_set_reg32(GCORE_REG_CORR, s);
}


void gCore::power_set_brightness(int percent)
{
	uint8_t pwm_val;
	
	if (percent < 0) percent = 0;
	if (percent > 100) percent = 100;
	
	pwm_val = 255 * percent / 100;
	(void) gcore_set_reg8(GCORE_REG_BL, pwm_val);
}


void gCore::power_set_button_short_press_msec(int msec)
{
	uint8_t btn_10msec_intervals;
	
	if (msec < 20) msec = 20;       // Min debounce time
	if (msec > 2550) msec = 2550;   // Max mSec
	
	btn_10msec_intervals = msec / 10;
	(void) gcore_set_reg8(GCORE_REG_PWR_TM, btn_10msec_intervals);
}


bool gCore::power_button_pressed()
{
	uint8_t reg_val;
	
	if (gcore_get_reg8(GCORE_REG_STATUS, &reg_val)) {
		return ((reg_val & GCORE_ST_PB_PRESS_MASK) == GCORE_ST_PB_PRESS_MASK);
	} else {
		return false;
	}
}


bool gCore::power_get_sdcard_present()
{
	uint8_t reg_val;
	
	if (gcore_get_reg8(GCORE_REG_GPIO, &reg_val)) {
		return ((reg_val & GCORE_GPIO_SD_CARD_MASK) == GCORE_GPIO_SD_CARD_MASK);
	} else {
		return false;
	}
}


void gCore::power_off()
{
	(void) gcore_set_reg8(GCORE_REG_SHDOWN, GCORE_SHUTDOWN_TRIG);
	//while (1) {};
}


time_t gCore::rtc_get_time_secs()
{
	uint32_t t = 0;
    
    (void) gcore_get_time_secs(&t);
    
    return((time_t) t);
}


bool gCore::rtc_set_time_secs(time_t t)
{
	return (gcore_set_time_secs((uint32_t) t));
}


void gCore::rtc_read_time(tmElements_t* tm)
{
	uint32_t t = 0;
	
	(void) gcore_get_time_secs(&t);
	
	rtc_breakTime(t, tm);
}


bool gCore::rtc_write_time(const tmElements_t tm)
{
	return (gcore_set_time_secs((uint32_t) rtc_makeTime(tm)) == true);
}


time_t gCore::rtc_get_alarm_secs()
{
	uint32_t t = 0;
    
    (void) gcore_get_alarm_secs(&t);
    
    return((time_t) t);
}


bool gCore::rtc_set_alarm_secs(time_t t)
{
	return (gcore_set_alarm_secs((uint32_t) t));
}


void gCore::rtc_read_alarm(tmElements_t* tm)
{
	uint32_t t = 0;
	
	(void) gcore_get_alarm_secs(&t);
	
	rtc_breakTime(t, tm);
}


bool gCore::rtc_write_alarm(const tmElements_t tm)
{
	return (gcore_set_alarm_secs((uint32_t) rtc_makeTime(tm)) == true);
}


bool gCore::rtc_enable_alarm(bool en)
{
	return (gcore_set_wakeup_bit(GCORE_WK_ALARM_MASK, en) == true);
}



bool gCore::rtc_get_alarm_enable(bool* en)
{
	uint8_t t8;
	
	if (gcore_get_reg8(GCORE_REG_WK_CTRL, &t8)) {
		*en = ((t8 & GCORE_WK_ALARM_MASK) == GCORE_WK_ALARM_MASK);
		return true;
	} else {
		return false;
	}
}


void gCore::rtc_breakTime(time_t timeInput, tmElements_t* tm)
{
	uint8_t year;
	uint8_t month, monthLength;
	uint32_t time;
	unsigned long days;

	time = (uint32_t)timeInput;
	tm->Second = time % 60;
	time /= 60; // now it is minutes
	tm->Minute = time % 60;
	time /= 60; // now it is hours
	tm->Hour = time % 24;
	time /= 24; // now it is days
	tm->Wday = ((time + 4) % 7) + 1;  // Sunday is day 1 
  
	year = 0;  
	days = 0;
	while((unsigned)(days += (LEAP_YEAR(year) ? 366 : 365)) <= time) {
		year++;
	}
	tm->Year = year; // year is offset from 1970 
  
	days -= LEAP_YEAR(year) ? 366 : 365;
	time  -= days; // now it is days in this year, starting at 0
  
	days=0;
	month=0;
	monthLength=0;
	for (month=0; month<12; month++) {
		if (month==1) { // february
			if (LEAP_YEAR(year)) {
				monthLength=29;
			} else {
				monthLength=28;
			}
		} else {
			monthLength = monthDays[month];
		}
    
		if (time >= monthLength) {
			time -= monthLength;
		} else {
			break;
		}
	}
	tm->Month = month + 1;  // jan is month 1  
	tm->Day = time + 1;     // day of month
}


time_t gCore::rtc_makeTime(const tmElements_t tm)
{
	int i;
	uint32_t seconds;

	// seconds from 1970 till 1 jan 00:00:00 of the given year
	seconds= tm.Year*(SECS_PER_DAY * 365);
	for (i = 0; i < tm.Year; i++) {
		if (LEAP_YEAR(i)) {
			seconds +=  SECS_PER_DAY;   // add extra days for leap years
		}
	}
  
	// add days for this year, months start from 1
	for (i = 1; i < tm.Month; i++) {
		if ( (i == 2) && LEAP_YEAR(tm.Year)) { 
			seconds += SECS_PER_DAY * 29;
		} else {
			seconds += SECS_PER_DAY * monthDays[i-1];  //monthDay array starts from 0
		}
	}
	seconds+= (tm.Day-1) * SECS_PER_DAY;
	seconds+= tm.Hour * SECS_PER_HOUR;
	seconds+= tm.Minute * SECS_PER_MIN;
	seconds+= tm.Second;
	return (time_t)seconds;
}
