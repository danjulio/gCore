/*
   gCore wake-up demo using button input and external hardware.  Also shows how to configure
   HSPI for use in Arduino.

   The interrupt output of an accelerometer is used to pull the power button (SW)
   input low to switch gCore on when it detects motion.  Its power is supplied by
   gCore's always-on 3.3V output (VI) and when gCore is powered off it is configured
   into a low power mode with motion detection enabled and configured to drive the
   interrupt low.  When gCore is powered on, the accelerometer is reconfigured into
   normal operating mode, the interrupt disabled (so it won't interfere with SW), and
   data from the accelerometer displayed in real time as a set of vectors.

   After the code runs once then powering off gCore using the power button will configure
   the accelerometer to wake it back up when it is shaken.  The idea is that you could
   make some gadget that would turn on when you pick it up (and then use gCore's ability
   for soft power off for it to turn off after some period of inactivity).  Cool huh?

   This code turns the LCD backlight down immediately upon detection of the power button
   press and then waits a few seconds before reconfiguring the accelerometer and powering off
   to allow the user to set the device down.

   This demo uses the Sparkfun ADXL362 accelerometer (https://www.sparkfun.com/products/11446)
   because that's what I had laying around.  It's a bit expensive and other parts can probably
   be made to work as well.

   The ADXL362 accelerometer communicates using a SPI interface.  It is connected to gCore
   using the following following connections:

   H3LIS331DL    gCore
   -----------------------------------------------------------------------------------
      GND        GND
      3.3V       VI (always-on 3.3V output)
      CS         GPIO 32 (CSN)
      SCLK       GPIO 25 (SCLK)
      MOSI       GPIO 26 (MOSI)
      MISO       GPIO 34 (MISO)
      INT1       SW (switch input) through a 1 k-ohm resistor (see note below)

   Since, by default, the ESP32 Arduino package uses the ESP32 VSPI peripheral for the default
   Arduino SPI interface.  However gCore uses that for the dedicated LCD interface so we must
   use the ESP32 HSPI peripheral.  This demo shows how to configure and use HSPI on any pins.

   Requires the following libraries
     1. Adafruit_GFX
     2. gCore
     3. gCore_ILI9488

*/
#include <Adafruit_GFX.h>
#include <gCore.h>
#include <gCore_ILI9488.h>
#include <SPI.h>



// ==========================
// Constants
//

// Motion Detection Force (16-bit unsigned mG)
#define MOTION_FORCE     400

// Motion Detection Duration (8-bit unsigned units of 10 mSec)
#define MOTION_TIME      20

// Auto-shutdown timeout (mSec)
#define SHUTDOWN_TIMEOUT 300000

// Pins
const int pin_csn = 32;
const int pin_sclk = 25;
const int pin_mosi = 26;
const int pin_miso = 34;

// SPI clock rate
const int spi_hz = 4000000; // 4 MHz

// Accelerometer registers addresses
#define ACCEL_DEVID_AD_REG       0x00
#define ACCEL_DEVID_MST_REG      0x01
#define ACCEL_PARTID_REG         0x02
#define ACCEL_REVID_REG          0x03
#define ACCEL_STATUS_REG         0x0B
#define ACCEL_XDATA_L_REG        0x0E
#define ACCEL_YDATA_L_REG        0x10
#define ACCEL_ZDATA_L_REG        0x12
#define ACCEL_TEMP_L_REG         0x14
#define ACCEL_SOFT_RESET_REG     0x1F
#define ACCEL_THRESH_ACT_L       0x20
#define ACCEL_THRESH_ACT_H       0x21
#define ACCEL_TIME_ACT           0x22
#define ACCEL_THRESH_INACT_L     0x23
#define ACCEL_THRESH_INACT_H     0x24
#define ACCEL_TIME_INACT_L       0x25
#define ACCEL_TIME_INACT_H       0x26
#define ACCEL_ACT_INACT_CTL      0x27
#define ACCEL_INTMAP1            0x2A
#define ACCEL_INTMAP2            0x2B
#define ACCEL_FILTER_CTL_REG     0x2C
#define ACCEL_POWER_CTL_REG      0x2D
#define ACCEL_SELF_TEST_REG      0x2E

