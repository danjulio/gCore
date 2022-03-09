/*
 * Time display screen
 *   - Analog clock showing time with date as text inside
 *   - Alarm time displayed below
 *   - Buttons
 *      - Alarm Enable toggle
 *      - Power off
 *      - Set Time -> moves to Set Screen
 *      - Set Alarm -> moves to Set Screen
 * 
 * Externally accessible entry points
 *   - screen_main_init()
 *   - screen_main_eval()
 */

// =====================
// External entry points
// =====================
void screen_main_init()
{
  uint8_t reg;
  
  // Get the current time and alarm values from gCore's RTC
  (void) gc.gcore_get_time_secs(&cur_time);
  (void) gc.gcore_get_alarm_secs(&alarm_time);

  // Get the current alarm enabled state
  (void) gc.gcore_get_reg8(GCORE_REG_WK_CTRL, &reg);
  alarm_enabled = ((reg & GCORE_WK_ALARM_MASK) == GCORE_WK_ALARM_MASK);

  // Draw the clock
  main_draw_clock();
  
  // Draw the alarm info
  main_draw_alarm_status();
  main_draw_alarm();
}


void screen_main_eval()
{
  int btn;
  uint8_t reg;
  
  // Update time
  (void) gc.gcore_get_time_secs(&cur_time);
  if (cur_time != prev_time) {
    main_update_time();
  }

  // Handle button presses
  btn = button_pressed();
  switch (btn) {
    case SCREEN_MAIN_BTN_ALARM:
      // Toggle alarm enable value
      alarm_enabled = !alarm_enabled;
      main_draw_alarm_status();

      // Update gCore's WAKE_ALARM register to enable/disable wake on alarm
      reg = (alarm_enabled) ? GCORE_WK_ALARM_MASK : 0;
      (void) gc.gcore_set_reg8(GCORE_REG_WK_CTRL, reg);
      break;
     
    case SCREEN_MAIN_BTN_OFF:
      gc.power_off();
      break;
     
    case SCREEN_MAIN_BTN_SET_TIME:
      set_screen(SCREEN_SET_T_INDEX);
      break;
     
    case SCREEN_MAIN_BTN_SET_ALARM:
      set_screen(SCREEN_SET_A_INDEX);
      break;
  }
}



// =====================
// Internal routines
// =====================

void main_draw_clock()
{
  gc.rtc_breakTime((time_t) cur_time, &n_tme);
  calc_Hands(n_hands, n_tme);
  drawFace();
 
  draw_NewHourHand();
  draw_NewMinuteHand();
  draw_NewSecondHand();
  tft.fillCircle(Xo, Yo-1, 3, CREF_FACE);              // draw center dot

  prev_time = cur_time;
  copyTME(o_tme, n_tme);
  copyHandSet(o_hands, n_hands);
}


void main_update_time()
{
  gc.rtc_breakTime((time_t) cur_time, &n_tme);
  calc_Hands(n_hands, n_tme);

  cdraw_SecondHand();

  if (o_tme.Minute != n_tme.Minute) {
    cdraw_HourHand();
    cdraw_MinuteHand();
  } else {
    draw_NewHourHand();
    draw_NewMinuteHand();
  }

  tft.fillCircle(Xo, Yo-1, 3, CREF_FACE);              // draw center dot

  prev_time = cur_time;
  copyTME(o_tme, n_tme);
  copyHandSet(o_hands, n_hands);
}


void main_draw_alarm_status()
{
   tft.setTextColor(CREF_TEXT);
   
  if (alarm_enabled) {
    strcpy(text_buf, "ALARM ON ");
  } else {
    strcpy(text_buf, "ALARM OFF");
  }
  print_string(A_STATUS_X, A_STATUS_Y, text_buf, 2);
}


void main_draw_alarm()
{
   tft.setTextColor(CREF_TEXT);
   
  timedate_to_string(alarm_time, text_buf);
  print_string(A_INFO_X, A_INFO_Y, text_buf, 2);
}


// =====================
// Clock drawing routines based on Ceez's work
// =====================

