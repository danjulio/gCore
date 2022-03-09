/*
 * Real Time CLock with power-on Alarm demo.  Demonstrates gCore's HW RTC
 * and wakeup alarm capability.  Set an alarm and then power-off gCore.  It
 * will automatically power back on when the alarm occurs.
 * 
 * Requires Adafruit_GFX, Adafruit_FT6206, gCore and gCore_ILI9488 libraries.
 * 
 * Uses Adafruit GFX buttons to set time, alarm time and control alarm.  Time
 * is represented by a 32-bit epoch time (seconds from midnight Jan 1, 1970.
 * Two "screens" are supported.  One showing the time and a second used to
 * set either the time or alarm time.
 * 
 * Thanks to "Ceez" for the analog clock rendering code.  I shameless pilfered his
 * Clock2.5.ino program at https://ceezblog.info/2015/05/15/arduino-analog-clock-with-ds1307/
 * 
 */
#include <Adafruit_GFX.h>
#include <Adafruit_FT6206.h>
#include <gCore.h>
#include <gCore_ILI9488.h>


// =====================
// Screen indicies
// =====================
#define SCREEN_MAIN_INDEX   0
#define SCREEN_SET_T_INDEX  1
#define SCREEN_SET_A_INDEX  2


// =====================
// Analog clock face related data structures and defines
// (much of this from Ceez)
// =====================
typedef struct POINT {uint16_t x; uint16_t y;};
typedef struct LINE {POINT a;POINT b;};
typedef struct HAND_POINTS {POINT a;POINT b; POINT e; POINT f;};
typedef struct HAND_SET{LINE Sec; HAND_POINTS Min; HAND_POINTS Hour;};

#define TME tmElements_t

// Clock colors
#define CREF_BACKGROUND ILI9488_BLACK
#define CREF_FACE       ILI9488_ORANGE
#define CREF_SECOND     ILI9488_RED
#define CREF_MINUTE     ILI9488_CYAN
#define CREF_HOUR       ILI9488_CYAN
#define NUMERIC_POINT   ILI9488_WHITE
#define CREF_HELLO      ILI9488_RED
#define CREF_TEXT       ILI9488_DARKCYAN
#define CREF_TIME       ILI9488_MAGENTA
#define CREF_DATE       ILI9488_GREENYELLOW

#define MINUTE_HAND  0
#define HOUR_HAND    1

// Clock position and dimension
#define Xo 240   // center point
#define Yo 160   // center point
#define RADIUS 100  // radius of the clock face

#define S_LEN  85  // second hand
#define S_TAIL  20

#define M_LEN  80  // minute hand
#define M_TAIL  18
#define M_SIDE  6

#define H_LEN  60  // hour hand
#define H_TAIL  15
#define H_SIDE  8

#define TEXT_SIZE 1

// Alarm info position and dimension
#define A_STATUS_X 190
#define A_STATUS_Y 15

#define A_INFO_X   145
#define A_INFO_Y   295

// Minimum time (start of year 2000)
#define MIN_TIME_SECS 946684800



// =====================
// Button data structure and initialization
// =====================
typedef struct {
  int16_t x1;        // Upper left corner of button
  int16_t y1;
  uint16_t w;         // Button dimensions
  uint16_t h;
  uint16_t outline_color;
  uint16_t fill_color;
  uint16_t text_color;
  char* label;
  uint8_t textsize;
} button_def_t;

//
// Button colors
//
#define BTN_OUTLINE_C ILI9488_BLUE
#define BTN_FILL_C    ILI9488_DARKCYAN
#define BTN_TEXT_C    ILI9488_CYAN

//
// Main Screen Buttons
//
#define SCREEN_MAIN_NUM_BTNS      4

// Button indicies
#define SCREEN_MAIN_BTN_ALARM     0
#define SCREEN_MAIN_BTN_OFF       1
#define SCREEN_MAIN_BTN_SET_TIME  2
#define SCREEN_MAIN_BTN_SET_ALARM 3

