/*
 * gCore RTC/PMIC/NVRAM controller register explorer.  Displays real time
 * values of device I2C registers.  Provides serial command interface to
 * read and write registers and NVRAM.  This program can be used how to use
 * gCore's features.
 * 
 * Command interface runs at 115200 baud in the Arduino serial monitor.  Type
 * 'H' followed by SEND to get a list of commands and syntax.  Note that all numeric
 * arguments are hex values.  For example, the BACKLIGHT register is at offset 14 (hex
 * 0x0E) in the register space and takes a value of 0 - 255 (hex 0 - 0xFF) representing
 * the brightness range 0 - 100%.  To set the brightness to 50% write the value 128 (0x80)
 * using the byte write command:
 * 
 *    B E = 80
 * 
 * followed by the carriage return key or hitting SEND in the Arduino serial monitor.
 * To read the value back:
 * 
 *    B E
 * 
 * And the string:
 * 
 *    B E = 80
 * 
 * is printed in the serial monitor output area.
 * 
 * To power off, either write 0F to the SHUTDOWN register or hold the power button
 * for more than 5 seconds (hard power-off).
 * 
 * Requires the following libraries
 *   1. gCore
 *   2. gCore_ILI9488
 *   3. Adafruit_GFX
 */
#include <Adafruit_GFX.h>
#include <gCore.h>
#include <gCore_ILI9488.h>


//
// Data structure containing information to read and update dynamic values
//
typedef struct {
  int16_t x;           // Upper left coordinate
  int16_t y;
  int16_t w;           // Maximum width of string - used to blank previous string value
  uint8_t reg_size;    // 8, 16 or 32 [bits]
  uint8_t reg_offset;  // Register address - high byte of 16- and 32-bit registers
  uint8_t disp_hex;       // 0: display as decimal, 1: display as hex
  char* reg_name;
  char* reg_units;     // Optional string printed after value for registers with unit values
} gCore_reg_disp_t;

#define NUM_DYN_REG 15

const gCore_reg_disp_t gcr[NUM_DYN_REG] = {
  { 20,  80, 460,  8, GCORE_REG_STATUS,  1, "STATUS" ,      ""},
  { 20, 110, 460,  8, GCORE_REG_GPIO,    1, "GPIO",         ""},
  { 20, 140, 460,  8, GCORE_REG_WK_CTRL, 1, "WAKE CONTROL", ""},
  { 20, 170, 140, 16, GCORE_REG_VU,      0, "V-USB",        "mV"},
  { 20, 200, 140, 16, GCORE_REG_IU,      0, "I-USB",        "mA"},
  { 20, 230, 140, 16, GCORE_REG_VB,      0, "V-BATT",       "mV"},
  { 20, 260, 140, 16, GCORE_REG_IL,      0, "I-LOAD",       "mA"},
  {160, 170, 140,  8, GCORE_REG_BL,      0, "BACKLIGHT",    ""},
  {160, 200, 140,  8, GCORE_REG_SHDOWN,  1, "SHUTDOWN",     ""},
  {160, 230, 140,  8, GCORE_REG_PWR_TM,  0, "BUTTON TO",    "x10 mS"},
  {160, 260, 140,  8, GCORE_REG_NV_CTRL, 1, "NVRAM FLASH",  ""},
  {300, 170, 140, 16, GCORE_REG_TEMP,    0, "TEMP",         "C x10"},
  {300, 200, 140, 32, GCORE_REG_TIME,    0, "TIME",         "sec"},
  {300, 230, 140, 32, GCORE_REG_ALARM,   0, "ALARM",        "sec"},
  {300, 260, 180, 32, GCORE_REG_CORR,    0, "TIME CORRECT", "sec/adj"}
};


//
// Command processor state
//
#define CMD_ST_IDLE 0
#define CMD_ST_CMD 1
#define CMD_ST_VAL 2

#define TERM_CHAR 0x0D


//
// Hardware objects
//
gCore gc;
gCore_ILI9488 tft;


//
// Variables
//
unsigned long prev_msec;
uint32_t prev_reg_val[NUM_DYN_REG];

// Command processor
static int cmd_state = CMD_ST_IDLE;
static char cmd_op;
static uint16_t cmd_arg;
static uint32_t cmd_val;
static bool cmd_has_val;