// Draw Clock Face
void drawFace()
{
  int i = 0, angle = 0;
  float x, y;

  // Draw outer frame
  tft.drawCircle(Xo, Yo, RADIUS + 21, CREF_FACE);
  tft.drawCircle(Xo, Yo, RADIUS + 20, CREF_FACE);

  // Draw inner frame
  tft.drawCircle(Xo, Yo, RADIUS + 12, CREF_FACE);
  tft.drawCircle(Xo, Yo, RADIUS + 11, CREF_FACE);
  tft.drawCircle(Xo, Yo, RADIUS + 10, CREF_FACE);

  //Draw Numeric point

  for (i = 0; i <= 12; i++) {
    x = Xo + RADIUS * cos(angle * M_PI / 180);
    y = Yo + RADIUS * sin(angle * M_PI / 180);
    tft.drawCircle(x, y, 2, NUMERIC_POINT);
    angle += 30;
  }

  for (i = 0; i < 360; i += 6) {
    tft.drawPixel(Xo + RADIUS * cos(i * M_PI / 180), Yo + RADIUS * sin(i * M_PI / 180), NUMERIC_POINT);
  }
}

void calc_SecondHand(TME t, LINE &ps )
{
  float angle; // in radian
  //  int Xa, Ya, Xb, Yb;
  angle =  t.Second * 0.1044 - 1.566;
  ps.a.x = Xo + (S_LEN) * cos(angle);
  ps.a.y = Yo + (S_LEN) * sin(angle);
  angle += 3.142; // +180 degree
  ps.b.x = Xo + (S_TAIL) * cos(angle);
  ps.b.y = Yo + (S_TAIL) * sin(angle);
}

// type = 0 -> Minute
// type = 1 -> Hour
void calc_HourMinHand(uint8_t hour_hand, TME t, HAND_POINTS &ps)
{
  float angle;
  if (hour_hand == HOUR_HAND) // 1 == hour, 0 == minute
    angle = t.Hour * 0.524 + t.Minute * 0.0087 - 1.571; // (theta + h*30 + (m*30/60))* M_PI/180
  else
    angle = t.Minute * 0.1044 - 1.571;  // (theta + n_tme.Minute*6)*M_PI/180

  ps.a.x = Xo + ((hour_hand) ? H_LEN : M_LEN) * cos(angle);
  ps.a.y = Yo + ((hour_hand) ? H_LEN : M_LEN) * sin(angle);
  angle += 3.142; //+180 degree to get the tail
  ps.b.x = Xo + ((hour_hand) ? H_TAIL : M_TAIL) * cos(angle);
  ps.b.y = Yo + ((hour_hand) ? H_TAIL : M_TAIL) * sin(angle);
  angle += 1.571; //+90 degree to get the side point
  ps.e.x = Xo + ((hour_hand) ? H_SIDE : M_SIDE) * cos(angle);
  ps.e.y = Yo + ((hour_hand) ? H_SIDE : M_SIDE) * sin(angle);
  angle += 3.142; //+180 degree to get other side point
  ps.f.x = Xo + ((hour_hand) ? H_SIDE : M_SIDE) * cos(angle);
  ps.f.y = Yo + ((hour_hand) ? H_SIDE : M_SIDE) * sin(angle);
}

void draw_NewHand(HAND_POINTS ps, int color)
{
  tft.drawLine(ps.a.x, ps.a.y, ps.e.x, ps.e.y, color);
  tft.drawLine(ps.a.x, ps.a.y, ps.f.x, ps.f.y, color);
  tft.drawLine(ps.b.x, ps.b.y, ps.e.x, ps.e.y, color);
  tft.drawLine(ps.b.x, ps.b.y, ps.f.x, ps.f.y, color);
}

void draw_NewMinuteHand()
{
  draw_NewHand(n_hands.Min, CREF_MINUTE);
}

void draw_NewHourHand()
{
  draw_NewHand(n_hands.Hour, CREF_HOUR);
}

void draw_NewSecondHand()
{
  tft.drawLine(n_hands.Sec.a.x, n_hands.Sec.a.y, n_hands.Sec.b.x, n_hands.Sec.b.y, CREF_SECOND);
  tft.fillCircle(n_hands.Sec.b.x, n_hands.Sec.b.y, 2, CREF_SECOND);
}

void cdraw_SecondHand()
{

  tft.fillCircle(o_hands.Sec.b.x, o_hands.Sec.b.y, 2, CREF_BACKGROUND); 
  tft.fillCircle(n_hands.Sec.b.x, n_hands.Sec.b.y, 2, CREF_SECOND);
  tft.drawLine(o_hands.Sec.a.x, o_hands.Sec.a.y, o_hands.Sec.b.x, o_hands.Sec.b.y, CREF_BACKGROUND);
  tft.drawLine(n_hands.Sec.a.x, n_hands.Sec.a.y, n_hands.Sec.b.x, n_hands.Sec.b.y, CREF_SECOND);
  tft.fillCircle(n_hands.Sec.b.x, n_hands.Sec.b.y, 2, CREF_SECOND);
}

