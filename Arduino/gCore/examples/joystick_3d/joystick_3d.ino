/********************************************************************
 * 3D mesh rendering demo on gCore using Arvind Singh's tgx 3D library,
 * an optional Sparkfun I2C Joystick and both ESP32 processors.
 * 
 *    1. tgx rendering code draws the "naruto" 3D mesh into a 160x240
 *       pixel dual ping-pong frame buffer.  Its task runs on CPU 0.
 *    2. A display task running on CPU 1 pixel-doubles the image to
 *       320x480 pixels and uses DMA to draw data to the LCD.
 *    3. The Sparkfun joystick pans around the model (horizontal)
 *       and/or zooms in/out from the model (vertical).  The joystick
 *       button toggles between Texture mapped, Gouraud or flat shading
 *       models.
 *    4. gCore is configured so that short presses of the power button
 *       turn gCore on and off.
 *       
 * The demo is limited by the 3D rendering process on CPU 1 but demonstrates
 * very fast LCD updates.
 * 
 * The demo is based on the tgx ESP32/naruto demo.  Many thanks to Arvind
 * for creating the wonderful tgx library!
 * 
 * Requires the following libraries:
 *    1. Bodmer's TFT_eSPI library ported to gCore (download from
 *       https://github.com/danjulio/TFT_eSPI)
 *    2. Sparkfun Qwiic Joystick Arduino Library (available from Arduino 
 *       library manager or from the Documents tab at Sparkfun's
 *       website https://sparkfun.com/products/15168).
 *    3. tgx 3D library (download from https://github.com/vindar/tgx)
 *    4. gCore library (https://github.com/danjulio/gCore2)
 *    
 * Further information about the Sparkfun joystick can be found at
 * https://www.sparkfun.com/products/15168.  The joystick may be connected
 * using a Qwiic cable to the Qwiic connectonr on gCore.  Make sure the 
 * joystick is in the middle position when starting the code.  This demo
 * does not require the joystick and will work in an animated fashion 
 * without it.  Be sure to uncomment the line
 * 
 * #define USE_JOYSTICK
 * 
 * to use the joystick.
 * 
 ********************************************************************/

// Uncomment this line to compile with support for the Sparkfun joystick
//#define USE_JOYSTICK

// Include files
#include "gCore.h"
#include <TFT_eSPI.h>
#include <tgx.h>

#ifdef USE_JOYSTICK
#include "SparkFun_Qwiic_Joystick_Arduino_Library.h"
#endif

// Mesh to draw
#include "naruto.h"

// Don't require always using the tgx:: prefix
using namespace tgx;

#define SWAP_BYTES(d) (d = d << 8 | d >> 8)

// gCore driver
gCore gc = gCore();

// Screen driver
TFT_eSPI tft = TFT_eSPI();

// Joystick driver
#ifdef USE_JOYSTICK
JOYSTICK joystick;
#endif

// Joystick center offset values
#define JOYSTICK_GB 32
bool joystick_present = false;
uint16_t j_center_x;
uint16_t j_center_y;

// Horribly terrible semaphores
bool fb_full = false;
bool fb2_full = false;

// size of the drawing framebuffer (half the LCD display for speed's state)
#define SLX (320/2)
#define SLY (480/2)

// Framebuffers tgx draws into
uint16_t * fb;
uint16_t * fb2;

// Small buffers for eSPI_TFT DMA transfers (SLY must be even multiple of NUM_DRAW_LINES)
// These buffers are used to double the rendered image as it's drawn to the LCD
#define NUM_DRAW_LINES 80
#define DRAW_BUF_LEN (2*NUM_DRAW_LINES*SLX)
uint16_t* rbuf1;
uint16_t* rbuf2;

// z-buffer
float* zbuf;

// 3D mesh drawer (with zbuffer and perspective projection)
Renderer3D<RGB565, SLX, SLY, true, false> renderer;

// Tasks to run on different CPUs
TaskHandle_t disp_task_handle;
TaskHandle_t draw_task_handle;



// ==================================================================================
// Arduino entry points
//