//
// Help string
//
const char help_string[] = \
"Command Interface (click SEND or hit Carriage Return to execute)\n" \
"  B<n> / B<n>=<v> : Read/Write 8-bit (byte) register\n" \
"  W<n> / W<n>=<v> : Read/Write 16-bit (word) register\n" \
"  L<n> / L<n>=<v> : Read/Write 32-bit (long) register\n" \
"  N<n> / N<n>=<v> : Read/Write 8-bit NVRAM location\n" \
"  H               : Print this help message\n" \
"\n" \
"  Arguments\n" \
"     <n> = register offset - specified as hex number\n" \
"     <v> = register value - specified as a hex number\n" \
"  Example : B E = 80\n" \
"     (sets Backlight to 50% - decimal 128)\n"\
"\n" \
"  Register offsets (hex offset)\n" \
"              ID :  0 (8-bit/RO)\n" \
"         VERSION :  1 (8-bit/RO)\n" \
"          STATUS :  2 (8-bit/RO)\n" \
"            GPIO :  3 (8-bit/RO)\n" \
"           V-USB :  4 (16-bit/RO)\n" \
"           I-USB :  6 (16-bit/RO)\n" \
"          V-BATT :  8 (16-bit/RO)\n" \
"          I-LOAD :  A (16-bit/RO)\n" \
"            TEMP :  C (16-bit, signed/RO)\n" \
"       BACKLIGHT :  E (8-bit/RW)\n" \
"    WAKE CONTROL :  F (8-bit/RW)\n" \
"        SHUTDOWN : 10 (8-bit/RW)\n" \
"       BUTTON TO : 11 (8-bit/RW)\n" \
"     NVRAM FLASH : 12 (8-bit/RW)\n" \
"            TIME : 13 (32-bit/RW)\n" \
"           ALARM : 17 (32-bit/RW)\n" \
"    TIME CORRECT : 1B (32-bit/RW)\n"\
"\n" \
" NVRAM : 0 - FFF : (8-bit/RW)\n" \
"\n" \
"  GPIO Register Masks (hex values)\n" \
"      SD Card present : 8\n" \
"    Power button down : 4\n" \
"      Charge Status 1 : 2\n" \
"      Charge Status 0 : 1\n" \
"\n" \
"  Status Register Masks (hex values)\n" \
"                        Critical Battery : 80\n" \
"                    Power button pressed : 10\n" \
"    Power on due to charge status change :  4\n" \
"                   Power on due to alarm :  2\n" \
"            Power on due to power button :  1\n" \
"\n" \
"  SHUTDOWN Trigger (W): F\n" \
"\n" \
"  NVRAM FLASH Register Busy Mask (R): 1\n" \
"  NVRAM FLASH Register Read trigger (W): 52 (reads 1024 bytes from flash into NVRAM)\n" \
"  NVRAM FLASH Register Write trigger (W): 57 (writes first 1024 bytes from NVRAM to flash)\n" \
"\n" \
"  WAKE CONTROL Register Masks (hex values)\n" \
"     Wake on Charge Done : 4\n" \
"    Wake on Charge Start : 2\n" \
"           Wake on Alarm : 1\n";



//
// Arduino entry points
//
void setup() {
  uint8_t reg8;
  
  // Setup hardware
  Serial.begin(115200);
  gc.begin();
  tft.begin();
  tft.setRotation(3);    // Rotate 270° so display correct with gCore IO at bottom
  tft.fillScreen(ILI9488_BLACK);
  gc.power_set_button_short_press_msec(1000);  // Slow detection to allow user to see delay in display
  gc.power_set_brightness(90);

  // Display a header
  //   - gCore ID is a fixed 8-bit value that identifies the controller and its capabilities
  //   - gCore Revision is an 8-bit number.  The upper four bits indicate the major version
  //     and the lower four bits indicate the minor version.
  // We check the return code from the register access routines here as an example but assume
  // they are working correctly in the main code.
  tft.setTextColor(ILI9488_CYAN);
  tft.setTextSize(2);
  tft.setCursor(100, 10);
  tft.print("gCore Register Explorer");

  tft.setTextColor(ILI9488_DARKCYAN);
  tft.setTextSize(1);
  tft.setCursor(20, 50);
  if (!gc.gcore_get_reg8(GCORE_REG_ID, &reg8)) {
    tft.print("  Error reading ID...");
    Serial.println("Error reading ID register");
    while (1) {
      delay(100);
    }
  } else {
    tft.printf("ID: 0x%X", reg8);
  }

  if (!gc.gcore_get_reg8(GCORE_REG_VER, &reg8)) {
    tft.print("  Error reading VERSION...");
    Serial.println("Error reading VERSION register");
    while (1) {
      delay(100);
    }
  } else {
    tft.printf("  Version: %d.%d", reg8 >> 8, reg8 & 0x0F);
  }

  tft.setTextColor(ILI9488_DARKGREEN);
  tft.setCursor(20, 290);
  tft.println("(Serial command interface at 115200 baud.  Type H for help.)");

  // Initial register value update (and set prev_reg_val so that in the future we only
  // update changed values)
  tft.setTextColor(ILI9488_ORANGE);
  update_reg_vals(true);

  Serial.println("gCore register explorer starting.  Type H followed by Carriage Return or Send for help.");
  prev_msec = millis();
}

