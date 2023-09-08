/*
 * Silicon Labs C2 programming interface low-level routines.  Based on their
 * AN127SW package.
 */



//-----------------------------------------------------------------------------
// IO Defines
//-----------------------------------------------------------------------------
#define C2D_DRIVER_ON() pinMode(c2dPin, OUTPUT)
#define C2D_DRIVER_OFF() pinMode(c2dPin, INPUT)
#define C2CK_DRIVER_ON() pinMode(c2ckPin, OUTPUT)
#define C2CK_DRIVER_OFF() pinMode(c2ckPin, INPUT)
#define SET_C2D_HIGH() digitalWrite(c2dPin, HIGH)
#define SET_C2D_LOW() digitalWrite(c2dPin, LOW)
#define SET_C2CK_HIGH() digitalWrite(c2ckPin, HIGH)
#define SET_C2CK_LOW() digitalWrite(c2ckPin, LOW)

#define C2CK digitalRead(c2ckPin)
#define C2D digitalRead(c2dPin)

#if defined(ARDUINO_ARCH_RP2040)
#define STROBE_C2CK() digitalWrite(c2ckPin, LOW); \
                      busy_wait_us(1); \
                      digitalWrite(c2ckPin, HIGH); \
                      busy_wait_us(1);
#else
#define STROBE_C2CK() digitalWrite(c2ckPin, LOW); \
                      delayMicroseconds(1); \
                      digitalWrite(c2ckPin, HIGH); \
                      delayMicroseconds(1);
#endif

//-----------------------------------------------------------------------------
// Exported constants
//-----------------------------------------------------------------------------

#define C2_AR_OUTREADY 0x01
#define C2_AR_INBUSY   0x02
#define C2_AR_OTPBUSY  0x80
#define C2_AR_OTPERROR 0x40
#define C2_AR_FLBUSY   0x80

#define C2_DEVICEID    0x00
#define C2_REVID       0x01
#define C2_FPCTL       0x02

#define C2_FPCTL_RUNNING    0x00
#define C2_FPCTL_HALT       0x01
#define C2_FPCTL_RESET      0x02
#define C2_FPCTL_CORE_RESET 0x04

#define C2_FPDAT                0xB4
#define C2_FPDAT_GET_VERSION    0x01
#define C2_FPDAT_GET_DERIVATIVE 0x02
#define C2_FPDAT_DEVICE_ERASE   0x03
#define C2_FPDAT_BLOCK_READ     0x06
#define C2_FPDAT_BLOCK_WRITE    0x07
#define C2_FPDAT_PAGE_ERASE     0x08
#define C2_FPDAT_DIRECT_READ    0x09
#define C2_FPDAT_DIRECT_WRITE   0x0a
#define C2_FPDAT_INDIRECT_READ  0x0b
#define C2_FPDAT_INDIRECT_WRITE 0x0c

#define C2_FPDAT_RETURN_INVALID_COMMAND 0x00
#define C2_FPDAT_RETURN_COMMAND_FAILED  0x02
#define C2_FPDAT_RETURN_COMMAND_OK      0x0D

#define C2_DEVCTL             0x02
#define C2_EPCTL              0xDF
#define C2_EPDAT              0xBF
#define C2_EPADDRH            0xAF
#define C2_EPADDRL            0xAE
#define C2_EPSTAT             0xB7
#define C2_EPSTAT_WRITE_LOCK  0x80
#define C2_EPSTAT_READ_LOCK   0x40
#define C2_EPSTAT_CAL_VALID   0x20
#define C2_EPSTAT_CAL_DONE    0x10
#define C2_EPSTAT_ERROR       0x01

#define C2_EPCTL_READ         0x00
#define C2_EPCTL_WRITE1       0x40
#define C2_EPCTL_WRITE2       0x58
#define C2_EPCTL_FAST_WRITE   0x78

#define C2_DIRECT   0
#define C2_INDIRECT 1

// C2 DR timeouts (us)
#define C2_WRITE_DR_TIMEOUT_US 10000
#define C2_READ_DR_TIMEOUT_US 10000

// C2 Debug timeouts (ms)
#define C2_POLL_INBUSY_TIMEOUT_MS 20
#define C2_POLL_OUTREADY_TIMEOUT_MS 20
#define C2_POLL_OTPBUSY_TIMEOUT_MS 20

// C2 Status masks
#define C2_INBUSY   0x02
#define C2_OUTREADY 0x01

// Flash timeouts
#define FLASH_READ_TIMEOUT_MS 20
#define FLASH_WRITE_TIMEOUT_MS 20
#define FLASH_PAGE_ERASE_TIMEOUT_MS 40
#define FLASH_DEVICE_ERASE_TIMEOUT_MS 10000