const button_def_t screen_main_btn_setup[SCREEN_MAIN_NUM_BTNS] = {
  { 10,  10,  90, 40, BTN_OUTLINE_C, BTN_FILL_C, BTN_TEXT_C, "ALARM", 2},
  {380,  10,  90, 40, BTN_OUTLINE_C, BTN_FILL_C, BTN_TEXT_C, "OFF", 2},
  { 10, 270,  90, 40, BTN_OUTLINE_C, BTN_FILL_C, BTN_TEXT_C, "SET T", 2},
  {380, 270,  90, 40, BTN_OUTLINE_C, BTN_FILL_C, BTN_TEXT_C, "SET A", 2}
};

//
// Setting Screen Buttons
//
#define SCREEN_SET_NUM_BTNS      14

// Button indicies
#define SCREEN_SET_BTN_0         0
#define SCREEN_SET_BTN_1         1
#define SCREEN_SET_BTN_2         2
#define SCREEN_SET_BTN_3         3
#define SCREEN_SET_BTN_4         4
#define SCREEN_SET_BTN_5         5
#define SCREEN_SET_BTN_6         6
#define SCREEN_SET_BTN_7         7
#define SCREEN_SET_BTN_8         8
#define SCREEN_SET_BTN_9         9
#define SCREEN_SET_BTN_BCK       10
#define SCREEN_SET_BTN_FOR       11
#define SCREEN_SET_BTN_SAVE      12
#define SCREEN_SET_BTN_CANCEL    13

const button_def_t screen_set_btn_setup[SCREEN_SET_NUM_BTNS] = {
  {325, 250,  50, 50, BTN_OUTLINE_C, BTN_FILL_C, BTN_TEXT_C, "0", 2},
  {270,  85,  50, 50, BTN_OUTLINE_C, BTN_FILL_C, BTN_TEXT_C, "1", 2},
  {325,  85,  50, 50, BTN_OUTLINE_C, BTN_FILL_C, BTN_TEXT_C, "2", 2},
  {380,  85,  50, 50, BTN_OUTLINE_C, BTN_FILL_C, BTN_TEXT_C, "3", 2},
  {270, 140,  50, 50, BTN_OUTLINE_C, BTN_FILL_C, BTN_TEXT_C, "4", 2},
  {325, 140,  50, 50, BTN_OUTLINE_C, BTN_FILL_C, BTN_TEXT_C, "5", 2},
  {380, 140,  50, 50, BTN_OUTLINE_C, BTN_FILL_C, BTN_TEXT_C, "6", 2},
  {270, 195,  50, 50, BTN_OUTLINE_C, BTN_FILL_C, BTN_TEXT_C, "7", 2},
  {325, 195,  50, 50, BTN_OUTLINE_C, BTN_FILL_C, BTN_TEXT_C, "8", 2},
  {380, 195,  50, 50, BTN_OUTLINE_C, BTN_FILL_C, BTN_TEXT_C, "9", 2},
  {270, 250,  50, 50, BTN_OUTLINE_C, BTN_FILL_C, BTN_TEXT_C, "<", 2},
  {380, 250,  50, 50, BTN_OUTLINE_C, BTN_FILL_C, BTN_TEXT_C, ">", 2},
  { 60, 140, 150, 50, BTN_OUTLINE_C, BTN_FILL_C, BTN_TEXT_C, "SAVE", 2},
  { 60, 250, 150, 50, BTN_OUTLINE_C, BTN_FILL_C, BTN_TEXT_C, "CANCEL", 2}
};


