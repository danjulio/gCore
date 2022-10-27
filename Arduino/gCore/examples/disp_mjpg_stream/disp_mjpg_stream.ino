/*
 * Streaming MJPEG demo.  Connects to an ESP32 CAM running the esp32-cam-webserver
 * sketch found in the supporting directory and displays a mjpeg stream.  Utilizes
 * both CPUs (one handling network connections and the other decoding and displaying
 * jpeg images).
 * 
 * Requires gCore, TFT_eSPI and TJpg_Decoder libraries.
 * 
 * Setup
 *   1. Program an ESP32 CAM with the esp32-cam-webserver sketch
 *   2. Connect a computer to the ESP32 CAM's Wifi (ESP32-CAM/esp32cam)
 *      and verify you can stream in a browser at http://192.168.4.1
 * 
 * Operation
 *   1. Run this code after the ESP32 CAM has booted and is advertising its WiFi.
 * 
 * This code should be able to decode and display MJPEG streams with images up to
 * 480x320 pixels.
 */
#include "gCore.h"
#include <TFT_eSPI.h>
#include <TJpg_Decoder.h>
#include <WiFi.h>

// ====================================
// Program Constants
// ====================================

// Maximum jpeg image size (images are stored in PSRAM)
// We choose a very large value compared to the typical size of images because we have
// plenty of room up there in PSRAM!
#define MAX_JPG_LEN (128 * 1024)

// Uncomment to use eSPI_TFT DMA
#define USE_DMA

// Wifi SSID/password strings - change these to connect to a different source
#define WIFI_SSID "ESP32-CAM"
#define WIFI_PASS "esp32cam"

// Stream URL string - change this to connect to a different source.  It should
// point to a raw mjpeg stream with resolution 480x320 or less.
#define STREAM_IP   "192.168.4.1"
#define STREAM_PORT 81



// ====================================
// Program Variables
// ====================================

// Jpeg image descriptor
typedef struct {
  bool img_ready;
  int img_len;
  uint8_t* img;
} img_descT;


// Hardware objects
gCore gc;
TFT_eSPI tft = TFT_eSPI();

// Jpeg image double buffer - One side is loaded with the next incoming image
// while the other side is being decoded and displayed
img_descT imgs[2];

// Tasks running on different CPUs
TaskHandle_t com_task_handle;
TaskHandle_t disp_task_handle;



// ====================================
// Arduino entry points
// ====================================
void setup() {
  Serial.begin(115200);

  // Setup gCore
  gc.begin();
  gc.power_set_brightness(75);
  gc.power_set_button_short_press_msec(100);

  // Configure the LCD driver
  tft.begin();
  tft.setRotation(3);
  tft.fillScreen(TFT_BLACK);
#ifdef USE_DMA
  tft.initDMA();
#endif

  // Say hello
  tft.setTextColor(TFT_GREEN);
  tft.setTextSize(1);
  tft.setCursor(0, 20);
  tft.println("disp_mjpg_stream starting");

  // Configure the decoder
  TJpgDec.setJpgScale(1);
  TJpgDec.setSwapBytes(true);  // Fastest way to swap bytes for 16bpp
  TJpgDec.setCallback(draw_pixels);

  // Allocate the image buffers in PSRAM
  imgs[0].img_ready = false;
  imgs[0].img = (uint8_t*) heap_caps_malloc(MAX_JPG_LEN, MALLOC_CAP_SPIRAM);
  if (imgs[0].img == NULL) {
    tft.setTextColor(TFT_RED);
    tft.println("Malloc 0 failed");
    while (1) {delay(100);}
  }
  imgs[1].img_ready = false;
  imgs[1].img = (uint8_t*) heap_caps_malloc(MAX_JPG_LEN, MALLOC_CAP_SPIRAM);
  if (imgs[1].img == NULL) {
    tft.setTextColor(TFT_RED);
    tft.println("Malloc 1 failed");
    while (1) {delay(100);}
  }

  // Start the display task on the Application CPU.  It will decode and display the
  // JPEG images.
  // Note: After it starts to draw images code running in the main arduino loop shouldn't
  // draw to the tft
  tft.println("Starting display task");
  xTaskCreatePinnedToCore(disp_task, "disp_task", 4096, NULL, 1, &disp_task_handle, 1);

  // Connect to the camera
  tft.print("Connecting to ");
  tft.print(WIFI_SSID);
  tft.print(" ... ");
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
  }
  tft.println("connected");

  // Start the communication task on the Protocol CPU.  It will request and process
  // the incoming stream.
  tft.println("Starting communication task");
  xTaskCreatePinnedToCore(com_task, "com_task", 4096, NULL, 1, &com_task_handle, 0);
}


void loop() {
  // Main loop looks for power button press periodically
  if (gc.power_button_pressed()) {
    Serial.println("Power down...");
    delay(10);
    gc.power_off();
    while (1) {};
  }
  
  delay(100);
}
