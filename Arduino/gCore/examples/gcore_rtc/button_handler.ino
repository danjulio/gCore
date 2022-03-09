/*
 * Button handler - read the touchscreen and determine button down and released
 * status (button presses are registered 
 * 
 *   setup_buttons() called during setup
 *   set_button_screen(screen_number) called when switching screens - by screen init
 *   eval_buttons() called in loop before evaulating a screen
 *   button_pressed() called by a screen evaulation
 */


// =====================
// External entry points
// =====================

void setup_buttons()
{
  int i;

  // Create main screen buttons
  for (i=0; i<SCREEN_MAIN_NUM_BTNS; i++) {
    screen_main_buttons[i].initButtonUL(&tft,
      screen_main_btn_setup[i].x1,
      screen_main_btn_setup[i].y1,
      screen_main_btn_setup[i].w,
      screen_main_btn_setup[i].h,
      screen_main_btn_setup[i].outline_color,
      screen_main_btn_setup[i].fill_color,
      screen_main_btn_setup[i].text_color,
      screen_main_btn_setup[i].label,
      screen_main_btn_setup[i].textsize
      );
  }

  // Create main screen buttons
  for (i=0; i<SCREEN_SET_NUM_BTNS; i++) {
    screen_set_buttons[i].initButtonUL(&tft,
      screen_set_btn_setup[i].x1,
      screen_set_btn_setup[i].y1,
      screen_set_btn_setup[i].w,
      screen_set_btn_setup[i].h,
      screen_set_btn_setup[i].outline_color,
      screen_set_btn_setup[i].fill_color,
      screen_set_btn_setup[i].text_color,
      screen_set_btn_setup[i].label,
      screen_set_btn_setup[i].textsize
      );
  }
}


void set_button_screen(int n)
{
  int i;

  // Setup for the selected array
  if (n == 0) {
    cur_buttons = screen_main_buttons;
    cur_num_btns = SCREEN_MAIN_NUM_BTNS;
  } else {
    cur_buttons = screen_set_buttons;
    cur_num_btns = SCREEN_SET_NUM_BTNS;
  }
  cur_btn_index = -1;
  btn_down_index = -1;

  // Draw the buttons
  for (i=0; i<cur_num_btns; i++) {
    cur_buttons[i].drawButton(false);
  }
}


void eval_buttons()
{
  int i;
  int cur_btn_down = -1;
  uint16_t x, y;

  if (screen_touched(&x, &y)) {
    // Determine if a button is being touched
    cur_btn_down = button_touched(x, y);

    if (cur_btn_down == -1) {
      // Touch is between buttons
      if (btn_down_index != -1) {
        // User slid outside of current button - de-highlight it
        cur_buttons[btn_down_index].drawButton(false);
        btn_down_index = -1;
      }
    } else {
      // Touch is in a button
      if (btn_down_index == -1) {
        // New button press detected - highlight it
        cur_buttons[cur_btn_down].drawButton(true);
        btn_down_index = cur_btn_down;
      }
    }
  } else {
    if (btn_down_index != -1) {
      // Release detected - de-highlight button and note button press
      cur_buttons[btn_down_index].drawButton(false);
      cur_btn_index = btn_down_index;
      btn_down_index = -1;
    }
  }

  
}


int button_pressed()
{
  int n;

  if (cur_btn_index >= 0) {
    n = cur_btn_index;
    cur_btn_index = -1;
    return n;
  }
  
  return cur_btn_index;
}


// =====================
// Internal routines
// =====================
bool screen_touched(uint16_t* x, uint16_t* y)
{
  TS_Point p;
  
  if (!ctp.touched()) {
    return false;
  }

  p = ctp.getPoint();
  
  // flip it around to match the screen orientation
  *y = p.x;
  *x = map(p.y, 0, 480, 480, 0);
  
  return true;
}


int button_touched(uint16_t x, uint16_t y)
{
  int i;

  for (i=0; i<cur_num_btns; i++) {
    if (cur_buttons[i].contains((int16_t) x, (int16_t) y)) {
      return i;
    }
  }

  return -1;
}
