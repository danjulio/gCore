/*
 * Time or Alarm set screen
 *   - Display a keypad and the current selected value to update with a cursor
 *     under the digit to udpate
 *   - Allow the user to set the time and date digit by digit using the keypad
 *     including forward/back cursor keys
 *   - Allow the user to save the new value or cancel and return to the main screen
 * 
 * Externally accessible entry points
 *   - screen_set_init()
 *   - screen_set_eval()
 */

// =====================
// External entry points
// =====================
void screen_set_init()
{
  // Set our timeset val based on the selected item to update
  if (cur_screen == SCREEN_SET_T_INDEX) {
    gc.rtc_breakTime((time_t) cur_time, &timeset_value);
    strcpy(text_buf, " SET TIME");
  } else {
    gc.rtc_breakTime((time_t) alarm_time, &timeset_value);
    strcpy(text_buf, "SET ALARM");
  }
  tft.setTextColor(CREF_TEXT);
  tft.setTextSize(2);
  tft.setCursor(SET_INFO_X, SET_INFO_Y);
  tft.print(text_buf);

  // Setup at the first position
  timeset_index = TIMESET_I_HOUR_H;

  // Display the initial value
  display_timeset_value();
}


void screen_set_eval()
{
  screen_set_eval_buttons();
}



// =====================
// Internal routines
// =====================
static bool is_valid_digit_position(int i)
{
  //       H  H  :  M  M  :  S  S     M  M  /  D  D  /  Y  Y
  // i     0  1  2  3  4  5  6  7  8  9  10 11 12 13 14 15 16
  return (!((i==2)||(i==5)||(i==8)||(i==11)||(i==14)));
}


/**
 * Update the set time/date label.  The current indexed digit is made to be full
 * white to indicate it is the one being changed.
 */