//
// Function return values - error codes
//
enum
{
   NO_ERROR = 0,                       // 0
   INVALID_COMMAND,                    // 1
   COMMAND_FAILED,                     // 2
   INVALID_PARAMS,                     // 3
   C2DR_WRITE_TIMEOUT,                 // 4
   C2DR_READ_TIMEOUT,                  // 5
   C2_POLL_INBUSY_TIMEOUT,             // 6
   C2_POLL_OUTREADY_TIMEOUT,           // 7
   C2_POLL_OTPBUSY_TIMEOUT,            // 8
   DEVICE_READ_PROTECTED,              // 9
   DEVICE_NOT_BLANK,                   // 10
   NOT_A_HEX_RECORD,                   // 11
   UNSUPPORTED_HEX_RECORD,             // 12
   COMMAND_OK,                         // 13 (0x0d)
   BAD_CHECKSUM,                       // 14
   FAMILY_NOT_SUPPORTED,               // 15
   BAD_DEBUG_COMMAND,                  // 16
   DERIVATIVE_NOT_SUPPORTED,           // 17
   READ_ERROR,                         // 18
   OTP_READ_TIMEOUT,                   // 19
   OTP_WRITE_TIMEOUT,                  // 20
   WRITE_ERROR,                        // 21
   SFR_WRITE_TIMEOUT,                  // 22
   SFR_READ_TIMEOUT,                   // 23
   ADDRESS_OUT_OF_RANGE,               // 24
   PAGE_ERASE_TIMEOUT,                 // 25
   DEVICE_ERASE_TIMEOUT,               // 26
   DEVICE_ERASE_FAILURE,               // 27
   DEVICE_IS_BLANK,                    // 28
   IMAGE_OUT_OF_RANGE,                 // 29
   EOF_HEX_RECORD,                     // 30
   VERIFY_FAILURE,                     // 31
   IMAGE_NOT_FORMATTED,                // 32
   JTAG_POLLBUSY_TIMEOUT,              // 33
   JTAG_IREAD_TIMEOUT,                 // 34
   JTAG_IWRITE_TIMEOUT,                // 35
   JTAG_WRITE_COMMAND_TIMEOUT,         // 36
   JTAG_READ_COMMAND_TIMEOUT           // 37
};


//
// Programmer variables
//
uint8_t C2_AR;
uint8_t C2_DR;
uint16_t c2SubError;



//-----------------------------------------------------------------------------
// API
//-----------------------------------------------------------------------------
void program_device() {
  int prog_len;
  uint8_t* prog_data;
  uint8_t t8;
  uint16_t t16;

  C2CK_DRIVER_ON();
  SET_C2CK_HIGH();

#if defined(ARDUINO_ARCH_RP2040)
  // For some reason the first time this subroutine is executed on the RP2040 the GPIO
  // operations for the clock are really slow violating the C2 spec and confusing the EFM8.
  // If we run this twice it seems to work (I won't share, dear reader, how many hours of
  // f*cking around it took me to settle on this ugly solution which I don't think even
  // works all the time).
  (void) C2_Halt();
#endif

  Serial.println("Halt Device");
  delay(250);
  t8 = C2_Halt();
  if (t8 != NO_ERROR) {
    Serial.printf("Error: C2_Halt returned %d - 0x%x\n", t8, C2_GetSubError());
    goto prog_end;
  }

  Serial.println("Verify Device Accessible");
  delay(250);
  t8 = C2_GetDevID(&t16);
  if (t8 != NO_ERROR) {
    Serial.printf("C2_GetDevID returned %d - 0x%x\n", t8, C2_GetSubError());
    goto prog_end;
  } else {
    if (t16 != 0x16) {
      Serial.printf("Error: Device ID = 0x%2x, Expected 0x16\n", t16);
      goto prog_end;
    }
  }

  Serial.println("Erase device");
  delay(250);
  t8 = C2_FLASH_DeviceErase();
  if (t8 != NO_ERROR) {
    Serial.printf("Error: C2_FLASH_DeviceErase returned %d - 0x%x\n", t8, C2_GetSubError());
    goto prog_end;
  }

  Serial.println("Init device for programming");
  delay(250);
  t8 = C2_init_sfr_efm8sb2();
  if (t8 != NO_ERROR) {
    Serial.printf("Error: C2_init_sfr_efm8bb2 returned %d - 0x%x\n", t8, C2_GetSubError());
    goto prog_end;
  }

  Serial.println("Program device");
  delay(250);
  prog_data = get_hex_array_addr(&prog_len);
  t8 = C2_FLASH_Write(0, prog_data, prog_len);
  if (t8 != NO_ERROR) {
    Serial.printf("Error: C2_FLASH_Write returned %d - 0x%x\n", t8, C2_GetSubError());
    goto prog_end;
  }

  (void) C2_Reset();

  Serial.println("EMF8 programmed successfully.  You may power down and disconnect.");
prog_end:
  C2CK_DRIVER_OFF();
}



//-----------------------------------------------------------------------------
// C2_GetSubError
//-----------------------------------------------------------------------------
//
// Return Value : c2SubError
// Parameters   : None
//
//-----------------------------------------------------------------------------
uint16_t C2_GetSubError (void)
{
  return c2SubError;
}


//-----------------------------------------------------------------------------
// C2_Reset
//-----------------------------------------------------------------------------
//
// Return Value : None
// Parameters   : None
//
// Performs a target device reset by pulling the C2CK pin low for >20us.
//
//-----------------------------------------------------------------------------
uint8_t C2_Reset (void)
{
   SET_C2CK_LOW();                      // Put target device in reset state
   delayMicroseconds (25);             // by driving C2CK low for >20us

   SET_C2CK_HIGH();                     // Release target device from reset
   delayMicroseconds (5);              // wait at least 2us before Start

   return NO_ERROR;
}