void setup() 
{
    Serial.begin(115200);

    // Initialize the power button for 100 mSec press and brightness to 80%
    gc.begin();
    gc.power_set_button_short_press_msec(100);
    gc.power_set_brightness(80);

    // Allocate the framebuffers in external PSRAM (due to size)
    fb = (uint16_t*)heap_caps_malloc(SLX * SLY * sizeof(uint16_t), MALLOC_CAP_SPIRAM);
    while (fb == nullptr) {
        Serial.println("Error: cannot allocate memory for fb");
        delay(1000);
    }
    fb2 = (uint16_t*)heap_caps_malloc(SLX * SLY * sizeof(uint16_t), MALLOC_CAP_SPIRAM);
    while (fb2 == nullptr) {
        Serial.println("Error: cannot allocate memory for fb2");
        delay(1000);
    }

    // Allocate the zbuffer in external PSRAM
    zbuf = (float*)heap_caps_malloc(SLX * SLY * sizeof(float), MALLOC_CAP_SPIRAM);
    while (zbuf == nullptr) {
        Serial.println("Error: cannot allocate memory for zbuf");
        delay(1000);
    }

    // Allocate the LCD render buffers in internal RAM (for DMA access)
    rbuf1 = (uint16_t*)malloc(DRAW_BUF_LEN * sizeof(uint16_t));
    while (rbuf1 == nullptr) {
        Serial.println("Error: cannot allocate memory for rbuf2");
        delay(1000);
    }
    
    rbuf2 = (uint16_t*)malloc(DRAW_BUF_LEN * sizeof(uint16_t));
    while (rbuf2 == nullptr) {
        Serial.println("Error: cannot allocate memory for rbuf2");
        delay(1000);
    }

    // Initialize the LCD driver
    tft.init();
    tft.setRotation(0);
    tft.fillScreen(TFT_BLACK);
    tft.initDMA();
    tft.startWrite();

    // Get baseline readings from the joystick
#ifdef USE_JOYSTICK
    if (joystick.begin()) {
        joystick_present = true;
        // Read joystick until it's about centered
        get_joystick_center_offsets();
        Serial.printf("Detected joystick - center: %d %d\n", j_center_x, j_center_y);
    }
#endif

    // Setup the 3D renderer
    renderer.setZbuffer(zbuf, SLX * SLY); // set the z buffer for depth testing
    renderer.setPerspective(45, ((float)SLX) / SLY, 0.1f, 1000.0f);  // set the perspective projection matrix.     
    renderer.setMaterial(RGBf(0.85f, 0.55f, 0.25f), 0.2f, 0.7f, 0.8f, 64); // bronze color with a lot of specular reflexion. 
    renderer.setOffset(0, 0);

    // Finally start tasks
    xTaskCreatePinnedToCore(disp_task, "disp_task", 3072, NULL, 1, &disp_task_handle, 0);  // PRO CPU
    xTaskCreatePinnedToCore(draw_task, "draw_task", 10800, NULL, 1, &draw_task_handle, 1); // APP CPU
}


void loop() 
{  
  // Main loop does nothing
  delay(1000);
}


#ifdef USE_JOYSTICK
void get_joystick_center_offsets()
{
  int i;
  uint16_t x = 0;
  uint16_t y = 0;

  // Read each axis until it's close to the center value, then read a small number and get an average for the 
  // value to use as our center.
  while ((x < (512 - JOYSTICK_GB)) || (x > (512 + JOYSTICK_GB))) {
    x = joystick.getHorizontal();
  }
  j_center_x = 0;
  for (i=0; i<4; i++) {
    x = joystick.getHorizontal();
    j_center_x += x;
  }
  j_center_x /= 4;

  while ((y < (512 - JOYSTICK_GB)) || (y > (512 + JOYSTICK_GB))) {
    y = joystick.getVertical();
  }
  j_center_y = 0;
  for (i=0; i<4; i++) {
    y = joystick.getVertical();
    j_center_y += y;
  }
  j_center_y /= 4;
}
#endif



// ==================================================================================
// Display task
//

void disp_task(void * parameter)
{
  while (1) {
    // Draw any valid frame buffers
    if (fb_full) {
      render_to_lcd(fb);
      fb_full = false;
    }
    if (fb2_full) {
      render_to_lcd(fb2);
      fb2_full = false;
    }

    // Allow FreeRTOS to schedule (and keep the watchdog timer from expiring)
    vTaskDelay(pdMS_TO_TICKS(20));
  }
}


void render_to_lcd(uint16_t* p)
{
  int n = 0;
  int x;
  uint16_t* fbP = p;
  uint16_t* rP;
  uint16_t pixel;
  
  while (fbP < (p + (SLX * SLY))) {
    // Swap bytes and pixel-double image into rbuf1 (in parallel with previous DMA)
    rP = rbuf1;
    x = 0;
    while (rP < (rbuf1 + DRAW_BUF_LEN)) {
      pixel = SWAP_BYTES(*fbP);
      *rP++ = pixel;
      *rP++ = pixel;
      fbP++;
      if (++x == SLX) {
        // Built up one line: now double it
        memcpy(rP, rP - (2 * SLX), 4 * SLX);
        rP += (2 * SLX);
        x = 0;
      }
    }

    // Load the buffer for an asynchronous DMA (control falls out of pushImageDMA before DMA is finished
    // by copying rbuf1 to rbuf2 and using that for the DMA)
    tft.dmaWait();
    tft.pushImageDMA(0, NUM_DRAW_LINES * n++, 2 * SLX, NUM_DRAW_LINES, rbuf1, rbuf2);
  }
}