static void display_timeset_value()
{
  int timeset_string_index = 0;  // Current timeset_string insertion point
  int time_string_index = 0;     // Current position in displayed "HH:MM:SS MM/DD/YY"
  int time_digit_index = 0;      // Current time digit index (0-11) for HHMMSSMMDDYY
  int i;
  int16_t x1, y1;
  uint16_t w, h;

  while (time_string_index <= 16) {    
    // Insert the appropriate time character
    //
    //                          H  H  :  M  M  :  S  S     M  M  /  D  D  /  Y  Y
    // time_string_index        0  1  2  3  4  5  6  7  8  9  10 11 12 13 14 15 16
    // time_digit_index         0  1     2  3     4  5     6  7     8  9     10 11
    //
    switch (time_string_index++) {
      case 0: // Hours tens
        timeset_string[timeset_string_index] = ASC_DIGIT(timeset_value.Hour / 10);
        timemarker_string[timeset_string_index++] = (timeset_index == time_digit_index) ? '_' : ' ';
        time_digit_index++;
        break;
      case 1: // Hours units
        timeset_string[timeset_string_index] = ASC_DIGIT(timeset_value.Hour % 10);
        timemarker_string[timeset_string_index++] = (timeset_index == time_digit_index) ? '_' : ' ';
        time_digit_index++;
        break;
      case 3: // Minutes tens
        timeset_string[timeset_string_index] = ASC_DIGIT(timeset_value.Minute / 10);
        timemarker_string[timeset_string_index++] = (timeset_index == time_digit_index) ? '_' : ' ';
        time_digit_index++;
        break;
      case 4: // Minutes units
        timeset_string[timeset_string_index] = ASC_DIGIT(timeset_value.Minute % 10);
        timemarker_string[timeset_string_index++] = (timeset_index == time_digit_index) ? '_' : ' ';
        time_digit_index++;
        break;
      case 6: // Seconds tens
        timeset_string[timeset_string_index] = ASC_DIGIT(timeset_value.Second / 10);
        timemarker_string[timeset_string_index++] = (timeset_index == time_digit_index) ? '_' : ' ';
        time_digit_index++;
        break;
      case 7: // Seconds units
        timeset_string[timeset_string_index] = ASC_DIGIT(timeset_value.Second % 10);
        timemarker_string[timeset_string_index++] = (timeset_index == time_digit_index) ? '_' : ' ';
        time_digit_index++;
        break;
      case 9: // Month tens
        timeset_string[timeset_string_index] = ASC_DIGIT(timeset_value.Month / 10);
        timemarker_string[timeset_string_index++] = (timeset_index == time_digit_index) ? '_' : ' ';
        time_digit_index++;
        break;
      case 10: // Month units
        timeset_string[timeset_string_index] = ASC_DIGIT(timeset_value.Month % 10);
        timemarker_string[timeset_string_index++] = (timeset_index == time_digit_index) ? '_' : ' ';
        time_digit_index++;
        break;
      case 12: // Day tens
        timeset_string[timeset_string_index] = ASC_DIGIT(timeset_value.Day / 10);
        timemarker_string[timeset_string_index++] = (timeset_index == time_digit_index) ? '_' : ' ';
        time_digit_index++;
        break;
      case 13: // Day units
        timeset_string[timeset_string_index] = ASC_DIGIT(timeset_value.Day % 10);
        timemarker_string[timeset_string_index++] = (timeset_index == time_digit_index) ? '_' : ' ';
        time_digit_index++;
        break;
      case 15: // Year tens - Assume we're post 2000
        timeset_string[timeset_string_index] = ASC_DIGIT(tmYearToY2k(timeset_value.Year) / 10);
        timemarker_string[timeset_string_index++] = (timeset_index == time_digit_index) ? '_' : ' ';
        time_digit_index++;
        break;
      case 16: // Year units
        timeset_string[timeset_string_index] = ASC_DIGIT(tmYearToY2k(timeset_value.Year) % 10);
        timemarker_string[timeset_string_index++] = (timeset_index == time_digit_index) ? '_' : ' ';
        time_digit_index++;
        break;
      
      case 2: // Time section separators
      case 5:
        timeset_string[timeset_string_index] = ':';
        timemarker_string[timeset_string_index++] = ' ';
        break;
        
      case 8: // Time / Date separator
        timeset_string[timeset_string_index] = ' ';
        timemarker_string[timeset_string_index++] = ' ';
        break;
        
      case 11: // Date section separators
      case 14:
        timeset_string[timeset_string_index] = '/';
        timemarker_string[timeset_string_index++] = ' ';
        break;
    }
  }
  
  // Make sure the strings are terminated
  timeset_string[timeset_string_index] = 0;
  timemarker_string[timeset_string_index] = 0;
  
  // Display the strings - underline string first since it is only slighly below the
  // time/date string
  tft.setTextSize(2);
  tft.getTextBounds(timeset_string, SET_VAL_X, SET_VAL_Y, &x1, &y1, &w, &h);
  tft.fillRect(x1, y1, w, h+10, ILI9488_BLACK);
  tft.setTextColor(CREF_TEXT);
  tft.setCursor(SET_VAL_X, SET_UNL_Y);
  tft.print(timemarker_string);
  tft.setCursor(SET_VAL_X, SET_VAL_Y);
  tft.print(timeset_string);
}


/**
 * Apply the button press value n to the timeset_value, making sure that only
 * valid values are allowed for each digit position (for example, you cannot set
 * an hour value > 23).  Return true if the digit position was updated, false if it
 * was not changed.
 */
