/*
 * FT6206 I2C driver for LVGL - configured for portrait mode
 * 
 */

// ==================================================
// Constants
//

#define FT62XX_ADDR           0x38
#define FT62XX_G_FT5201ID     0xA8
#define FT62XX_REG_NUMTOUCHES 0x02

#define FT62XX_NUM_X             0x33
#define FT62XX_NUM_Y             0x34

#define FT62XX_REG_MODE 0x00
#define FT62XX_REG_CALIBRATE 0x02
#define FT62XX_REG_WORKMODE 0x00
#define FT62XX_REG_FACTORYMODE 0x40
#define FT62XX_REG_THRESHHOLD 0x80
#define FT62XX_REG_POINTRATE 0x88
#define FT62XX_REG_FIRMVERS 0xA6
#define FT62XX_REG_CHIPID 0xA3
#define FT62XX_REG_VENDID 0xA8

#define FT62XX_VENDID 0x11
#define FT6206_CHIPID 0x06
#define FT6236_CHIPID 0x36
#define FT6236U_CHIPID 0x64 // mystery!

#define FT62XX_DEFAULT_THRESHOLD 128


// Calibration constants
#define TS_X_MIN       0
#define TS_Y_MIN       0
#define TS_X_MAX       320
#define TS_Y_MAX       480
#define TS_XY_SWAP     false
#define TS_X_INV       false
#define TS_Y_INV       false


// ==================================================
// lvgl integration
//
bool lvgl_tp_read(lv_indev_drv_t * indev_driver, lv_indev_data_t *data)
{
  uint8_t i2cdat[16];
  static int16_t last_x = 0;
  static int16_t last_y = 0;
  bool valid;
  int c = 0;
  int16_t x = 0;
  int16_t y = 0;

  
  // Only handle one touch
  c = ftReadRegister8(FT62XX_REG_NUMTOUCHES);
  if (c == 1) {
    // Get data from the chip
    Wire.beginTransmission(FT62XX_ADDR);
    Wire.write((byte)0);  
    Wire.endTransmission();

    Wire.requestFrom((byte)FT62XX_ADDR, (byte)16);
    for (uint8_t i=0; i<16; i++) {
      i2cdat[i] = Wire.read();
    }

    // Get the touch coordinates
    x = i2cdat[0x03] & 0x0F;
    x <<= 8;
    x |= i2cdat[0x04];
    y = i2cdat[0x05] & 0x0F;
    y <<= 8;
    y |= i2cdat[0x06];
#ifdef TS_DEBUG
  Serial.printf("raw %d %d", x, y);
#endif
    _ts_adjust_data(&x, &y);
#ifdef TS_DEBUG
  Serial.printf(" => %d %d\n", x, y);
#endif
    last_x = x;
    last_y = y;
    valid = true;
  } else {
    x = last_x;
    y = last_y;
    valid = false;
  }

  data->point.x = x;
  data->point.y = y;
  data->state = valid == false ? LV_INDEV_STATE_REL : LV_INDEV_STATE_PR;

  return false;
}



// ==================================================
// API Code
//
void ts_begin()
{
  // Initialize the hardware
  Wire.begin();

  // Check if we can communicate
  uint8_t id = ftReadRegister8(FT62XX_REG_CHIPID);
  if ((id != FT6206_CHIPID) && (id != FT6236_CHIPID) && (id != FT6236U_CHIPID)) {
    Serial.println("Error: Cannot open FT");
  } else {
    // Dump info
    Serial.print("Vend ID: 0x"); 
    Serial.println(ftReadRegister8(FT62XX_REG_VENDID), HEX);
    Serial.print("Chip ID: 0x"); 
    Serial.println(ftReadRegister8(FT62XX_REG_CHIPID), HEX);
    Serial.print("Firm V: "); Serial.println(ftReadRegister8(FT62XX_REG_FIRMVERS));
    Serial.print("Point Rate Hz: "); 
    Serial.println(ftReadRegister8(FT62XX_REG_POINTRATE));
    Serial.print("Thresh: "); 
    Serial.println(ftReadRegister8(FT62XX_REG_THRESHHOLD));

    // Set the threshold
    ftWriteRegister8(FT62XX_REG_THRESHHOLD, FT62XX_DEFAULT_THRESHOLD);
  }
}



// ==================================================
// Internal Module Code
//
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


void _ts_adjust_data(int16_t * x, int16_t * y)
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

    (*x) = (uint32_t)((uint32_t)(*x) * LV_HOR_RES) /
           (TS_X_MAX - TS_X_MIN);

    (*y) = (uint32_t)((uint32_t)(*y) * LV_VER_RES) /
           (TS_Y_MAX - TS_Y_MIN);

#if TS_X_INV != 0
    (*x) =  LV_HOR_RES - (*x);
#endif

#if TS_Y_INV != 0
    (*y) =  LV_VER_RES - (*y);
#endif
}
