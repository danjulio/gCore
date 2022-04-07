/*
 * Touch demo using the dual-touch capabilities of the FT6236 touchscreen controller.
 * 
 * Displays 0, 1, or 2 circles on the screen centered on the touch point.
 * 
 * Note: When two touches are vertically aligned the FT6236 seems to malfunction and
 * return one touch centered somewhere between them.  Also seen by others:
 * 
 * https://www.reddit.com/r/esp32/comments/ngqd47/ft6236_touch_controller_multitouch_test_two/
 * 
 * Requires the following libraries
 *   1. Adafruit_GFX
 *   2. gCore
 *   3. gCore_ILI9488
 * 
 */
#include <Adafruit_GFX.h>
#include <gCore.h>
#include <gCore_ILI9488.h>
#include <Wire.h>


// ==========================
// Constants
//

// Uncomment for additional debug information
//#define TS_DEBUG

// Circle radius
#define RADIUS 35

// FT6236 touchscreen controller
#define FT62XX_ADDR           0x38
#define FT62XX_G_FT5201ID     0xA8
#define FT62XX_REG_NUMTOUCHES 0x02

#define FT62XX_REG_MODE 0x00
#define FT62XX_REG_GEST_ID 0x01
#define FT62XX_REG_CALIBRATE 0x02
#define FT62XX_REG_P1_XH 0x03
#define FT62XX_REG_P2_XH 0x09
#define FT62XX_REG_THRESHHOLD 0x80
#define FT62XX_REG_POINTRATE 0x88
#define FT62XX_REG_FIRMVERS 0xA6
#define FT62XX_REG_CHIPID 0xA3
#define FT62XX_REG_VENDID 0xA8

#define FT62XX_VENDID 0x11
#define FT6206_CHIPID 0x06
#define FT6236_CHIPID 0x36
#define FT6236U_CHIPID 0x64 

#define FT62XX_DEFAULT_THRESHOLD 80

// Calibration constants
#define TFT_WIDTH      480
#define TFT_HEIGHT     320
#define TS_X_MIN       0
#define TS_Y_MIN       0
#define TS_X_MAX       480
#define TS_Y_MAX       320
#define TS_XY_SWAP     true
#define TS_X_INV       true
#define TS_Y_INV       false



// ==========================
// Objects
//
gCore gc;
gCore_ILI9488 tft;



// ==========================
// Variables
//

typedef struct {
  int16_t x;
  int16_t y;
  bool valid;
} circ_T;

circ_T points[FT62XX_REG_NUMTOUCHES];



// ==========================
// Arduino entry points
//

void setup() {
  Serial.begin(115200);

  // Setup gCore
  gc.begin();
  gc.power_set_brightness(75);
  gc.power_set_button_short_press_msec(100);

  // Setup the display driver (landscape mode)
  tft.begin();
  tft.setRotation(3);
  tft.fillScreen(ILI9488_BLACK);

  // Setup the touchscreen controller
  ts_begin();

  // No points currently detected
  for (int i = 0; i<FT62XX_REG_NUMTOUCHES; i++) {
    points[i].valid = false;
  }
}


void loop() {
  int i;
  int n;

  // Erase any previous points each time through the loop
  for (i=0; i<FT62XX_REG_NUMTOUCHES; i++) {
    if (points[i].valid) {
      points[i].valid = false;
      draw_circle(i, ILI9488_BLACK);
    }
  }
    
  // Look for touch
  n = ts_get_num_points();
  if (n > 0) {
    for (i=0; i<n; i++) {
      ts_get_point(i, &(points[i].x), &(points[i].y));
      points[i].valid = true;
      draw_circle(i, ILI9488_RED);
    }
  }

  delay(50);
}



// ==========================
// Subroutines
//
bool ts_begin()
{
  // Check if we can communicate
  uint8_t id = ftReadRegister8(FT62XX_REG_CHIPID);
  if ((id != FT6206_CHIPID) && (id != FT6236_CHIPID) && (id != FT6236U_CHIPID)) {
    Serial.printf("Error: Cannot open FT (id = 0x%x)\n", id);
  } else {
    // Set the threshold
    ftWriteRegister8(FT62XX_REG_THRESHHOLD, FT62XX_DEFAULT_THRESHOLD);
  }
}


int ts_get_num_points()
{
  int n;
  n = (int) ftReadRegister8(FT62XX_REG_NUMTOUCHES);

  // Register can return 0xFF initially when there are no touches
  if (n > 2) n = 0;

  return n;
}


// n = 0 for point 1, n = 1 for point 2
void ts_get_point(int n, int16_t* x, int16_t* y)
{
  uint8_t i2cdat[4];
  int pt_offset;

  // Starting register offset for selected point (each point takes 4 consecutive byte registers)
  pt_offset = (n == 0) ? FT62XX_REG_P1_XH : FT62XX_REG_P2_XH;

  // Get the register block
  Wire.beginTransmission(FT62XX_ADDR);
  Wire.write(pt_offset);  
  Wire.endTransmission();

  Wire.requestFrom(FT62XX_ADDR, 4);
  for (uint8_t i=0; i<4; i++) {
    i2cdat[i] = Wire.read();
  }

  // Extract the raw coordinate values
  *x = i2cdat[0] & 0x0F;
  *x <<= 8;
  *x |= i2cdat[1];
  *y = i2cdat[2] & 0x0F;
  *y <<= 8;
  *y |= i2cdat[3];

#ifdef TS_DEBUG
  Serial.printf("raw %d %d", *x, *y);
#endif

  // Adjust to our coordinate system
  ts_adjust_data(x, y);

#ifdef TS_DEBUG
  Serial.printf(" => %d %d\n", *x, *y);
#endif
}


void ts_adjust_data(int16_t* x, int16_t* y)
{
#if TS_XY_SWAP != 0
    int16_t swap_tmp;
    swap_tmp = *x;
    *x = *y;
    *y = swap_tmp;
#endif

    if((*x) > TS_X_MIN)(*x) -= TS_X_MIN;
    else(*x) = 0;

    if((*y) > TS_Y_MIN)(*y) -= TS_Y_MIN;
    else(*y) = 0;

    (*x) = (uint32_t)((uint32_t)(*x) * TFT_WIDTH) /
           (TS_X_MAX - TS_X_MIN);

    (*y) = (uint32_t)((uint32_t)(*y) * TFT_HEIGHT) /
           (TS_Y_MAX - TS_Y_MIN);

#if TS_X_INV != 0
    (*x) =  TFT_WIDTH - (*x);
#endif

#if TS_Y_INV != 0
    (*y) =  TFT_HEIGHT - (*y);
#endif
}


uint8_t ftReadRegister8(uint8_t reg) {
  uint8_t x ;
  
  Wire.beginTransmission(FT62XX_ADDR);
  Wire.write((byte)reg);
  Wire.endTransmission();
  
  Wire.requestFrom((byte)FT62XX_ADDR, (byte)1);
  x = Wire.read();
  
  return x;
}


void ftWriteRegister8(uint8_t reg, uint8_t val) {
  Wire.beginTransmission(FT62XX_ADDR);
  Wire.write((byte)reg);
  Wire.write((byte)val);
  Wire.endTransmission();
}


void draw_circle(int n, uint16_t c)
{
  tft.drawCircle(points[n].x, points[n].y, RADIUS, c);
}