static bool set_timeset_indexed_value(int n)
{
  bool changed = false;
  uint8_t u8;
  
  switch (timeset_index) {
    case TIMESET_I_HOUR_H:
      if (n < 3) {
        timeset_value.Hour = (n * 10) + (timeset_value.Hour % 10);
        changed = true;
      }
      break;
    case TIMESET_I_HOUR_L:
      if (timeset_value.Hour >= 20) {
        // Only allow 20 - 23
        if (n < 4) {
          timeset_value.Hour = ((timeset_value.Hour / 10) * 10) + n;
          changed = true;
        }
      } else {
        // Allow 00-09 or 10-19
        timeset_value.Hour = ((timeset_value.Hour / 10) * 10) + n;
        changed = true;
      }
      break;
    case TIMESET_I_MIN_H:
      if (n < 6) {
        timeset_value.Minute = (n * 10) + (timeset_value.Minute % 10);
        changed = true;
      }
      break;
    case TIMESET_I_MIN_L:
      timeset_value.Minute = ((timeset_value.Minute / 10) * 10) + n;
      changed = true;
      break;
    case TIMESET_I_SEC_H:
      if (n < 6) {
        timeset_value.Second = (n * 10) + (timeset_value.Second % 10);
        changed = true;
      }
      break;
    case TIMESET_I_SEC_L:
      timeset_value.Second = ((timeset_value.Second / 10) * 10) + n;
      changed = true;
      break;
    case TIMESET_I_MON_H:
      if (n < 2) {
        timeset_value.Month = (n * 10) + (timeset_value.Month % 10);
        if (timeset_value.Month == 0) timeset_value.Month = 1;
        changed = true;
      }
      break;
    case TIMESET_I_MON_L:
      if (timeset_value.Month >= 10) {
        // Only allow 10-12
        if (n < 3) {
          timeset_value.Month = ((timeset_value.Month / 10) * 10) + n;
          changed = true;
        }
      } else {
        // Allow 1-9
        if (n > 0) {
          timeset_value.Month = ((timeset_value.Month / 10) * 10) + n;
          changed = true;
        }
      }
      break;
    case TIMESET_I_DAY_H:
      u8 = days_per_month[timeset_value.Month - 1];
      if (n <= (u8 / 10)) {
        // Only allow valid tens digit for this month (will be 2 or 3)
        timeset_value.Day = (n * 10) + (timeset_value.Day % 10);
        changed = true;
      }
      break;
    case TIMESET_I_DAY_L:
      u8 = days_per_month[timeset_value.Month - 1];
      if ((timeset_value.Day / 10) == (u8 / 10)) {
        if (n <= (u8 % 10)) {
          // Only allow valid units digits when the tens digit is the highest
          timeset_value.Day = ((timeset_value.Day / 10) * 10) + n;
          changed = true;
        }
      } else {
        // Units values of 0-9 are valid when the tens is lower than the highest
        timeset_value.Day = ((timeset_value.Day / 10) * 10) + n;
        changed = true;
      }
      break;
    case TIMESET_I_YEAR_H:
      u8 = tmYearToY2k(timeset_value.Year);
      u8 = (n * 10) + (u8 % 10);
      timeset_value.Year = y2kYearToTm(u8);
      changed = true;
      break;
    case TIMESET_I_YEAR_L:
      u8 = tmYearToY2k(timeset_value.Year);
      u8 = ((u8 / 10) * 10) + n;
      timeset_value.Year = y2kYearToTm(u8);
      changed = true;
      break;
  }

  return changed;
}


void screen_set_eval_buttons()
{
  int n = button_pressed();
  
  if (n == SCREEN_SET_BTN_CANCEL) {
    // Bail back to the main screen
    set_screen(SCREEN_MAIN_INDEX);
  } else if (n == SCREEN_SET_BTN_SAVE) {
    // Set the time before going back to the main screen
    update_new_value();
    set_screen(SCREEN_MAIN_INDEX);
  } else if (n == SCREEN_SET_BTN_BCK) {
    // Decrement to the previous digit
    if (timeset_index > TIMESET_I_HOUR_H) {
      timeset_index--;
    }
    display_timeset_value();
  } else if (n == SCREEN_SET_BTN_FOR) {
    // Increment to the next digit
    if (timeset_index < TIMESET_I_YEAR_L) {
      timeset_index++;
    }
    display_timeset_value();
  } else if (n >= SCREEN_SET_BTN_0) {
    // Update the indexed digit based on the button value
    if (set_timeset_indexed_value(n)) {
      // Increment to next digit if the digit was changed
      if (timeset_index < TIMESET_I_YEAR_L) {
        timeset_index++;
      }
    }
      
    // Update the display
    display_timeset_value();
  }
}


void update_new_value()
{
  if (cur_screen == SCREEN_SET_T_INDEX) {
    cur_time = gc.rtc_makeTime(timeset_value);
    (void) gc.gcore_set_time_secs(cur_time);
  } else {
    alarm_time = gc.rtc_makeTime(timeset_value);
    (void) gc.gcore_set_alarm_secs(alarm_time);
  }
}
