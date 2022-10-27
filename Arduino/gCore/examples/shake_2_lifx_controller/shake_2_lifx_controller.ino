/*
 * A mash-up between the "shake_2_wake" and "lifx_controller" sketches.  Controls LIFX
 * LED light bulbs with a GUI interface and uses an accelerometer to wake up the device
 * when it is moved.
 * 
 * LIFX LED Control
 * ----------------
 * Connects to a local network via Wifi and discovers all active LIFX light bulbs using
 * their UDP-based discovery protocol.  Then allows control of each light with a control
 * panel based on the light type.  Color Capable lights get a 2-dimensional hue/saturation
 * picker, a brightness picker and a white kelvin picker.  Adjustable white lights get a
 * brightness picker and a white kelvin picker while non-adjustable white lights get an on/off
 * switch.  Graphics drawn using the LVGL library.
 * 
 * Motion controlled wake-up
 * -------------------------
 * The interrupt output of an accelerometer is used to pull the power button (SW)
 * input low to switch gCore on when it detects motion.  Its power is supplied by
 * gCore's always-on 3.3V output (VI) and when gCore is powered off it is configured
 * into a low power mode with motion detection enabled and configured to drive the
 * interrupt low.  When gCore is powered on, the accelerometer is reconfigured into
 * normal operating mode and the interrupt disabled (so it won't interfere with SW).
 *
 * After the code runs once then powering off gCore using the power button will configure
 * the accelerometer to wake it back up when it is shaken.
 *
 * This code turns the LCD backlight down immediately upon detection of the power button
 * press and then waits a few seconds before reconfiguring the accelerometer and powering off
 * to allow the user to set the device down.
 * 
 * Other functionality
 * -------------------
 * Powers down automatically after two minutes of inactivity (when running on battery) or if
 * the power button is pressed for more than 200 mSec.
 * 
 * Turns the LCD backlight down immediately upon detection of the power button
 * press and then waits a few seconds before reconfiguring the accelerometer and powering off
 * to allow the user to set the device down.
 * 
 * Software Configuration
 * -----------------------  
 * You must configure "ssid" and "password" in USER CONFIGURATION below with the credentials
 * of your network.
 * 
 * Hardware Configuration
 * ----------------------
 * This demo uses the Sparkfun ADXL362 accelerometer (https://www.sparkfun.com/products/11446).
 * 
 * The ADXL362 accelerometer communicates using a SPI interface.  It is connected to gCore
 * using the following following connections:
 *
 * ADXL362    gCore
 * -----------------------------------------------------------------------------------
 *    GND        GND
 *    3.3V       VI (always-on 3.3V output)
 *    CS         GPIO 32 (CSN)
 *    SCLK       GPIO 25 (SCLK)
 *    MOSI       GPIO 26 (MOSI)
 *    MISO       GPIO 34 (MISO)
 *    INT1       SW (switch input) through a 1 k-ohm resistor
 *
 * Since, by default, the ESP32 Arduino package uses the ESP32 VSPI peripheral for the default
 * Arduino SPI interface.  However gCore uses that for the dedicated LCD interface so we must
 * use the ESP32 HSPI peripheral.  This demo shows how to configure and use HSPI on any pins.
 *
 * Sketch Requirements
 * -------------------
 * Requires the following libraries
 *   1. gCore
 *   2. LVGL Arduino (see note below for configuration)
 *   3. TFT_eSPI ported to gCore
 *   4. Lifx and LifxProducts 
 * 
 * This code configures the display to operate in Portrait mode.  The lv_conf.h file in the LVGL library
 * directory must be configured to match
 *   #define LV_HOR_RES_MAX          (320)
 *   #define LV_VER_RES_MAX          (480)
 *
 */
#include "gCore.h"
#include <lvgl.h>
#include "Lifx.h"
#include "LifxProducts.h"
#include <TFT_eSPI.h>
#include <Wire.h>

// ================================================
// USER CONFIGURATION
// ================================================
// Set your Wifi credentials here
const char* ssid = "YOUR_SSID_HERE";
const char* password = "YOUR_PASSWORD_HERE";



// ================================================
// PROGRAM CONSTANTS AND VARIABLES
// ================================================

// Auto-shutdown timeout (mSec)
#define APP_INACTIVITY_TIMEOUT 120000

// Motion Detection Force (16-bit unsigned mG)
#define MOTION_FORCE     400

// Motion Detection Duration (8-bit unsigned units of 10 mSec)
#define MOTION_TIME      20


// ------------------------------------------------
// GUI Display states
#define GUI_WIFI_SETUP  0
#define GUI_NO_WIFI     1
#define GUI_DEV_SCAN    2
#define GUI_NO_DEV      3
#define GUI_CTRL_DEV    4

// Application state
#define APP_WIFI_SETUP  0
#define APP_DEV_SETUP   1
#define APP_DEV_CTRL    2
#define APP_ERR_WAIT    3


// ------------------------------------------------
// IO Pins
const int pin_csn = 32;
const int pin_sclk = 25;
const int pin_mosi = 26;
const int pin_miso = 34;


