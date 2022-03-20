/*
 * MJPEG file playback demo.  Plays back mjpeg files from the Micro-SD Card selected 
 * using the Arduino serial monitor.
 * 
 * Requires TFT_eSPI and TJpg_Decoder libraries.
 * 
 * Operation
 *   1. Load mjpeg movies onto a Micro-SD card.  Video size should be 480x320 pixels
 *      or less. 
 *   2. Insert Micro-SD card before power on
 *   3. Open the serial monitor (115200 baud) With Arduino connected to gCore.
 *   4. Power on gCore.  You should see a prompt requesting you to enter the filename
 *      to play back.  By default the program will play "/pexels.mjp" which is included
 *      in the respository "supporting" directory.  Hit return or enter the filename and
 *      hit return to start playback.
 *   5. After playback the program will print performance statistics about the video.
 * 
 * By default the program uses the TFT_eSPI DMA support.  This can be disabled at a slight
 * performance penalty by compiling with the USE_DMA directive commented out.
 * 
 */
#include <SD.h>
#include <SD_MMC.h>
#include <TFT_eSPI.h>
#include <TJpg_Decoder.h>

// Uncomment to use eSPI_TFT DMA
#define USE_DMA

// Default file (load this file from the repository onto a Micro-SD Card)
#define MJPEG_FILENAME "/pexels.mjp"

// Buffer sizes (individual jpeg images in the video must be <= MJPEG_BUF_LEN in size)
#define READ_BUF_LEN   8192
#define MJPEG_BUF_LEN  32768
#define DMA_BUF_LEN    512

// JPEG stream decode state
#define ST_IDLE    0
#define ST_FF_1    1
#define ST_IN_JPEG 2
#define ST_FF_2    3



// Variables
char file_name[80];
uint8_t buf[READ_BUF_LEN];
uint8_t mjpeg_buf[MJPEG_BUF_LEN];
uint16_t dma_buf[DMA_BUF_LEN/2];

unsigned long total_msec;
unsigned long jpeg_msec;
unsigned long draw_msec;
int num_frames;
int max_jpg_len;

File vFile;

TFT_eSPI tft;


void setup() {
  Serial.begin(115200);

  // Configure the driver
  tft.begin();
  tft.setRotation(3);
  tft.fillScreen(TFT_BLACK);
#ifdef USE_DMA
  tft.initDMA();
#endif

  // Configure the decoder
  TJpgDec.setJpgScale(1);
  TJpgDec.setSwapBytes(true);  // Fastest way to swap bytes for 16bpp
  TJpgDec.setCallback(TftOutput);

  // Configure a 4-bit SD Interface
  if (!SD_MMC.begin()) {
    Serial.println("ERROR: SD card mount failed!");
    while (1) {
      delay(1000);
    }  
  }

  delay(1000);
}


void loop() {
  fs::FS &fs = SD_MMC;
  
  GetFileName();

  vFile = fs.open(file_name);
  if (vFile.isDirectory()) {
    Serial.printf("ERROR: %s is directory\n", file_name);
  }
  else if (!vFile) {
    Serial.printf("ERROR: Failed to open %s file for reading\n", file_name);
  } else {
    Serial.printf("Opened file %s\n", file_name);
    tft.fillScreen(TFT_BLACK);
    total_msec = millis();
    jpeg_msec = 0;
    draw_msec = 0;
    num_frames = 0;
    max_jpg_len = 0;
    ProcessFile();
    vFile.close();

    // Display statistics for this video
    total_msec = millis() - total_msec;
    Serial.printf("Total = %u mSec\n", total_msec);
    Serial.printf("  file read   = %u\n", total_msec - jpeg_msec);
    Serial.printf("  jpeg decode = %u\n", jpeg_msec - draw_msec);
    Serial.printf("  render      = %u\n", draw_msec);
    Serial.printf("Frames = %d, fps = %1.1f\n", num_frames, 1000.0 / (float) (total_msec / num_frames));
    Serial.printf("Max image length = %d\n", max_jpg_len);
  }
}


// Allow the user to enter a mjpeg filename on the SD Card or just use the default file
void GetFileName()
{
  bool done = false;
  char c;
  int i = 0;

  strcpy(file_name, MJPEG_FILENAME);
  Serial.printf("Enter filename or just hit RETURN (default %s)\n", file_name);
  while (!done) {
    if (Serial.available()) {
      c = Serial.read();
      if (c == 13) {
        if (i != 0) {
          file_name[i] = 0;
        }
        done = true;
      } else if (c != 10) {
        file_name[i++] = c;
      }
    }
  }
}


// Read the mjpeg file, extracting individual images for display
void ProcessFile()
{
  int fst = ST_IDLE;
  int i;
  int read_len;
  uint8_t* bP;
  uint8_t b;

  while (read_len = vFile.read(buf, READ_BUF_LEN)) {
    bP = buf;
    while (read_len--) {
      b = *bP++;
      switch (fst) {
        case ST_IDLE:
          if (b == 0xFF) {
            fst = ST_FF_1;
          }
          break;
        
        case ST_FF_1:
          if (b == 0xD8) {
            i = 0;
            mjpeg_buf[i++] = 0xFF;
            mjpeg_buf[i++] = b;
            fst = ST_IN_JPEG;
          } else {
            fst = ST_IDLE;
          }
          break;
        
        case ST_IN_JPEG:
          if (i > MJPEG_BUF_LEN) {
            Serial.printf("Skipping large image at frame %d\n", num_frames + 1);
            fst = ST_IDLE;
          } else {
            mjpeg_buf[i++] = b;
            if (b == 0xFF) {
              fst = ST_FF_2;
            }
          }
          break;
        
        case ST_FF_2:
          if (i > MJPEG_BUF_LEN) {
            Serial.printf("Skipping large image at frame %d\n", num_frames + 1);
            fst = ST_IDLE;
          } else {
            mjpeg_buf[i++] = b;
            if (b == 0xD9) {
              fst = ST_IDLE;
              num_frames++;
              if (!DisplayImage(i)) {
                return;
              }
            } else {
              fst = ST_IN_JPEG;
            }
          }
          break;
      }
    }
  }
}


// Decode and display a jpeg image
bool DisplayImage(int len)
{
  JRESULT res;
  unsigned long start = millis();

  if (len > max_jpg_len) max_jpg_len = len;

#ifdef USE_DMA
  tft.startWrite();
#endif
  res = TJpgDec.drawJpg(0, 0, mjpeg_buf, (uint32_t) len);
#ifdef USE_DMA
  tft.endWrite();
#endif
  if (res != JDR_OK) {
    Serial.printf("drawJpg failed with %d\n", (int) res);
    return false;
  }
  jpeg_msec += millis() - start;
  
  return true;
}


// Callback for the jpeg decoder
bool TftOutput(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap)
{
  int n = w*h;
  uint16_t* rP;
  unsigned long start = millis();
  
   // Stop further decoding as image is running off bottom of screen
  if (y >= tft.height()) return 0;

#ifdef USE_DMA
  // Load the buffer for an asynchronous DMA (control falls out of pushImageDMA before
  // DMA is finished by copying bitmap to dma_buf and using that for the DMA)
  tft.dmaWait();
  tft.pushImageDMA(x, y, w, h, bitmap, dma_buf);
#else
  tft.pushImage(x, y, w, h, bitmap);
#endif

  draw_msec += millis() - start;

  // Return 1 to decode next block
  return 1;
}