//-----------------------------------------------------------------------------
// C2_WriteAR
//-----------------------------------------------------------------------------
//
// Return Value : Error code
// Parameters   : value for address
//
// Performs a C2 Address register write (writes the <addr> input to Address
// register).
//
//-----------------------------------------------------------------------------
uint8_t C2_WriteAR (uint8_t addr)
{
   uint8_t i;

   noInterrupts();

   // START field
   STROBE_C2CK ();                     // Strobe C2CK with C2D driver disabled

   // INS field (11b, LSB first)
   C2D_DRIVER_ON ();                   // Enable C2D driver (output)
   SET_C2D_HIGH();
   STROBE_C2CK ();
   SET_C2D_HIGH();
   STROBE_C2CK ();

   // send 8-bit address
   for (i = 0; i < 8; i++)             // Shift the 8-bit addr field
   {
      if (addr & 0x01)                 // present LSB of addr at C2D pin
      {
         SET_C2D_HIGH();
      }
      else
      {
         SET_C2D_LOW();
      }

      STROBE_C2CK ();

      addr = addr >> 1;
   }

   // STOP field
   C2D_DRIVER_OFF ();                  // Disable C2D driver
   STROBE_C2CK ();                     // Strobe C2CK with C2D driver disabled

   interrupts();

   return NO_ERROR;
}

//-----------------------------------------------------------------------------
// C2_ReadAR
//-----------------------------------------------------------------------------
//
// Return Value : Error code, C2_AR global variable
// Parameters   : None
//
// Performs a C2 Address register read.
// Returns the 8-bit register content in global variable 'C2_AR'.  Return
// value is an error code.
//
//-----------------------------------------------------------------------------
uint8_t C2_ReadAR (void)
{
   uint8_t i;                               // bit counter
   uint8_t addr;                            // value read from C2 interface

   noInterrupts();

   // START field
   STROBE_C2CK ();                     // Strobe C2CK with C2D driver disabled

   // INS field (10b, LSB first)
   C2D_DRIVER_ON ();                   // Enable C2D driver (output)
   SET_C2D_LOW();
   STROBE_C2CK ();
   SET_C2D_HIGH();
   STROBE_C2CK ();

   C2D_DRIVER_OFF ();                  // Disable C2D driver (input)

   // read 8-bit ADDRESS field
   addr = 0;
   for (i = 0; i < 8; i++)             // Shift in 8 bit ADDRESS field
   {                                   // LSB-first
      addr >>= 1;
      STROBE_C2CK ();
      if (C2D == HIGH)
      {
         addr |= 0x80;
      }
   }
   C2_AR = addr;                       // update global variable with result

   // STOP field
   STROBE_C2CK ();                     // Strobe C2CK with C2D driver disabled

   interrupts();

   return NO_ERROR;
}

//-----------------------------------------------------------------------------
// C2_WriteDR
//-----------------------------------------------------------------------------
//
// Return Value : Error code
// Parameters   : data to write to C2
//
// Performs a C2 Data register write (writes <dat> input to data register).
//
//-----------------------------------------------------------------------------
uint8_t C2_WriteDR (uint8_t dat, unsigned long timeout_us)
{
   uint8_t i;                          // bit counter
   unsigned long start_us;
   uint8_t return_value;

   return_value = NO_ERROR;

   noInterrupts();

   // START field
   STROBE_C2CK ();                     // Strobe C2CK with C2D driver disabled

   // INS field (01b, LSB first)
   C2D_DRIVER_ON ();                   // Enable C2D driver
   SET_C2D_HIGH();
   STROBE_C2CK ();
   SET_C2D_LOW();
   STROBE_C2CK ();

   // LENGTH field (00b -> 1 byte)
   SET_C2D_LOW();
   STROBE_C2CK ();
   SET_C2D_LOW();
   STROBE_C2CK ();

   // write 8-bit DATA field
   for (i = 0; i < 8; i++)             // Shift out 8-bit DATA field
   {                                   // LSB-first
      if (dat & 0x01)
      {
         SET_C2D_HIGH();
      }
      else
      {
         SET_C2D_LOW();
      }

      STROBE_C2CK ();
      dat >>= 1;
   }

   // WAIT field
   C2D_DRIVER_OFF ();                  // Disable C2D driver for input

   STROBE_C2CK ();                     // clock first wait field

   start_us = micros();
   
   // sample Wait until it returns High or until timeout expires
   while (C2D == LOW) {
    if (AbsDiff32u(start_us, micros()) < timeout_us) {
      STROBE_C2CK ();
    } else {
      return_value = C2DR_WRITE_TIMEOUT;
      break;
    }
   }

   // STOP field
   STROBE_C2CK ();                     // Strobe C2CK with C2D driver disabled

   interrupts();

   return return_value;
}