void loop() {
  char c;
  unsigned long cur_msec;

  // Process incoming serial data
  while (Serial.available()) {
    c = Serial.read();
    process_rx_data(c);
  }

  
  // Periodically read and update register values on the LCD
  cur_msec = millis();
  if (cur_msec > (prev_msec + 500)) {
    prev_msec = cur_msec;
    
    // Update displayed register values
    update_reg_vals(false);
  }

}



//
// Internal subroutines
//

// State-machine based character parser
void process_rx_data(char c)
{
  int v;
  
  switch (cmd_state) {
    case CMD_ST_IDLE:
      if (((c >= 'a') && (c <= 'z')) || ((c >= 'A') && (c <= 'Z'))) {
        cmd_op = c;
        cmd_has_val = false;
        cmd_arg = 0;
        cmd_state = CMD_ST_CMD;
      }
      break;
    
    case CMD_ST_CMD:
      if (c == TERM_CHAR) {
        process_command();
        cmd_state = CMD_ST_IDLE;
      } else {
        if ((v = is_valid_hex(c)) >= 0) {
          cmd_arg = (cmd_arg * 16) + v;
        } else if (c == '=') {
          cmd_state = CMD_ST_VAL;
          cmd_val = 0;
          cmd_has_val = true;
        } else if (c != ' ') {
          cmd_state = CMD_ST_IDLE;
          Serial.println("Illegal command");
        }
      }
      break;
    
    case CMD_ST_VAL:
      if (c == TERM_CHAR) {
        process_command();
        cmd_state = CMD_ST_IDLE;
      } else {
        if ((v = is_valid_hex(c)) >= 0) {
          cmd_val = (cmd_val * 16) + v;
        } else if (c != ' ') {
          cmd_state = CMD_ST_IDLE;
          Serial.println("Illegal command");
        }
      }
      break;

    default:
      cmd_state = CMD_ST_IDLE;
  }
}


void process_command()
{
  uint8_t reg8;
  uint16_t reg16;
  uint32_t reg32;
  int32_t val;
  
  switch (cmd_op) {
    case 'b':
    case 'B':
      if (cmd_has_val) {
        reg8 = cmd_val & 0xFF;
        if (gc.gcore_set_reg8((uint8_t) cmd_arg, reg8)) {
          Serial.printf("%c %X = %X\n", cmd_op, cmd_arg, reg8);
        } else {
          Serial.printf("Write %X failed\n", cmd_arg);
        }
      } else {
        if (gc.gcore_get_reg8((uint8_t) cmd_arg, &reg8)) {
          Serial.printf("%c %X = %X\n", cmd_op, cmd_arg, reg8);
        } else {
          Serial.printf("Read %X failed\n", cmd_arg);
        }
      }
      break;

    case 'w':
    case 'W':
      if (cmd_has_val) {
        reg16 = cmd_val & 0xFFFF;
        if (gc.gcore_set_reg16((uint8_t) cmd_arg, reg16)) {
          Serial.printf("%c %X = %X\n", cmd_op, cmd_arg, reg16);
        } else {
          Serial.printf("Write %X failed\n", cmd_arg);
        }
      } else {
        if (gc.gcore_get_reg16((uint8_t) cmd_arg, &reg16)) {
          Serial.printf("%c %X = %X\n", cmd_op, cmd_arg, reg16);
        } else {
          Serial.printf("Read %X failed\n", cmd_arg);
        }
      }
      break;

    case 'l':
    case 'L':
      if (cmd_has_val) {
        reg32 = (uint32_t) cmd_val;
        if (gc.gcore_set_reg32((uint8_t) cmd_arg, reg32)) {
          Serial.printf("%c %X = %X\n", cmd_op, cmd_arg, reg32);
        } else {
          Serial.printf("Write %X failed\n", cmd_arg);
        }
      } else {
        if (gc.gcore_get_reg32((uint8_t) cmd_arg, &reg32)) {
          Serial.printf("%c %X = %X\n", cmd_op, cmd_arg, reg32);
        } else {
          Serial.printf("Read %X failed\n", cmd_arg);
        }
      }
      break;

    case 'n':
    case 'N':
      if (cmd_has_val) {
        reg8 = cmd_val & 0xFF;
        if (gc.gcore_set_nvram_byte((uint16_t) cmd_arg, reg8)) {
          Serial.printf("%c %X = %X\n", cmd_op, cmd_arg, reg8);
        } else {
          Serial.printf("Write %X failed\n", cmd_arg);
        }
      } else {
        if (gc.gcore_get_nvram_byte((uint16_t) cmd_arg, &reg8)) {
          Serial.printf("%c %X = %X\n", cmd_op, cmd_arg, reg8);
        } else {
          Serial.printf("Read %X failed\n", cmd_arg);
        }
      }
      break;

    case 'h':
    case 'H':
      Serial.println(help_string);
      break;

    default:
      Serial.printf("Unknonw command op: %c\n", cmd_op);
  }
}