// Accelerometer write command
#define ACCEL_WRITE              0x0A

// Accelerometer read command
#define ACCEL_READ               0x0B


// Display Y-axis offsets
const char axis_names[3] = {'X', 'Y', 'Z'};
const int axis_offsets[3] = {80, 160, 240};
const uint16_t axis_colors[3] = {ILI9488_RED, ILI9488_GREEN, ILI9488_BLUE};

// Scale factor for converting accel data to graph data
#define GR_SCALE (2048 / 80)



// ==========================
// Objects
//
gCore gc;
gCore_ILI9488 tft;
SPIClass* spiA = NULL;



// ==========================
// Variables
//

// Auto-shutdown
unsigned long shutdown_prev_msec;

// Graph
int16_t gr_x = 0;



// ==========================
// Arduino entry points
//

void setup() {
  uint8_t reg;

  Serial.begin(115200);
  Serial.println("shake_2_wake");

  // Setup the ESP32 HSPI peripheral for SPI communications with the accelerometer
  spiA = new SPIClass(HSPI);
  spiA->begin(pin_sclk, pin_miso, pin_mosi, pin_csn); //SCK,MISO,MOSI,SS
  pinMode(pin_csn, OUTPUT);
  digitalWrite(pin_csn, HIGH);

  // Read and verify the accelerometer
  reg_read(ACCEL_DEVID_AD_REG, 1, &reg);
  if (reg == 0xAD) {
    reg_read(ACCEL_DEVID_MST_REG, 1, &reg);
    if (reg == 0x1D) {
      Serial.println("Found accelerometer");
    } else {
      Serial.printf("Unexpected DEVICE MST value = 0x%x\n", reg);
    }
  } else {
    Serial.printf("Unexpected DEVICE ID value = 0x%x\n", reg);
  }

  // Setup accelerometer for normal operation
  accel_setup_wake();

  // Setup gCore
  gc.begin();
  gc.power_set_brightness(75);
  gc.power_set_button_short_press_msec(100);
  
  // Setup the display driver (landscape mode)
  tft.begin();
  tft.setRotation(3);
  tft.fillScreen(ILI9488_BLACK);
  tft.setTextColor(ILI9488_LIGHTGREY);

  // Setup for operation
  shutdown_prev_msec = millis();
}


void loop() {
  uint8_t reg;
  int16_t a_val[3];

  // Plot current accelerometer values
  reg_read(ACCEL_STATUS_REG, 1, &reg);
  if (reg & 0x01) {
    // New data available
    accel_get_xyz(&a_val[0], &a_val[1], &a_val[2]);
    graph_vals(a_val);
  }

  // Finally look for a manual power-off or an auto-shutdown (when running on battery)
  (void) gc.gcore_get_reg8(GCORE_REG_GPIO, &reg);
  if (gc.power_button_pressed() ||
      (task_timeout(&shutdown_prev_msec, SHUTDOWN_TIMEOUT) && ((reg & GCORE_GPIO_CHG_MASK) == GCORE_CHG_IDLE))) {

    // Turn the backlight down to indicate to the user we've a detected power off condition
    gc.power_set_brightness(5);

    // Then wait a few seconds before reconfiguring the accelerometer and switching off to let them set
    // the device down so it doesn't immediately wake back up
    delay(3000);
    Serial.println("Power down...");
    delay(10);
    accel_setup_sleep();
    gc.power_off();
    while (1) {};
  }

  delay(10);
}



// ==========================
// Subroutines
//