//-----------------------------------------------------------------------------
// C2_ReadDR
//-----------------------------------------------------------------------------
//
// Return Value : Error code
// Parameters   : None
//
// Performs a C2 Data register read.
// Returns the 8-bit register content in global C2_DR.
//
//-----------------------------------------------------------------------------
uint8_t C2_ReadDR (unsigned long timeout_us)
{
   uint8_t i;                               // bit counter
   uint8_t dat;                             // data holding register
   uint8_t return_value;
   unsigned long start_us;

   noInterrupts();

   return_value = NO_ERROR;

   // START field
   STROBE_C2CK ();                     // Strobe C2CK with C2D driver disabled

   // INS field (00b, LSB first)
   C2D_DRIVER_ON ();                   // Enable C2D driver (output)
   SET_C2D_LOW();
   STROBE_C2CK ();
   SET_C2D_LOW();
   STROBE_C2CK ();

   // LENGTH field (00b -> 1 byte)
   SET_C2D_LOW();
   STROBE_C2CK ();
   SET_C2D_LOW();
   STROBE_C2CK ();

   // WAIT field
   C2D_DRIVER_OFF ();                  // Disable C2D driver for input
   
   // stobe C2CK until Wait returns '1' or until timeout expires
   start_us = micros();
   STROBE_C2CK ();                     // sample first Wait cycle
   while (C2D == LOW) {
    if (AbsDiff32u(start_us, micros()) < timeout_us) {
      STROBE_C2CK ();
    } else {
      return_value = C2DR_READ_TIMEOUT;
      break;
    }
   }

   // 8-bit DATA field
   dat = 0;
   for (i = 0; i < 8; i++)             // Shift in 8-bit DATA field
   {                                   // LSB-first
      dat >>= 1;
      STROBE_C2CK ();
      if (C2D == HIGH)
      {
         dat  |= 0x80;
      }
   }
   C2_DR = dat;                        // update global C2 DAT value

   // STOP field
   STROBE_C2CK ();                     // Strobe C2CK with C2D driver disabled

   interrupts();
   
   return return_value;
}


//-----------------------------------------------------------------------------
// C2_Poll_InBusy
//-----------------------------------------------------------------------------
//
// Return Value : None
// Parameters   : None
//
// This function polls the INBUSY flag of C2_AR.
//
//-----------------------------------------------------------------------------
uint8_t C2_Poll_InBusy (unsigned long timeout_ms)
{
   uint8_t return_value;
   unsigned long start_ms;

   start_ms = millis();

   return_value = NO_ERROR;

   while (1)
   {
      C2_ReadAR ();
      if ((C2_AR & C2_AR_INBUSY) == 0)
      {
         break;                        // exit on NO_ERROR and not busy
      }
      if (AbsDiff32u(start_ms, millis()) >= timeout_ms)
      {
         // exit on POLL_INBUSY timeout
         return_value = C2_POLL_INBUSY_TIMEOUT;
         break;
      }
   }

   return return_value;
}

//-----------------------------------------------------------------------------
// C2_Poll_OutReady
//-----------------------------------------------------------------------------
//
// Return Value : None
// Parameters   : None
//
// This function polls the OUTREADY flag of C2_AR.
//
//-----------------------------------------------------------------------------
uint8_t C2_Poll_OutReady (unsigned long timeout_ms)
{
   uint8_t return_value;
   unsigned long start_ms;

   start_ms = millis();

   return_value = NO_ERROR;

   while (1)
   {
      C2_ReadAR ();

      if (C2_AR & C2_AR_OUTREADY)
      {
         break;                        // exit on NO_ERROR and data ready
      }
      if (AbsDiff32u(start_ms, millis()) >= timeout_ms)
      {
         // exit on POLL_INBUSY timeout
         return_value = C2_POLL_OUTREADY_TIMEOUT;
         break;
      }
   }

   return return_value;
}


//-----------------------------------------------------------------------------
// C2_Halt
//-----------------------------------------------------------------------------
//
// Return Value : None
// Parameters   : None
//
// This routine issues a pin reset of the device, followed by a device
// reset, core reset, then Halt request.
//
//-----------------------------------------------------------------------------
uint8_t C2_Halt (void)
{
   uint8_t return_value = NO_ERROR;

   C2_Reset ();                        // issue pin reset

   // issue standard reset
   C2_WriteAR (C2_FPCTL);

   if ((return_value = C2_WriteDR (C2_FPCTL_RESET, C2_WRITE_DR_TIMEOUT_US)) != 0)
   {
      c2SubError = 1;
      return return_value;
   }

   // issue core reset
   if ((return_value = C2_WriteDR (C2_FPCTL_CORE_RESET, C2_WRITE_DR_TIMEOUT_US)) != 0)
   {
    c2SubError = 2;
      return return_value;
   }

   // issue HALT
   if ((return_value = C2_WriteDR (C2_FPCTL_HALT, C2_WRITE_DR_TIMEOUT_US)) != 0)
   {
      c2SubError = 3;
      return return_value;
   }

   delay (25);                       // wait at least 20 ms

   return return_value;
}

//-----------------------------------------------------------------------------
// C2_GetDevID
//-----------------------------------------------------------------------------
//
// Return Value : None
// Parameters   : None
//
// This routine returns the device family ID by reference to <devid>.
//
//-----------------------------------------------------------------------------
uint8_t C2_GetDevID (uint16_t *devid)
{
   uint8_t return_value = NO_ERROR;
   uint8_t temp;

   if ((return_value = C2_ReadSFR (C2_DEVICEID, &temp)) != 0)
   {
      c2SubError = 1;
      return return_value;
   }

   *devid = temp;

   return return_value;
}

//-----------------------------------------------------------------------------
// C2_GetRevID
//-----------------------------------------------------------------------------
//
// Return Value : None
// Parameters   : None
//
// This routine returns the device revision ID by reference to <revid>.
//
//-----------------------------------------------------------------------------
uint8_t C2_GetRevID (uint16_t *revid)
{
   uint8_t return_value;
   uint8_t temp;

   if ((return_value = C2_ReadSFR (C2_REVID, &temp)) != 0)
   {
      c2SubError = 1;
      return return_value;
   }

   *revid = temp;

   return return_value;
}