// =====================
// Time set indicies
// =====================
#define TIMESET_I_HOUR_H 0
#define TIMESET_I_HOUR_L 1
#define TIMESET_I_MIN_H  2
#define TIMESET_I_MIN_L  3
#define TIMESET_I_SEC_H  4
#define TIMESET_I_SEC_L  5
#define TIMESET_I_MON_H  6
#define TIMESET_I_MON_L  7
#define TIMESET_I_DAY_H  8
#define TIMESET_I_DAY_L  9
#define TIMESET_I_YEAR_H 10
#define TIMESET_I_YEAR_L 11
#define TIMESET_NUM_I    12

// Macro to convert a single-digit numeric value (0-9) to an ASCII digit ('0' - '9')
#define ASC_DIGIT(n)     '0' + n

// Time set string locations
#define SET_VAL_X   210
#define SET_VAL_Y   40
#define SET_UNL_Y   45
#define SET_INFO_X  60
#define SET_INFO_Y  40



// =====================
// Variables
// =====================

// Library objects
Adafruit_FT6206 ctp = Adafruit_FT6206();
gCore_ILI9488 tft = gCore_ILI9488();
gCore gc = gCore();


// Screen
int cur_screen;        // 0: main screen, 1: set screen (time), 2: set screen (alarm)
char text_buf[80];     // Temporary buffer used for formatting text to display (e.g. alarm/date)

// Time
bool alarm_enabled;
uint32_t cur_time;
uint32_t prev_time;    // used to detect second rollover for updates
uint32_t alarm_time;

// Analog Clock variables
HAND_SET o_hands;
HAND_SET n_hands;
TME o_tme;
TME n_tme;

// Time set state
tmElements_t timeset_value;
int timeset_index;
char timeset_string[18];   // "HH:MM:SS MM/DD/YY"
char timemarker_string[18];

// Days per month (not counting leap years) for validation (0-based index)
static const uint8_t days_per_month[]={31,28,31,30,31,30,31,31,30,31,30,31};

// Button management
int cur_btn_index;       // -1 when no button down, 0-N for index into button structure
int btn_down_index;
int cur_num_btns;
Adafruit_GFX_Button screen_main_buttons[SCREEN_MAIN_NUM_BTNS];
Adafruit_GFX_Button screen_set_buttons[SCREEN_SET_NUM_BTNS];
Adafruit_GFX_Button* cur_buttons;



// =====================
// Arduino entry points
// =====================
void setup() {
  // Hardware setup
  Serial.begin(115200);
  gc.begin();
  tft.begin();
  tft.setRotation(3);    // Rotate 270° so display correct with gCore IO at bottom
  if (!ctp.begin(40)) {  // pass in 'sensitivity' coefficient
    Serial.println("Couldn't start FT6206 touchscreen controller");
    delay(100);
    gc.power_off();
  }
  gc.power_set_button_short_press_msec(500);
  gc.power_set_brightness(100);

  // Make sure the gCore RTC has a time and alarm value at least as recent as the beginning
  // of this century (so everything displays nicely)
  (void) gc.gcore_get_time_secs(&cur_time);
  if (cur_time < MIN_TIME_SECS) {
    cur_time = MIN_TIME_SECS;
    (void) gc.gcore_set_time_secs(cur_time);
    Serial.printf("Setting time to %d\n", cur_time);
  }
  (void) gc.gcore_get_alarm_secs(&alarm_time);
  if (alarm_time < MIN_TIME_SECS) {
    alarm_time = MIN_TIME_SECS;
    (void) gc.gcore_set_alarm_secs(alarm_time);
    Serial.printf("Setting alarm to %d\n", alarm_time);
  }

  // Setup button management
  setup_buttons();
  
  // Setup the main screen
  set_screen(SCREEN_MAIN_INDEX);
}


void loop() {
  // Evaluate the current screen every 25 mSec
  eval_buttons();
  if (cur_screen == SCREEN_MAIN_INDEX) {
    screen_main_eval();
  } else {
    screen_set_eval();
  }

  // Check for button press to power off
  if (gc.power_button_pressed()) {
    gc.power_off();
  }

  delay(25);
}
