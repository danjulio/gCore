/*
 * The image decode and display task and supporting code running on the Application CPU
 * 
 * This task is typically the bottleneck because of the JPEG decode (the more complex
 * and larger the iamge the slower the decode).
 */

// ====================================
// Display Constants

// TFT_eSPI internal DMA buffer
#define DMA_BUF_LEN    512

// Number of frames to use to compute average FPS
#define NUM_FPS_FRAMES 5



// ====================================
// Display Variables
uint16_t dma_buf[DMA_BUF_LEN/2];



// ====================================
// Task
// ====================================
void disp_task(void * parameter)
{
  int disp_img_index = 0;  // Ping-pong buffer index
  int frame_count = 0;
  float fps;
  JRESULT res;
  unsigned long timestamp;
  
  while (true) {
    // Wait for an image to become available
    while (!imgs[disp_img_index].img_ready) {
      delay(10);
    }

    if (frame_count == 0) {
      timestamp = millis();
    }
    
#ifdef USE_DMA
    tft.startWrite();
#endif
    res = TJpgDec.drawJpg(0, 0, imgs[disp_img_index].img, (uint32_t) imgs[disp_img_index].img_len);
#ifdef USE_DMA
    tft.endWrite();
#endif
    if (res != JDR_OK) {
      Serial.printf("drawJpg failed with %d\n", (int) res);
    }

    // Setup for next image
    imgs[disp_img_index].img_ready = false;
    if (++disp_img_index >= 2) disp_img_index = 0;

    // Update FPS in lower right corner
    if (++frame_count == NUM_FPS_FRAMES) {
      fps = (frame_count * 1000.0) / (float) (millis() - timestamp);
      frame_count = 0;
      tft.fillRect(450, 310, 30, 10, TFT_BLACK);
      tft.setCursor(450, 310);
      tft.print(fps);
    }
  }
}


// ====================================
// JPEG decode display callback
// ====================================
bool draw_pixels(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap)
{
  // Stop further decoding as image is running off bottom of screen
  if (y >= tft.height()) return false;

#ifdef USE_DMA
  // Load the buffer for an asynchronous DMA (control falls out of pushImageDMA before
  // DMA is finished by copying bitmap to dma_buf and using that for the DMA)
  tft.dmaWait();
  tft.pushImageDMA(x, y, w, h, bitmap, dma_buf);
#else
  tft.pushImage(x, y, w, h, bitmap);
#endif

  // Return true to decode next block
  return true;
}