//-----------------------------------------------------------------------------
// C2_ReadSFR
//-----------------------------------------------------------------------------
//
// Return Value : Error code
// Parameters   : sfraddress contains address to read, sfrdata points to
//                a result holding register
//
// This routine reads the contents of the SFR at address <sfraddress> and
// returns its value by reference into <sfrdata>.
//
//-----------------------------------------------------------------------------
uint8_t C2_ReadSFR (uint8_t sfraddress, uint8_t *sfrdata)
{
   uint8_t return_value;

   C2_WriteAR (sfraddress);

   if ((return_value = C2_ReadDR (C2_READ_DR_TIMEOUT_US)) != 0)
   {
      c2SubError = 1;
      return SFR_READ_TIMEOUT;
   }

   *sfrdata = C2_DR;

   return return_value;
}

//-----------------------------------------------------------------------------
// C2_WriteSFR
//-----------------------------------------------------------------------------
//
// Return Value : Error code
// Parameters   : sfraddress contains address to write, sfrdata contains data
//                to write
//
// This routine writes the contents of the SFR at address <sfraddress> to
// the value of <sfrdata>.
//
//-----------------------------------------------------------------------------
uint8_t C2_WriteSFR (uint8_t sfraddress, uint8_t sfrdata)
{
   uint8_t return_value;

   C2_WriteAR (sfraddress);

   if ((return_value = C2_WriteDR (sfrdata, C2_WRITE_DR_TIMEOUT_US)) != 0)
   {
      c2SubError = 1;
      return SFR_WRITE_TIMEOUT;
   }

   return return_value;
}

//-----------------------------------------------------------------------------
// C2_WriteCommand
//-----------------------------------------------------------------------------
//
// Return Value : Error code
// Parameters   : command
//
// This routine writes a C2 Flash command (WriteDR + INBUSY polling).
//
//-----------------------------------------------------------------------------
uint8_t C2_WriteCommand (uint8_t command, unsigned long C2_poll_inbusy_timeout_ms)
{
   uint8_t return_value;

   // Send command
   if ((return_value = C2_WriteDR (command, C2_WRITE_DR_TIMEOUT_US)) != 0)
   {
      c2SubError = 1;
      return return_value;
   }

   // verify acceptance
   if ((return_value = C2_Poll_InBusy (C2_poll_inbusy_timeout_ms)) != 0)
   {
      c2SubError = 2;
      return return_value;
   }

   return return_value;
}

//-----------------------------------------------------------------------------
// C2_ReadResponse
//-----------------------------------------------------------------------------
//
// Return Value : Error code
// Parameters   : None
//
// This routine reads a C2 Flash command response (Outready polling + ReadDR).
//
//-----------------------------------------------------------------------------
uint8_t C2_ReadResponse (unsigned long C2_poll_outready_timeout_ms)
{
   uint8_t return_value;

   // check for response
   if ((return_value = C2_Poll_OutReady (C2_poll_outready_timeout_ms)) != 0)
   {
      c2SubError = 1;
      return return_value;
   }

   // read status
   if ((return_value = C2_ReadDR (C2_READ_DR_TIMEOUT_US)) != 0)
   {
      c2SubError = 2;
      return return_value;
   }

   if (C2_DR != COMMAND_OK)
   {
      if (C2_DR == C2_FPDAT_RETURN_INVALID_COMMAND)
      {
         c2SubError = 3;
         return BAD_DEBUG_COMMAND;
      }
      else
      {
         c2SubError = 4;
         return C2_DR;
      }
   }

   return NO_ERROR;
}

//-----------------------------------------------------------------------------
// C2_ReadData
//-----------------------------------------------------------------------------
//
// Return Value : Error code
// Parameters   : None
//
// This routine reads a C2 Flash command data byte (Outready polling + ReadDR).
//
//-----------------------------------------------------------------------------
uint8_t C2_ReadData (unsigned long C2_poll_outready_timeout_ms)
{
   uint8_t return_value;

   // check for response
   if ((return_value = C2_Poll_OutReady (C2_poll_outready_timeout_ms)) != 0)
   {
      c2SubError = 1;
      return return_value;
   }

   // read status
   if ((return_value = C2_ReadDR (C2_READ_DR_TIMEOUT_US)) != 0)
   {
      c2SubError = 2;
      return return_value;
   }

   return NO_ERROR;
}