// Returns -1 for invalid hex character, 0 - 15 for valid hex
int is_valid_hex(char c) {
  if ((c >= '0') && (c <= '9')) {
    return (c - '0');
  } else if ((c >= 'a') && (c <= 'f')) {
    return (c - 'a' + 10);
  } else if ((c >= 'A') && (c <= 'F')) {
    return (c - 'A' + 10);
  }
  return -1;
}


void update_reg_vals(bool force_udpate)
{
  int i;
  const gCore_reg_disp_t* cr;
  uint8_t reg8;
  uint16_t reg16;
  uint32_t reg32;
  int32_t reg_val;

  for (i=0; i<NUM_DYN_REG; i++) {
    cr = &(gcr[i]);

    // Read the register into reg_val
    switch (cr->reg_size) {
      case 8:
        (void) gc.gcore_get_reg8(cr->reg_offset, &reg8);
        reg_val = (int32_t) reg8;
        break;
      case 16:
        (void) gc.gcore_get_reg16(cr->reg_offset, &reg16);
        reg_val = (int32_t) reg16;
        break;
      default:
        (void) gc.gcore_get_reg32(cr->reg_offset, &reg32);
        // So this is a hack.  gCore RTC keeps time as a uint32_t allowing a range from the start
        // of epoch time at midnight Jan 1 1970 to February 7, 2106 6:28:16 AM.  But for simplicities
        // sake in this program, since we want to print negative numbers too, we just convert it to
        // a int32 meaning the display here will rollover to negative numbers in 2038.
        reg_val = (int32_t) reg32;
    }

    // Update value if forced or changed
    if (force_udpate || (reg_val != prev_reg_val[i])) {
      // Save value for future comparisons
      prev_reg_val[i] = reg_val;
      
      // Blank the previous text
      tft.fillRect(cr->x, cr->y, cr->w, 16, ILI9488_BLACK);
      
      // Display the register name and value
      tft.setCursor(cr->x, cr->y);
      tft.print(cr->reg_name);
      tft.print(": ");
      if (cr->disp_hex == 1) {
        tft.printf("0x%X ", reg_val);
      } else {
        tft.printf("%d ", reg_val);
      }

      // Display any suffix
      tft.print(cr->reg_units);

      // Display custom additional information for certain registers
      switch(cr->reg_offset) {
        case GCORE_REG_STATUS:
          tft.print("  (");
          if (reg_val & GCORE_ST_CRIT_BATT_MASK) tft.print("Critical Battery, ");
          if (reg_val & GCORE_ST_PB_PRESS_MASK) tft.print("Button Pressed, ");
          switch (reg_val & GCORE_ST_PWR_ON_RSN_MASK) {
            case GCORE_PWR_ON_BTN_MASK: tft.print("Button power-on"); break;
            case GCORE_PWR_ON_ALARM_MASK: tft.print("Alarm power-on"); break;
            case GCORE_PWR_ON_CHG_MASK: tft.print("Charge power-on"); break;
          }
          tft.print(")");
          break;

        case GCORE_REG_GPIO:
          tft.print("  (");
          if (reg_val & GCORE_GPIO_SD_CARD_MASK) tft.print("SD Card, ");
          if (reg_val & GCORE_GPIO_PWR_BTN_MASK) tft.print("Button, ");
          switch (reg_val & GCORE_GPIO_CHG_MASK) {
            case GCORE_CHG_IDLE: tft.print("Not Charging"); break;
            case GCORE_CHG_ACTIVE: tft.print("Charging"); break;
            case GCORE_CHG_DONE: tft.print("Charge Done"); break;
            case GCORE_CHG_FAULT: tft.print("Charge Fault"); break;
          }
          tft.print(")");
          break;

        case GCORE_REG_WK_CTRL:
          if (reg_val != 0) {
            tft.print("  (Wake on ");
            if (reg_val & GCORE_WK_CHRG_DONE_MASK) tft.print("Charge Done, ");
            if (reg_val & GCORE_WK_CHRG_START_MASK) tft.print("Charge Start, ");
            if (reg_val & GCORE_WK_ALARM_MASK) tft.print("Alarm, ");
          } else {
            tft.print("  (Disabled");
          }
          tft.print(")");
          break;
      }
    }
  }
}
