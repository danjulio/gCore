/*
 * Demo Hex Calculator with NVRAM storage.  A simple programmers calculator with
 * number entry and conversion between decimal and hexadecimal bases with logic
 * functions and memory.  Inspired by the Apple OS X calculator that I often use
 * during design and programming (but with the functionality I want...).
 * 
 * A drop-down menu allows selecting the number of bits to work with (8-64 bits
 * in 8-bit increments).  A switch selects decimal or hexadecimal operation.
 * 
 * Configures gCore for a fast (200 mSec) short press detection for power off.
 * 
 * Uses gCore NVRAM to store calculator state between power cycles.
 * 
 * Requires the following libraries
 *   1. gCore
 *   2. LVGL Arduino (see note below for configuration)
 *   3. TFT_eSPI ported to gCore
 * 
 * This code configures the display to operate in Landscape mode.  The lv_conf.h file
 * in the LVGL library directory must be configured to match and to enable both 16-
 * and 28-point Montserrat fonts.
 *   #define LV_HOR_RES_MAX          (480)
 *   #define LV_VER_RES_MAX          (320)
 *   
 *   #define LV_FONT_MONTSERRAT_16   1
 *   #define LV_FONT_MONTSERRAT_28   1
 *   
 */
#include "gCore.h"
#include <lvgl.h>
#include <TFT_eSPI.h>
#include <Wire.h>



// ================================================
// USER CONFIGURATION
// ================================================
// Number display color
#define READOUT_COLOR LV_COLOR_MAKE(0xF0, 0x80, 0x00)

// Auto-shutdown timeout (mSec)
#define APP_INACTIVITY_TIMEOUT 120000



// ================================================
// PROGRAM CONSTANTS AND VARIABLES
// ================================================
#define APP_GCORE_EVAL_MSEC  100
#define APP_LVGL_EVAL_MSEC   20
#define APP_INACTIVITY_COUNT (APP_INACTIVITY_TIMEOUT/APP_GCORE_EVAL_MSEC)



// ------------------------------------------------
// Objects
gCore gc;
TFT_eSPI tft = TFT_eSPI();


// ------------------------------------------------
// Global Variables

// LVGL
lv_disp_buf_t disp_buf;
lv_color_t buf[LV_HOR_RES_MAX * 10];

const int screenWidth = 480;
const int screenHeight = 320;

// Application evaluation timers
unsigned long gcore_prev_msec;
unsigned long lvgl_prev_msec;
unsigned long app_inactivity_count;



// ================================================
// Application subroutines
// ================================================
void note_activity()
{
  app_inactivity_count = 0;
}


bool task_timeout(unsigned long* prevT, unsigned long timeout)
{
  unsigned long curT = millis();
  unsigned long deltaT;
  
  if (curT > *prevT) {
    deltaT = curT - *prevT;
  } else {
    // Handle wrap
    deltaT = ~(*prevT - curT) + 1;
  }

  if (deltaT >= timeout) {
    *prevT = curT;
    return true;
  } else {
    return false;
  }
}



// ================================================
// Arduino entry-points
// ================================================
void setup()
{
  // Diagnostic output
  Serial.begin(115200);

  // Setup gCore
  gc.begin();
  gc.power_set_brightness(75);
  gc.power_set_button_short_press_msec(200);

  // Setup LVGL
  lvgl_setup();

  // Initialize NVRAM (must be done before calc_init)
  nv_init();

  // Setup the initial calculator values
  calc_init();

  // Draw the display (after calculator setup)
  gui_init();

  // Finally start the app
  gcore_prev_msec = millis();
  lvgl_prev_msec = gcore_prev_msec;
  app_inactivity_count = 0;
}


void loop()
{
  uint8_t reg;
  uint16_t batt_mv;
  
  // Periodically give time to LVGL to update
  if (task_timeout(&lvgl_prev_msec, 20)) {
    lv_task_handler();
  }
  
  // Periodically look for conditions to power-off
  //   1. Manual power off from button press
  //   2. Inactivity shutdown (when running on battery)
  //   3. Low battery detection (we do this manually here instead of letting gCore do it so
  //      we can save state to NVRAM)
  //
  if (task_timeout(&gcore_prev_msec, 100)) {
    (void) gc.gcore_get_reg8(GCORE_REG_GPIO, &reg);
    (void) gc.gcore_get_reg16(GCORE_REG_VB, &batt_mv);
    if (gc.power_button_pressed() ||
        ((++app_inactivity_count >= APP_INACTIVITY_COUNT) && ((reg & GCORE_GPIO_CHG_MASK) == GCORE_CHG_IDLE)) ||
        (batt_mv < 3400)) {
      
      Serial.println("Power down...");
      calc_save_state();
      delay(10);
      gc.power_off();
      while (1) {};
    }
  }
}