//-----------------------------------------------------------------------------
// C2_ReadDirect
//-----------------------------------------------------------------------------
//
// Return Value : Error code
// Parameters   : sfraddress contains address to read, sfrdata points to
//                a result holding register
//
// This routine reads the contents of the SFR at address <sfraddress> and
// returns its value by reference into <sfrdata>.
// If <address> is 0x00 to 0x7f, this function accesses RAM.
// Requires that FPDAT is known and the device is Halted.
// If <indirect> is "C2_INDIRECT", <address> targets RAM for all addresses.
//
//-----------------------------------------------------------------------------
uint8_t C2_ReadDirect (uint8_t sfraddress, uint8_t *sfrdata, uint8_t indirect)
{
   uint8_t return_value;

   C2_WriteAR (C2_FPDAT);

   // set up command accesses
   if (indirect == C2_DIRECT)
   {
      if ((return_value = C2_WriteCommand (C2_FPDAT_DIRECT_READ, C2_POLL_INBUSY_TIMEOUT_MS)) != 0)
      {
         c2SubError |= 0x10;
         return return_value;
      }
   }
   else
   {
      if ((return_value = C2_WriteCommand (C2_FPDAT_INDIRECT_READ, C2_POLL_INBUSY_TIMEOUT_MS)) != 0)
      {
         c2SubError |= 0x20;
         return return_value;
      }
   }


   // Check command status
   if ((return_value = C2_ReadResponse (C2_POLL_OUTREADY_TIMEOUT_MS)) != 0)
   {
      c2SubError |= 0x30;
      return return_value;
   }

   // now send address of byte to read
   if ((return_value = C2_WriteCommand (sfraddress, C2_POLL_INBUSY_TIMEOUT_MS)) != 0)
   {
      c2SubError |= 0x40;
      return return_value;
   }

   // Send number of bytes to read
   if ((return_value = C2_WriteCommand (1, C2_POLL_INBUSY_TIMEOUT_MS)) != 0)
   {
      c2SubError |= 0x50;
      return return_value;
   }

   // read the data
   if ((return_value = C2_ReadData (C2_POLL_OUTREADY_TIMEOUT_MS)) != 0)
   {
      c2SubError |= 0x60;
      return return_value;
   }

   *sfrdata = C2_DR;

   return return_value;
}

//-----------------------------------------------------------------------------
// C2_WriteDirect
//-----------------------------------------------------------------------------
//
// Return Value : Error code
// Parameters   : sfraddress contains address to write, sfrdata contains data
//                to write
//
// This routine writes <sfrdata> to address <sfraddress>.
// If <address> is 0x00 to 0x7f, this function accesses RAM.
// Requires that FPDAT is known and the device is Halted.
// If <indirect> is "C2_INDIRECT", <address> targets RAM for all addresses.
//
//-----------------------------------------------------------------------------
uint8_t C2_WriteDirect (uint8_t sfraddress, uint8_t sfrdata, uint8_t indirect)
{
   uint8_t return_value;

   // set up command accesses
   C2_WriteAR (C2_FPDAT);

   if (indirect == C2_DIRECT)
   {
      if ((return_value = C2_WriteCommand (C2_FPDAT_DIRECT_WRITE, C2_POLL_INBUSY_TIMEOUT_MS)) != 0)
      {
         c2SubError |= 0x10;
         return return_value;
      }
   }
   else
   {
      if ((return_value = C2_WriteCommand (C2_FPDAT_INDIRECT_WRITE, C2_POLL_INBUSY_TIMEOUT_MS)) != 0)
      {
         c2SubError |= 0x20;
         return return_value;
      }
   }

   // Check response
   if ((return_value = C2_ReadResponse (C2_POLL_OUTREADY_TIMEOUT_MS)) != 0)
   {
      c2SubError |= 0x30;
      return return_value;
   }

   // now send address of byte to write
   if ((return_value = C2_WriteCommand (sfraddress, C2_POLL_INBUSY_TIMEOUT_MS)) != 0)
   {
      c2SubError |= 0x40;
      return return_value;
   }

   // Send number of bytes to write
   if ((return_value = C2_WriteCommand (1, C2_POLL_INBUSY_TIMEOUT_MS)) != 0)
   {
      c2SubError |= 0x50;
      return return_value;
   }

   // Send the data
   if ((return_value = C2_WriteCommand (sfrdata, C2_POLL_INBUSY_TIMEOUT_MS)) != 0)
   {
      c2SubError |= 0x60;
      return return_value;
   }

   return return_value;
}


//-----------------------------------------------------------------------------
// C2_FLASH_Read
//-----------------------------------------------------------------------------
//
// Return Value : Error code
// Parameters   : None
//
// Reads a block of FLASH memory of <length> starting at <addr> and
// copies it to the buffer pointed to by <dest>.
// Assumes that FPDAT has been determined and the device is in the HALT
// state.
//
//
//-----------------------------------------------------------------------------
uint8_t C2_FLASH_Read (uint8_t *dest, uint16_t addr, uint16_t length)
{
   uint8_t return_value;

   uint16_t i;                              // byte counter
   uint16_t blocksize;                      // size of this block transfer

   return_value = NO_ERROR;

   // Set up command writes
   C2_WriteAR (C2_FPDAT);

   while (length != 0x0000)
   {

      // Send Block Read command
      if ((return_value = C2_WriteCommand (C2_FPDAT_BLOCK_READ, C2_POLL_INBUSY_TIMEOUT_MS)) != 0)
      {
         c2SubError |= 0x10;
         return return_value;
      }

      // check status
      if ((return_value = C2_ReadResponse (C2_POLL_OUTREADY_TIMEOUT_MS)) != 0)
      {
         c2SubError |= 0x20;
         return return_value;
      }

      // now send address, high-byte first
      if ((return_value = C2_WriteCommand (addr>>8, C2_POLL_INBUSY_TIMEOUT_MS)) != 0)
      {
         c2SubError |= 0x30;
         return return_value;
      }

      // Send address low byte to FPDAT
      if ((return_value = C2_WriteCommand (addr&0xFF, C2_POLL_INBUSY_TIMEOUT_MS)) != 0)
      {
         c2SubError |= 0x40;
         return return_value;
      }

      // now set up transfer for either a 256-byte block or less
      if (length > 255)
      {
         // indicate 256-byte block
         if ((return_value = C2_WriteCommand (0, C2_POLL_INBUSY_TIMEOUT_MS)) != 0)
         {
            c2SubError |= 0x60;
            return return_value;
         }
         length = length - 256;        // update length
         addr = addr + 256;            // update FLASH address
         blocksize = 256;
      }
      else
      {
         if ((return_value = C2_WriteCommand (length, C2_POLL_INBUSY_TIMEOUT_MS)) != 0)
         {
            c2SubError |= 0x70;
            return return_value;
         }
         blocksize = length;
         length = 0;
      }

      // Check status
      if ((return_value = C2_ReadResponse (C2_POLL_OUTREADY_TIMEOUT_MS)) != 0)
      {
         c2SubError |= 0x80;
         return return_value;
      }

      // Read FLASH block
      for (i = 0; i < blocksize; i++)
      {
         if ((return_value = C2_ReadData (FLASH_READ_TIMEOUT_MS)) != 0)
         {
            c2SubError |= 0x90;
            return return_value;
         }

         *dest++ = C2_DR;
      }
   }

   return return_value;
}