// Setup the accelerometer to detect motion and assert the interrupt low
void accel_setup_sleep()
{
  // Reset
  reg_write(ACCEL_SOFT_RESET_REG, 0x52);
  delay(1);

  // Filter Control: Default - 2G, halved bandwidth, 100 Hz sample rate
  reg_write(ACCEL_FILTER_CTL_REG, 0x13);

  // Activity Threshold - mG
  reg_write16(ACCEL_THRESH_ACT_L, MOTION_FORCE);

  // Activity Duration Time Detect - units of sample rate
  reg_write(ACCEL_TIME_ACT, MOTION_TIME);

  // Interrupt 1 Register - Map ACT to interrupt pin - Output active low to drive SW low
  reg_write(ACCEL_INTMAP1, 0x90);

  // Power Control: Internal Clock, Normal Noise Operation, Enable Measurement mode
  reg_write(ACCEL_POWER_CTL_REG, 0x02);

  // Delay for the 4/ODR Measurement Mode Instruction to Valid data timeout in the spec table 1
  delay(50);

  // Activity/Inactivity Control Register - Set Default Mode, referenced mode for
  //  activity, enable activity functionality.  This sets the reference.
  reg_write(ACCEL_ACT_INACT_CTL, 0x03);
}


// Setup the accelerometer for normal operation
void accel_setup_wake()
{
  // Reset
  reg_write(ACCEL_SOFT_RESET_REG, 0x52);
  delay(1);

  // Filter Control: Default - 2G, halved bandwidth, 100 Hz sample rate
  reg_write(ACCEL_FILTER_CTL_REG, 0x13);

  // Activity/Inactivity Control Register - Normal Mode
  reg_write(ACCEL_ACT_INACT_CTL, 0x00);

  // Interrupt 1 Register - Disable interrupts
  reg_write(ACCEL_INTMAP1, 0x00);

  // Power Control: Internal Clock, Normal Noise Operation, Enable Measurement mode
  reg_write(ACCEL_POWER_CTL_REG, 0x02);
}


void accel_get_xyz(int16_t* x, int16_t* y, int16_t* z)
{
  uint8_t buf[6];

  reg_read(ACCEL_XDATA_L_REG, 6, buf);

  *x = buf[0] | ((int16_t) buf[1] << 8);
  *y = buf[2] | ((int16_t) buf[3] << 8);
  *z = buf[4] | ((int16_t) buf[5] << 8);
}


void reg_read(uint8_t reg, uint8_t len, uint8_t* buf)
{
  digitalWrite(pin_csn, LOW);
  spiA->beginTransaction(SPISettings(spi_hz, MSBFIRST, SPI_MODE0));
  spiA->transfer(ACCEL_READ);
  spiA->transfer(reg);            // Register address
  for (int i = 0; i < len; i++) {
    buf[i] = spiA->transfer(0);   // Read data byte(s)
  }
  spiA->endTransaction();
  digitalWrite(pin_csn, HIGH);
}


void reg_write(uint8_t reg, uint8_t buf)
{
  digitalWrite(pin_csn, LOW);
  spiA->beginTransaction(SPISettings(spi_hz, MSBFIRST, SPI_MODE0));
  spiA->transfer(ACCEL_WRITE);
  spiA->transfer(reg);            // Register address
  spiA->transfer(buf);            // Write data byte(s)
  spiA->endTransaction();
  digitalWrite(pin_csn, HIGH);
}


void reg_write16(uint8_t reg, uint16_t buf)
{
  digitalWrite(pin_csn, LOW);
  spiA->beginTransaction(SPISettings(spi_hz, MSBFIRST, SPI_MODE0));
  spiA->transfer(ACCEL_WRITE);
  spiA->transfer(reg);            // Register address
  spiA->transfer(buf & 0xFF);     // Write data byte(s)
  spiA->transfer(buf >> 8);
  spiA->endTransaction();
  digitalWrite(pin_csn, HIGH);
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


void graph_vals(int16_t* a_val)
{
  int16_t gr_y;
  int i;

  // Draw a thick black line to erase previous point data and give indication of where we are
  tft.fillRect(gr_x, 0, 2, 320, ILI9488_BLACK);
    
  // Draw the scaled points and axis lines
  for (i=0; i<3; i++) {
    if (gr_x < 6) {
      // Redraw axis name
      tft.setCursor(1, axis_offsets[i] + 2);
      tft.print(axis_names[i]);      
    }
    tft.writePixel(gr_x, axis_offsets[i], ILI9488_DARKGREY);
    gr_y = axis_offsets[i] + (a_val[i] / GR_SCALE);
    tft.writePixel(gr_x, gr_y, axis_colors[i]);
  }

  // Setup for next time
  if (++gr_x == 480) gr_x = 0;
}
