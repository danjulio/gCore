/*
 * Adafruit's CapTouchPaint ported to gCore - demonstrates the following
 *   1. Using the gCore library to conifgure the power button for a
 *      1 second press/release and detecting the press to power-off gCore.
 *   2. Using the gCore_ILI9488 driver with the Adafruit GFX library
 *      to render.
 *   3. Using the Adafruit_FT6206 library to access the capacitive touchscreen.
 *
 * Requires the following libraries and ESP32 support added to Arduino
 *   1. gCore
 *   2. gCore_ILI9488
 *   3. Adafruit_GFX
 *   4. Adafruit_FT6206
 *   
 * To compile, configure the Arduino environment as follows
 *   1. Board: ESP32 Wrover Module
 *   2. Port: Serial port associated with gCore
 *   
 * Press the power button for at least one second and then release to power
 * on or off the board (pressing for more than 5 seconds will hard power off).
 *   
 * The following is the original Adafruit header/license, copied in its
 * entirety.
 * 
 * **************************************************
 * This is our touchscreen painting example for the Adafruit ILI9488
 * captouch shield
 * ----> http://www.adafruit.com/products/1947
 *
 * Check out the links above for our tutorials and wiring diagrams
 *
 * Adafruit invests time and resources providing this open source code,
 * please support Adafruit and open-source hardware by purchasing
 * products from Adafruit!
 *
 * Written by Limor Fried/Ladyada for Adafruit Industries.
 * MIT license, all text above must be included in any redistribution
 ****************************************************
 *
*/
#include <Adafruit_GFX.h>
#include <Adafruit_FT6206.h>
#include <gCore.h>
#include <gCore_ILI9488.h>


// The FT6206 uses hardware I2C (SCL/SDA)
Adafruit_FT6206 ctp = Adafruit_FT6206();

// The Display uses gCore's internal SPI bus (no need to specify pins)
gCore_ILI9488 tft = gCore_ILI9488();

// gCore control
gCore gc = gCore();



// Size of the color selection boxes and the paintbrush size
#define BOXSIZE 40
#define PENRADIUS 3
int oldcolor, currentcolor;



void setup() {
  // Start hardware
  Serial.begin(115200);
  gc.begin();
  tft.begin();
  tft.setRotation(2);    // Rotate 180° so display correct with gCore IO at bottom
  if (!ctp.begin(40)) {  // pass in 'sensitivity' coefficient
    Serial.println("Couldn't start FT6206 touchscreen controller");
    delay(100);
    gc.power_off();
  }

  // Configure gCore's power button for a 100 mSec second press detection
  gc.power_set_button_short_press_msec(100);

  // Configure LCD backlight to 75% full brightness
  gc.power_set_brightness(75);

  // Make the color selection boxes
  tft.fillRect(0, 0, BOXSIZE, BOXSIZE, ILI9488_RED);
  tft.fillRect(BOXSIZE, 0, BOXSIZE, BOXSIZE, ILI9488_YELLOW);
  tft.fillRect(BOXSIZE*2, 0, BOXSIZE, BOXSIZE, ILI9488_GREEN);
  tft.fillRect(BOXSIZE*3, 0, BOXSIZE, BOXSIZE, ILI9488_CYAN);
  tft.fillRect(BOXSIZE*4, 0, BOXSIZE, BOXSIZE, ILI9488_BLUE);
  tft.fillRect(BOXSIZE*5, 0, BOXSIZE, BOXSIZE, ILI9488_MAGENTA);
 
  // select the current color 'red'
  tft.drawRect(0, 0, BOXSIZE, BOXSIZE, ILI9488_WHITE);
  currentcolor = ILI9488_RED;

  Serial.println("CapTouchPaint ready to go!");
}


void loop() {
  // Check for button press to power off
  if (gc.power_button_pressed()) {
    gc.power_off();
  }
  
  // Wait for a touch
  if (! ctp.touched()) {
    return;
  }

  // Retrieve a point  
  TS_Point p = ctp.getPoint();
  
 /*
  // Print out raw data from screen touch controller
  Serial.print("X = "); Serial.print(p.x);
  Serial.print("\tY = "); Serial.print(p.y);
  Serial.print(" -> ");
 */

  // flip it around to match the screen.
  p.x = map(p.x, 0, 320, 320, 0);
  p.y = map(p.y, 0, 480, 480, 0);

  // Print out the remapped (rotated) coordinates
  Serial.print("("); Serial.print(p.x);
  Serial.print(", "); Serial.print(p.y);
  Serial.println(")");
  

  if (p.y < BOXSIZE) {
     oldcolor = currentcolor;

     if (p.x < BOXSIZE) { 
       currentcolor = ILI9488_RED; 
       tft.drawRect(0, 0, BOXSIZE, BOXSIZE, ILI9488_WHITE);
     } else if (p.x < BOXSIZE*2) {
       currentcolor = ILI9488_YELLOW;
       tft.drawRect(BOXSIZE, 0, BOXSIZE, BOXSIZE, ILI9488_WHITE);
     } else if (p.x < BOXSIZE*3) {
       currentcolor = ILI9488_GREEN;
       tft.drawRect(BOXSIZE*2, 0, BOXSIZE, BOXSIZE, ILI9488_WHITE);
     } else if (p.x < BOXSIZE*4) {
       currentcolor = ILI9488_CYAN;
       tft.drawRect(BOXSIZE*3, 0, BOXSIZE, BOXSIZE, ILI9488_WHITE);
     } else if (p.x < BOXSIZE*5) {
       currentcolor = ILI9488_BLUE;
       tft.drawRect(BOXSIZE*4, 0, BOXSIZE, BOXSIZE, ILI9488_WHITE);
     } else if (p.x <= BOXSIZE*6) {
       currentcolor = ILI9488_MAGENTA;
       tft.drawRect(BOXSIZE*5, 0, BOXSIZE, BOXSIZE, ILI9488_WHITE);
     }

     if (oldcolor != currentcolor) {
        if (oldcolor == ILI9488_RED) 
          tft.fillRect(0, 0, BOXSIZE, BOXSIZE, ILI9488_RED);
        if (oldcolor == ILI9488_YELLOW) 
          tft.fillRect(BOXSIZE, 0, BOXSIZE, BOXSIZE, ILI9488_YELLOW);
        if (oldcolor == ILI9488_GREEN) 
          tft.fillRect(BOXSIZE*2, 0, BOXSIZE, BOXSIZE, ILI9488_GREEN);
        if (oldcolor == ILI9488_CYAN) 
          tft.fillRect(BOXSIZE*3, 0, BOXSIZE, BOXSIZE, ILI9488_CYAN);
        if (oldcolor == ILI9488_BLUE) 
          tft.fillRect(BOXSIZE*4, 0, BOXSIZE, BOXSIZE, ILI9488_BLUE);
        if (oldcolor == ILI9488_MAGENTA) 
          tft.fillRect(BOXSIZE*5, 0, BOXSIZE, BOXSIZE, ILI9488_MAGENTA);
     }
  }
  if (((p.y-PENRADIUS) > BOXSIZE) && ((p.y+PENRADIUS) < tft.height())) {
    tft.fillCircle(p.x, p.y, PENRADIUS, currentcolor);
  }
}