//-----------------------------------------------------------------------------
// C2_FLASH_Write
//-----------------------------------------------------------------------------
//
// Return Value : Error code
// Parameters   : None
//
// Copies a buffer pointed to by <src> of <length> bytes to FLASH address
// starting at <addr>.
// Assumes that FPDAT has been determined and the device is in the HALT
// state.
//
//-----------------------------------------------------------------------------
uint8_t C2_FLASH_Write (uint16_t addr, uint8_t *src, uint16_t length)
{
   uint8_t return_value;

   uint16_t i;                              // byte counter
   uint16_t blocksize;                      // size of this block transfer
   
   return_value = NO_ERROR;

   // Set up for command writes
   C2_WriteAR (C2_FPDAT);

   while (length != 0x0000)
   {
      // Send Block Write command
      if ((return_value = C2_WriteCommand (C2_FPDAT_BLOCK_WRITE, C2_POLL_INBUSY_TIMEOUT_MS)) != 0)
      {
         c2SubError |= 0x10;
         return return_value;
      }

      // Check response
      if ((return_value = C2_ReadResponse (C2_POLL_OUTREADY_TIMEOUT_MS)) != 0)
      {
         c2SubError |= 0x20;
         return return_value;
      }

      // now send address, high-byte first
      if ((return_value = C2_WriteCommand (addr>>8, C2_POLL_INBUSY_TIMEOUT_MS)) != 0)
      {
         c2SubError |= 0x30;
         return return_value;
      }

      // Send address low byte to FPDAT
      if ((return_value = C2_WriteCommand (addr&0xFF, C2_POLL_INBUSY_TIMEOUT_MS)) != 0)
      {
         c2SubError |= 0x40;
         return return_value;
      }

      // now set up transfer for either a 256-byte block or less
      if (length > 255)
      {
         // indicate 256-byte block
         if ((return_value = C2_WriteCommand (0, C2_POLL_INBUSY_TIMEOUT_MS)) != 0)
         {
            c2SubError |= 0x50;
            return return_value;
         }
         length = length - 256;        // update length
         addr = addr + 256;            // update FLASH address
         blocksize = 256;
      }
      else
      {
         if ((return_value = C2_WriteCommand (length, C2_POLL_INBUSY_TIMEOUT_MS)) != 0)
         {
            c2SubError |= 0x60;
            return return_value;
         }
         blocksize = length;
         length = 0;
      }

      // Check status before writing FLASH block
      if ((return_value = C2_ReadResponse (C2_POLL_OUTREADY_TIMEOUT_MS)) != 0)
      {
         return ADDRESS_OUT_OF_RANGE;
      }

      // Write FLASH block
      for (i = 0; i < blocksize; i++)
      {
         if ((return_value = C2_WriteCommand (*src++, FLASH_WRITE_TIMEOUT_MS)) != 0)
         {
            c2SubError |= 0x70;
Serial.printf("i = %d\n", i);
            return return_value;
         }
      }

      // Check status
      if ((return_value = C2_ReadResponse (C2_POLL_OUTREADY_TIMEOUT_MS)) != 0)
      {
         c2SubError |= 0x80;
         return return_value;
      }
   }

   return return_value;
}


//-----------------------------------------------------------------------------
// C2_FLASH_DeviceErase
//-----------------------------------------------------------------------------
//
// Return Value : Error code
// Parameters   : None
//
// Erases the entire FLASH memory space.
// Assumes that FPDAT has been determined and the device is in the HALT
// state.
//
//-----------------------------------------------------------------------------
uint8_t C2_FLASH_DeviceErase (void)
{
   uint8_t return_value;

   // Set up for commands
   C2_WriteAR (C2_FPDAT);

   // Send Page Erase command
   if ((return_value = C2_WriteCommand (C2_FPDAT_DEVICE_ERASE, C2_POLL_INBUSY_TIMEOUT_MS)) != 0)
   {
      c2SubError |= 0x10;
      return return_value;
   }

   // check status
   if ((return_value = C2_ReadResponse (C2_POLL_OUTREADY_TIMEOUT_MS)) != 0)
   {
      c2SubError |= 0x20;
      return return_value;
   }

   // now send sequence #1
   if ((return_value = C2_WriteCommand (0xDE, C2_POLL_INBUSY_TIMEOUT_MS)) != 0)
   {
      c2SubError |= 0x30;
      return return_value;
   }

   // now send sequence #2
   if ((return_value = C2_WriteCommand (0xAD, C2_POLL_INBUSY_TIMEOUT_MS)) != 0)
   {
      c2SubError |= 0x40;
      return return_value;
   }

   // now send sequence #3
   if ((return_value = C2_WriteCommand (0xA5, C2_POLL_INBUSY_TIMEOUT_MS)) != 0)
   {
      c2SubError |= 0x50;
      return return_value;
   }

   // check status
   if ((return_value = C2_ReadResponse (FLASH_DEVICE_ERASE_TIMEOUT_MS)) != 0)
   {
      c2SubError |= 0x60;
      return return_value;
   }

   return return_value;
}

