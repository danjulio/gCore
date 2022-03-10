/*
 * Read/Write files to a Micro-SD Card using various modes on gCore to compare performance..
 *   1. SPI mode (slowest)
 *   2. 1-bit mode
 *   3. 4-bit mode (fastest)
 *   
 *   Based on the original ESP SD Benchmark program.  Writes and then reads a 4 MB file using
 *   different block sizes (1k, 2k, 4k, 8k, 16k, 32k, 64k) and outputs the speed of the 
 *   operation in KB/sec.
 *   
 *   Be sure to have a fast Micro-SD Card installed and open the serial monitor at
 *   115200 baud to see the test results.  The test power cycles gCore between modes
 *   in order to reset the ESP32 peripherls and the Micro-SD Card which I found necessary
 *   or switching to a new mode would fail.
 *   
 */
#include "FS.h"
#include "SD.h"
#include "SD_MMC.h"
#include "gCore.h"

#define TEST_FILE_SIZE (4 * 1024 * 1024)

// Hardware objects
gCore gc = gCore();



void testWriteFile(fs::FS &fs, const char *path, uint8_t *buf, int len)
{
  unsigned long start_time = millis();
  Serial.printf("Test write %s\n", path);

  File file = fs.open(path, FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open file for writing");
    return;
  }
  int loop = TEST_FILE_SIZE / len;
  while (loop--)
  {
    if (!file.write(buf, len)) {
      Serial.println("Write failed");
      return;
    }
  }
  file.flush();
  file.close();
  unsigned long time_used = millis() - start_time;
  Serial.printf("Write file used: %d ms, %f KB/s\n", time_used, (float)TEST_FILE_SIZE / time_used);
}


void testReadFile(fs::FS &fs, const char *path, uint8_t *buf, int len)
{
  unsigned long start_time = millis();
  Serial.printf("Test read %s\n", path);

  File file = fs.open(path);
  if (!file) {
    Serial.println("Failed to open file for reading");
    return;
  }
  int loop = TEST_FILE_SIZE / len;
  while (loop--)
  {
    if (!file.read(buf, len)) {
      Serial.println("Read failed");
      return;
    }
  }
  file.close();
  unsigned long time_used = millis() - start_time;
  Serial.printf("Read file used: %d ms, %f KB/s\n", time_used, (float)TEST_FILE_SIZE / time_used);
}


void testIO(fs::FS &fs)
{
  /* malloc will not reset all bytes to zero, so it is a random data */
  uint8_t *buf = (uint8_t*)malloc(64 * 1024);

  testWriteFile(fs, "/test_1k.bin", buf, 1024);
  testWriteFile(fs, "/test_2k.bin", buf, 2 * 1024);
  testWriteFile(fs, "/test_4k.bin", buf, 4 * 1024);
  testWriteFile(fs, "/test_8k.bin", buf, 8 * 1024);
  testWriteFile(fs, "/test_16k.bin", buf, 16 * 1024);
  testWriteFile(fs, "/test_32k.bin", buf, 32 * 1024);
  testWriteFile(fs, "/test_64k.bin", buf, 64 * 1024);

  testReadFile(fs, "/test_1k.bin", buf, 1024);
  testReadFile(fs, "/test_2k.bin", buf, 2 * 1024);
  testReadFile(fs, "/test_4k.bin", buf, 4 * 1024);
  testReadFile(fs, "/test_8k.bin", buf, 8 * 1024);
  testReadFile(fs, "/test_16k.bin", buf, 16 * 1024);
  testReadFile(fs, "/test_32k.bin", buf, 32 * 1024);
  testReadFile(fs, "/test_64k.bin", buf, 64 * 1024);
}


void wait_for_card_insertion() {
  uint8_t reg;

  (void) gc.gcore_get_reg8(GCORE_REG_GPIO, &reg);
  while ((reg & GCORE_GPIO_SD_CARD_MASK) == 0) {
    delay(10);
    (void) gc.gcore_get_reg8(GCORE_REG_GPIO, &reg);
  }
}


void reboot_gcore() {
  uint32_t t;

  // Get the current RTC value
  (void) gc.gcore_get_time_secs(&t);

  // Set an alarm for 2 seconds in the future
  t += 2;
  (void) gc.gcore_set_alarm_secs(t);

  // Enable the alarm to wake us
  (void) gc.gcore_set_reg8(GCORE_REG_WK_CTRL, GCORE_WK_ALARM_MASK);

  // Power off
  gc.power_off();
}


void setup() {
  uint8_t n;
  SPIClass spi = SPIClass(HSPI);
  
  // put your setup code here, to run once:
  Serial.begin(115200);
  gc.begin();

  // Get NVRAM byte 0x400 (by default this is 0 when a battery is first attached, we use it to
  // keep state between power cycles)
  (void) gc.gcore_get_nvram_byte(0x400, &n);
  if (n > 2) n = 0;

  // Only print some stuff at the beginning
  if (n == 0) {
    Serial.println("Starting SD Card Benchmark\n");
    Serial.print("Waiting for card insertion...");
    wait_for_card_insertion();
    Serial.println("inserted");
  }

  // Run a test and then power cycle gCore for the next test
  switch (n) {
    case 0: // test SD SPI Mode using the HSPI peripheral
      spi.begin(14 /* SCK */, 2 /* MISO */, 15 /* MOSI */, 13 /* SS */);
      if (!SD.begin(13 /* SS */, spi, 20000000)) {
        Serial.println("HSPI Card Mount Failed");
        delay(10);
        gc.power_off();
      }
      Serial.println("\nHSPI Card Tests:");
      testIO(SD);
      
      // Setup for the next test
      (void) gc.gcore_set_nvram_byte(0x400, n + 1);
      reboot_gcore();
      break;
    
    case 1: // test SD_MMC 1-bit Mode
      if (!SD_MMC.begin("/sdcard", true)) {
        Serial.println("1-bit Card Mount Failed");
        delay(10);
        gc.power_off();
      }
      Serial.println("\n1-bit Card Tests:");
      testIO(SD_MMC);
      
      // Setup for the next test
      (void) gc.gcore_set_nvram_byte(0x400, n + 1);
      reboot_gcore();
      break;

    case 2: // test SD_MMC 4-bit Mode
      if (!SD_MMC.begin()) {
        Serial.println("4-bit Card Mount Failed");
        delay(10);
        gc.power_off();
      }
      Serial.println("\n4-bit Card Tests:");
      testIO(SD_MMC);
      SD_MMC.end();

      // Final test
      Serial.println("Done!");
      delay(10);
      (void) gc.gcore_set_nvram_byte(0x400, 0);  // Reset for next time
      gc.power_off();
      break;
  }
}


void loop() {
}