// ------------------------------------------------
// Objects
gCore gc;
Lifx lifx;
SPIClass* spiA = NULL;
TFT_eSPI tft = TFT_eSPI();

// SPI clock rate
const int spi_hz = 4000000; // 4 MHz


// ------------------------------------------------
// Global Variables

// LVGL
lv_disp_buf_t disp_buf;
lv_color_t buf[LV_HOR_RES_MAX * 10];

const int screenWidth = 320;
const int screenHeight = 480;

// Application
int app_state;
bool lifx_discovery_in_process = false;
unsigned long lvgl_prev_msec;
unsigned long app_inactivity_prev_msec;



// ================================================
// Application subroutines
// ================================================
void app_set_state(int st)
{
  switch (st) {
    case APP_WIFI_SETUP:
      gui_display_mode(GUI_WIFI_SETUP);

      WiFi.disconnect(true);
      WiFi.mode(WIFI_STA);
      Serial.print("Connecting to WiFi...");
      WiFi.begin(ssid, password);
      break;

    case APP_DEV_SETUP:
      gui_display_mode(GUI_DEV_SCAN);

      WiFi.setAutoReconnect(true);
      Serial.print("IP address: ");
      Serial.println(WiFi.localIP());

      // Initialize the LIFX library once networking is up
      lifx.begin();
      break;

    case APP_DEV_CTRL:
      gui_display_mode(GUI_CTRL_DEV);
      break;

    case APP_ERR_WAIT:
      if (app_state == APP_WIFI_SETUP) {
        gui_display_mode(GUI_NO_WIFI);
      } else if (app_state == APP_DEV_SETUP) {
        gui_display_mode(GUI_NO_DEV);
      }
      break;
  }

  app_state = st;
}


void lifx_discovery_complete(Lifx& l)
{
  Serial.println("Discovery Complete");
  l.PrintDevices();
  lifx_discovery_in_process = false;
}


void note_activity()
{
  app_inactivity_prev_msec = millis();
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
  Serial.println("shake_2_lifx_controller");

  // Setup the ESP32 HSPI peripheral for SPI communications with the accelerometer
  spiA = new SPIClass(HSPI);
  spiA->begin(pin_sclk, pin_miso, pin_mosi, pin_csn); //SCK,MISO,MOSI,SS
  pinMode(pin_csn, OUTPUT);
  digitalWrite(pin_csn, HIGH);

  // Setup accelerometer
  accel_init();
  
  // Setup gCore
  gc.begin();
  gc.power_set_brightness(75);
  gc.power_set_button_short_press_msec(200);

  // Setup LVGL
  lvgl_setup();
  gui_init();

  // Start the app
  app_set_state(APP_WIFI_SETUP);
  lvgl_prev_msec = millis();
  note_activity();
}


void loop()
{
  uint8_t reg;
  
  // Execute loop with minimal delays (routines shouldn't block), evaluating each subsystem as necessary

  // Let the LIFX library process incoming UDP packets
  if (app_state != APP_WIFI_SETUP) {
    lifx.loop();
  }
  
  // Periodically give time to LVGL to update
  if (task_timeout(&lvgl_prev_msec, 20)) {
    lv_task_handler();
  }

  // Evaluate the top-level app state
  switch (app_state) {
    case APP_WIFI_SETUP:
      if (WiFi.status() == WL_CONNECTED) {
        app_set_state(APP_DEV_SETUP);

        // Start device discovery
        Serial.println("Start discovery...");
        lifx.DiscoveryCompleteCallback(lifx_discovery_complete);
        lifx.StartDiscovery();
        lifx_discovery_in_process = true;
      } else {
        delay(10);
      }
      break;
      
    case APP_DEV_SETUP:
      if (!lifx_discovery_in_process) {
        if (lifx.DeviceCount() > 0) {
          app_set_state(APP_DEV_CTRL);
        } else {
          app_set_state(APP_ERR_WAIT);
        }
      }
      break;
      
    case APP_DEV_CTRL:
      break;

    case APP_ERR_WAIT:
      break;
  }

  // Finally look for a manual power-off or an inactivity shutdown (when running on battery)
  //   This blocks when the shutdown sequence starts
  (void) gc.gcore_get_reg8(GCORE_REG_GPIO, &reg);
  if (gc.power_button_pressed() ||
      (task_timeout(&app_inactivity_prev_msec, APP_INACTIVITY_TIMEOUT) && ((reg & GCORE_GPIO_CHG_MASK) == GCORE_CHG_IDLE))) {

    // Turn the backlight down to indicate to the user we've a detected power off condition
    gc.power_set_brightness(5);

    // Then wait a few seconds before reconfiguring the accelerometer and switching off to let them set
    // the device down so it doesn't immediately wake back up
    delay(3000);

    // Finally configure the accelerometer for wakeup and then power off
    Serial.println("Power down...");
    delay(10);
    accel_setup_sleep();
    gc.power_off();
    while (1) {};
  }
}
