/*
 * The HTTP client task and supporting code running on the Protocol CPU
 * 
 * This task brute-forces converting the stream into individual JPEG images
 * by simply parsing all received bytes, looking for the 0xFFD8 and 0xFFD9
 * delimiters.  A more "web-centric" process might parse the stream into the
 * HTTP delimiter, headers and data and only process the data (but this would
 * incur more overhead).
 */

// ====================================
// Com Constants
// ====================================

// JPEG stream decode state
#define ST_IDLE    0
#define ST_FF_1    1
#define ST_IN_JPEG 2
#define ST_FF_2    3



// ====================================
// Com Variables
// ====================================
WiFiClient client;



// ====================================
// Task
// ====================================
void com_task(void * parameter)
{
  int read_len;
  
  while (true) {
    // Attempt to handle wifi connection/disconnection
    while (WiFi.status() != WL_CONNECTED) {
      delay(100);
    }
    
    Serial.println("Connecting to server");
    if (client.connect(STREAM_IP, STREAM_PORT)) {
      // Request stream
      client.println("GET / HTTP/1.0");
      client.println();

      // Process data from the stream
      while (client.connected()) {
        read_len = client.available();
        if (read_len != 0) {
          com_process_data(read_len);
        } else {
          delay(10);  // Delay to let FreeRTOS scheduler work
        }
      }
      Serial.println("Server disconnected");
      client.stop();
    } else {
      Serial.println("Could not connect to server");
      delay(1000);
    }
  }  
}


// ====================================
// Com Routines
// ====================================
void com_process_data(int len)
{
  static bool push_img = false;
  static int fst = ST_IDLE;
  static int com_img_index = 0;  // Ping-pong buffer index
  static uint8_t* imgP;
  uint8_t c;
  
  while (len-- > 0) {
    c = client.read();
    
    switch (fst) {
      case ST_IDLE:
        if (c == 0xFF) {
          fst = ST_FF_1;
        }
        break;
      
      case ST_FF_1:
        if (c == 0xD8) {
          // Start of JPEG image - only load if the buffer is available, otherwise just
          // throw this image away
          if (!imgs[com_img_index].img_ready) {
            push_img = true;
            imgP = imgs[com_img_index].img;
            *imgP++ = 0xFF;
            *imgP++ = c;
          } else {
            push_img = false;
          }
          fst = ST_IN_JPEG;
        } else {
          fst = ST_IDLE;
        }
        break;
      
      case ST_IN_JPEG:
        if (push_img) *imgP++ = c;
        if (c == 0xFF) {
          fst = ST_FF_2;
        }
        break;
      
      case ST_FF_2:
        if (push_img) *imgP++ = c;
        if (c == 0xD9) {
          // End of JPEG image - Let display task know and move to the next ping-pong half
          fst = ST_IDLE;
          if (push_img) {
            imgs[com_img_index].img_len = imgP - imgs[com_img_index].img;
            imgs[com_img_index].img_ready = true;
            if (++com_img_index >= 2) com_img_index = 0;
          }
        } else {
          fst = ST_IN_JPEG;
        }
        break;
    }
  }
}