// ==================================================================================
// 3D Render task
//

void draw_task(void * parameter)
{
  bool pp = true;   // ping-pong control
  bool btn;
  fMat4 M;
  int cur_shader = 2;
  int shader = TGX_SHADER_GOURAUD | TGX_SHADER_TEXTURE;
  
  // Set the images that encapsulate the frame buffers
  Image<RGB565> imfb(fb,SLX,SLY);
  Image<RGB565> imfb2(fb2,SLX,SLY);
    
  while (1) {
    // Compute the model position
    moveModel(&M, &btn);
    renderer.setModelMatrix(M);
    
    // Change the shader every time the joystick button is pressed
    if (btn) {
      switch ((++cur_shader) % 3) {
        case 0: shader = TGX_SHADER_FLAT; break;
        case 1: shader = TGX_SHADER_GOURAUD; break;
        case 2: shader = TGX_SHADER_GOURAUD | TGX_SHADER_TEXTURE; break;
      }
    }

    // Draw the 3D mesh into one of the ping-pong frame buffers
    if (pp) {
      while (fb_full) {};
      renderer.setImage(&imfb);           // set the image to draw onto (ie the screen framebuffer)
      imfb.fillScreen(RGB565_Black);             // clear the framebuffer (black background)
    } else {
      while (fb2_full) {};
      renderer.setImage(&imfb2);
      imfb2.fillScreen(RGB565_Black);
    }
    renderer.clearZbuffer();                     // clear the z-buffer
    renderer.drawMesh(shader, &naruto_1, false); // draw the mesh !

    // Signal display task and flip ping-pong buffer
    if (pp) {
      fb_full = true;
      pp = false;
    } else {
      fb2_full = true;
      pp = true;
    }

    // Check for button press for soft power off (done in the same task that accesses the joystick since
    // both use I2C and we don't want different tasks trying to access it at the same time)
    if (gc.power_button_pressed()) {
      gc.power_off();
    }

    // Allow FreeRTOS to schedule (and keep the watchdog timer from expiring)
    vTaskDelay(pdMS_TO_TICKS(20));
  }
}


/* Compute the model matrix based on either joystick input or simple animation */  
void moveModel(fMat4* M, bool* btn)
{
  const float dilat = 9; // scale model
  static int num_evals = 0;
  uint16_t jx, jy;
  static float djx = -0.5;      // Default values for use when joystick is not present
  static float djy = 0.5;
  static float ty = 0;
  static float tz = -25;
  static float roty = 0;

#ifdef USE_JOYSTICK
  if (joystick_present) {
    // Check for the joystick button press
    *btn = joystick.checkButton() != 0;
    
    // Read the joystick, apply a guard band then compute a range -1 to 1 for each axis
    //   Left-Right => jx 1023 - 0
    //   Up-Down => jy 0 - 1023
    jx = joystick.getHorizontal();
    jy = joystick.getVertical();
    if (jx < (j_center_x - JOYSTICK_GB)) {
      djx = (float) (j_center_x - jx) / j_center_x;
    } else if (jx > (j_center_x + JOYSTICK_GB)) {
      djx = -1.0 * (float) (jx - (1024.0 - j_center_x)) / (1024.0 - j_center_x);
    } else {
      djx = 0;
    }
     
    if (jy < (j_center_y - JOYSTICK_GB)) {
      djy = (float) (j_center_y - jy) / j_center_y;
    } else if (jy > (j_center_y + JOYSTICK_GB)) {
      djy = -1.0 * (float) (jy - (1024.0 - j_center_y)) / (1024.0 - j_center_y);
    } else {
      djy = 0;
    }
    //Serial.printf("%d %d -> %f %f\n", jx, jy, djx, djy);
  } else {
#endif
    // Automatic animation is based on number of frames
    *btn = ((num_evals % 25) == 0);

    if ((num_evals % 50) == 0) {
      // Flip zoom
      djy *= -1.0;
    }
#ifdef USE_JOYSTICK
  }
#endif

  // Compute rotaton based on horizontal axis
  roty += djx * 10;
  if (roty < 0) roty += 360;
  if (roty >= 360) roty -= 360;

  // Compute translation toward or from the model based on the vertical axis (values chosen for nice visuals)
  ty += -1.0 * djy * 0.39;
  if (ty < -7) ty = -7;
  if (ty > 0) ty = 0;
  
  tz += djy * 1;
  if (tz < -25) tz = -25;
  if (tz > -7) tz = -7;

  // Set the matrix
  M->setScale({ dilat, dilat, dilat }); // scale the model
  M->multRotate(-roty, { 0,1,0 }); // rotate around y
  M->multTranslate({ 0,ty, tz }); // translate

  num_evals += 1;
}