//-----------------------------------------------------------------------------
// C2_FLASH_BlankCheck
//-----------------------------------------------------------------------------
//
// Return Value : Error code
// Parameters   : None
//
// Reads up to the entire FLASH memory, confirming that each byte is 0xFF.
// Returns NO_ERROR if the device is blank, DEVICE_NOT_BLANK or
// DEVICE_READ_PROTECTED otherwise.
// Assumes that the device is in the HALT state.
//
//-----------------------------------------------------------------------------
uint8_t C2_FLASH_BlankCheck (uint16_t addr, uint16_t length)
{
   uint8_t return_value;
   uint16_t i;                              // byte counter
   uint32_t blocksize;                      // size of this block transfer
   bool device_is_blank;

   device_is_blank = true;

   return_value = NO_ERROR;

   // Set up for command writes
   C2_WriteAR (C2_FPDAT);

   while (length != 0x0000L)
   {

      // Send Block Read command
      if ((return_value = C2_WriteCommand (C2_FPDAT_BLOCK_READ, C2_POLL_INBUSY_TIMEOUT_MS)) != 0)
      {
         c2SubError |= 0x10;
         return return_value;
      }

      // check status
      if ((return_value = C2_ReadResponse (C2_POLL_OUTREADY_TIMEOUT_MS)) != 0)
      {
         c2SubError |= 0x20;
         return return_value;
      }

      // now send address, high-byte first
      if ((return_value = C2_WriteCommand (addr>>8, C2_POLL_INBUSY_TIMEOUT_MS)) != 0)
      {
         c2SubError |= 0x30;
         return return_value;
      }

      // Send address low byte to FPDAT
      if ((return_value = C2_WriteCommand (addr&0xFF, C2_POLL_INBUSY_TIMEOUT_MS)) != 0)
      {
         c2SubError |= 0x40;
         return return_value;
      }

      // now set up transfer for either a 256-byte block or less
      if (length > 255)
      {
         // indicate 256-byte block
         if ((return_value = C2_WriteCommand (0, C2_POLL_INBUSY_TIMEOUT_MS)) != 0)
         {
            c2SubError |= 0x50;
            return return_value;
         }
         length = length - 256;        // update length
         addr = addr + 256;            // update FLASH address
         blocksize = 256;
      }
      else
      {
         if ((return_value = C2_WriteCommand (length, C2_POLL_INBUSY_TIMEOUT_MS)) != 0)
         {
            c2SubError |= 0x60;
            return return_value;
         }
         blocksize = length;
         length = 0;
      }

      // Check status before reading FLASH block
      if ((return_value = C2_ReadResponse (C2_POLL_OUTREADY_TIMEOUT_MS)) != 0)
      {
         c2SubError |= 0x70;
         return ADDRESS_OUT_OF_RANGE;
      }

      // Read FLASH block
      for (i = 0; i < blocksize; i++)
      {
         if ((return_value = C2_ReadData (FLASH_READ_TIMEOUT_MS)) != 0)
         {
            c2SubError |= 0x80;
            return return_value;
         }

         if (C2_DR != 0xFF)
         {
            device_is_blank = false;
         }
      }
   }

   if (device_is_blank == false)
   {
      c2SubError |= 0x90;
      return_value = DEVICE_NOT_BLANK;
   }
   else
   {
      return_value = DEVICE_IS_BLANK;
   }

   return return_value;
}


//-----------------------------------------------------------------------------
// C2_init_sfr_efm8sb2
//-----------------------------------------------------------------------------
//
// Return Value : Error code
// Parameters   : None
//
// Configure EFM8SB2-specific SFRs for programming.
// Assumes that FPDAT has been determined and the device is in the HALT
// state.
//
//-----------------------------------------------------------------------------
uint8_t C2_init_sfr_efm8sb2() {
  uint8_t return_value;
  
  return_value = NO_ERROR;

  // Setup EFM8SB2 SFRs (from AN127 Table 3.6)
  if ((return_value = C2_WriteDirect(0xA7, 0x00, C2_DIRECT)) != 0)  // Oscillator Initialization
  {
    c2SubError |= 0x100;
    return return_value;
  }
  delay(1);
  if ((return_value = C2_WriteDirect(0xB2, 0x8F, C2_DIRECT)) != 0)
  {
    c2SubError |= 0x200;
    return return_value;
  }
  delay(1);
  if ((return_value = C2_WriteDirect(0xA9, 0x00, C2_DIRECT)) != 0)
  {
    c2SubError |= 0x400;
    return return_value;
  }
  
  return return_value;
}


//-----------------------------------------------------------------------------
// Utilities
//-----------------------------------------------------------------------------
unsigned long AbsDiff32u(unsigned long n1, unsigned long n2) {
  if (n2 >= n1) {
    return (n2-n1);
  } else {
    return (n2-n1+((unsigned long) -1));
  }
}
