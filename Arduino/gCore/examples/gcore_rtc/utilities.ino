/*
 * Common utiltities used by any screen
 */


void set_screen(int n)
{
  // Clear the screen
  tft.fillScreen(ILI9488_BLACK);

  // Setup the buttons
  set_button_screen(n);

  // Let the screen initialize its graphical elements
  cur_screen = n;
  if (n == SCREEN_MAIN_INDEX) {
    screen_main_init();
  } else {
    screen_set_init();
  }
}


void time_to_string(uint32_t t, char* buf)
{
  tmElements_t te;

  // Break down the time into individual components
  gc.rtc_breakTime((time_t) t, &te);

  // Format it in buf
  sprintf(buf, "%d:%02d:%02d", te.Hour, te.Minute, te.Second);
}


void date_to_string(uint32_t t, char* buf)
{
  tmElements_t te;

  // Break down the time into individual components
  gc.rtc_breakTime((time_t) t, &te);
  
  // Always assume we are running post 2000
  te.Year = tmYearToY2k(te.Year);
  
  // Build up the string - US Format
  sprintf(buf, "%d/%d/%02d", te.Month, te.Day, te.Year);
}


void timedate_to_string(uint32_t t, char* buf)
{
  tmElements_t te;

  // Break down the time into individual components
  gc.rtc_breakTime((time_t) t, &te);
  
  // Always assume we are running post 2000
  te.Year = tmYearToY2k(te.Year);
  
  // Build up the string - US Format
  sprintf(buf, "%d:%02d:%02d %d/%d/%02d", te.Hour, te.Minute, te.Second, te.Month, te.Day, te.Year);
}


void print_string(uint16_t x, uint16_t y, const char* msg, uint8_t textsize)
{
  int16_t x1, y1;
  uint16_t w, h;

  // Get a bounding box for the text
  tft.setTextSize(textsize);
  tft.getTextBounds(msg, x, y, &x1, &y1, &w, &h);
  
  // Blank the bounding box
  tft.fillRect(x1, y1, w, h, ILI9488_BLACK);

  // Draw the text
  tft.setCursor(x, y);
  tft.print(msg);
}
