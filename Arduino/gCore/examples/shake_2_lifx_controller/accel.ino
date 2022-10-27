/*
 * ADXL362 support functions
 */

// ================================================
// CONSTANTS
// ================================================

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


// ================================================
// API
// ================================================
void accel_init()
{
  uint8_t reg;
  
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
}


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


// ================================================
// MODULE ROUTINES
// ================================================
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