void cdraw_MinuteHand()
{
  tft.drawLine(o_hands.Min.b.x, o_hands.Min.b.y, o_hands.Min.f.x, o_hands.Min.f.y, CREF_BACKGROUND);
  tft.drawLine(n_hands.Min.b.x, n_hands.Min.b.y, n_hands.Min.f.x, n_hands.Min.f.y, CREF_MINUTE);
  
  tft.drawLine(o_hands.Min.b.x, o_hands.Min.b.y, o_hands.Min.e.x, o_hands.Min.e.y, CREF_BACKGROUND);
  tft.drawLine(n_hands.Min.b.x, n_hands.Min.b.y, n_hands.Min.e.x, n_hands.Min.e.y, CREF_MINUTE);

  
  tft.drawLine(o_hands.Min.a.x, o_hands.Min.a.y, o_hands.Min.e.x, o_hands.Min.e.y, CREF_BACKGROUND);
  tft.drawLine(n_hands.Min.a.x, n_hands.Min.a.y, n_hands.Min.e.x, n_hands.Min.e.y, CREF_MINUTE);
  
  tft.drawLine(o_hands.Min.a.x, o_hands.Min.a.y, o_hands.Min.f.x, o_hands.Min.f.y, CREF_BACKGROUND);
  tft.drawLine(n_hands.Min.a.x, n_hands.Min.a.y, n_hands.Min.f.x, n_hands.Min.f.y, CREF_MINUTE);
}

void cdraw_HourHand()
{
  tft.drawLine(o_hands.Hour.b.x, o_hands.Hour.b.y, o_hands.Hour.f.x, o_hands.Hour.f.y, CREF_BACKGROUND);
  tft.drawLine(n_hands.Hour.b.x, n_hands.Hour.b.y, n_hands.Hour.f.x, n_hands.Hour.f.y, CREF_HOUR);
  
  tft.drawLine(o_hands.Hour.b.x, o_hands.Hour.b.y, o_hands.Hour.e.x, o_hands.Hour.e.y, CREF_BACKGROUND);
  tft.drawLine(n_hands.Hour.b.x, n_hands.Hour.b.y, n_hands.Hour.e.x, n_hands.Hour.e.y, CREF_HOUR);

  
  tft.drawLine(o_hands.Hour.a.x, o_hands.Hour.a.y, o_hands.Hour.e.x, o_hands.Hour.e.y, CREF_BACKGROUND);
  tft.drawLine(n_hands.Hour.a.x, n_hands.Hour.a.y, n_hands.Hour.e.x, n_hands.Hour.e.y, CREF_HOUR);
  
  tft.drawLine(o_hands.Hour.a.x, o_hands.Hour.a.y, o_hands.Hour.f.x, o_hands.Hour.f.y, CREF_BACKGROUND);
  tft.drawLine(n_hands.Hour.a.x, n_hands.Hour.a.y, n_hands.Hour.f.x, n_hands.Hour.f.y, CREF_HOUR);
}

void calc_Hands(HAND_SET &hs, TME t)
{
  calc_SecondHand(t, hs.Sec);
  calc_HourMinHand(MINUTE_HAND, t, hs.Min);
  calc_HourMinHand(HOUR_HAND, t, hs.Hour);
}

void copyPoint(POINT &dest, POINT src)
{
  dest.x = src.x;
  dest.y = src.y;
}

void copyHandSet(HAND_SET &dest,HAND_SET src)
{
  copyPoint(dest.Sec.a, src.Sec.a);
  copyPoint(dest.Sec.b, src.Sec.b);
  copyPoint(dest.Min.a, src.Min.a);
  copyPoint(dest.Min.b, src.Min.b);
  copyPoint(dest.Min.e, src.Min.e);
  copyPoint(dest.Min.f, src.Min.f);
  copyPoint(dest.Hour.a, src.Hour.a);
  copyPoint(dest.Hour.b, src.Hour.b);
  copyPoint(dest.Hour.e, src.Hour.e);
  copyPoint(dest.Hour.f, src.Hour.f);
}

void copyTME(tmElements_t &dest, tmElements_t src)
{
  dest.Second = src.Second;
  dest.Minute = src.Minute;
  dest.Hour = src.Hour;
  dest.Wday = src.Wday;
  dest.Day = src.Day;
  dest.Month = src.Month;
  dest.Year = src.Year;
}
