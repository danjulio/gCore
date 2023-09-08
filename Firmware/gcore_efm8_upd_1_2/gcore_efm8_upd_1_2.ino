/*
 * gCore EFM8 firmware updater sketch.
 * 
 * This sketch has been designed to run on a 3.3V Arduino-compatible 32-bit development board.  It has been tested
 * on the following boards.
 * 
 *   1. PJRC Teensy 3.2 using Teensyduino 1.56
 *      - Important: Set Optimization Level to "Faster" (CPU default 96 MHz)
 *   2. PJRC Teensy 4 using Teensyduino 1.56
 *      - Important: Set Optimization Level to "Faster" (CPU default 600 MHz)
 *   3. Sparkfun ESP32 Thing Plus using Espressif ESP32 for Arduino version 2.0.9
 *      - Important: Flash frequency: 80 MHz
 *   4. Espressif ESP32-S3-DevKitC-1 using Espressif ESP32 for Arduino version 2.0.9
 *      - Important: Be sure Flash Mode matches your ESP32-S3 module (e.g. I used OPI 80 MHz for my MON32R8V module)
 *   5. Waveshare RP2040 Zero using Earle Philhower’s Raspberry Pi Pico/RP2040 version 3.4.1
 *      - Important: Set Optimization level to "Optimize Even More (-O3)" (CPU default 133 Mhz)
 *   
 * The C2 interface has some timing restrictions and various Arduino implementations have wildly different GPIO
 * performance so unfortunately I cannot guarantee operation on any other board (although different ESP32 and
 * RP2040 boards will probably work).
 * 
 * Updates the firmware on the gCore EFM8 PMIC/RTC co-processor to version 1.2 (fixing a bug in the RTC).
 * 
 * Use this sketch only if you are comfortable making the following connections to gCore.  There is the possibility
 * you'll damage the board (e.g. short something or rip a pad off the PCB).
 * 
 * Needs to be connected to the following 3 EFM8 programming pads on gCore (see my github repository
 * https://github.com/danjulio/gCore/tree/main/Firmware for more detailed instructions and pictures).
 * 
 *   Arduino GND    : gCore "G" pad
 *   Arduino c2dPin : gCore "D" pad
 *   Arduino c2ckPin: gCore "C" pad
 *   
 * c2dPin and c2ckPin are defined below.  You may either very carefully solder the three wires to gCore's pads or
 * [better] use male dupont pins held together and pressed against the pads (you may want to clean any oxide off the
 * pads using isopropyl alcohol first).
 * 
 * gCore is powered via a connected battery or USB-C.  It does not need to be turned on (the EFM8 will be powered
 * when gCore is powered).
 * 
 * Instructions
 *   1. Load this sketch into your 3.3V 32-bit Arduino board.
 *   2. The Arduino board should remain connected to the computer and the Serial Monitor opened and configured
 *      for 115200 baud.
 *   3. Type the character H into the Serial Monitor and hit send.  You should see a banner message indicating
 *      the sketch is running.
 *   4. With the Arduino connected to gCore and gCore powered, type P into the Serial Monitor and hit send.  You
 *      should see the programming sequence.  Programming will take a few seconds.  If you are pressing a set of
 *      dupont connectors into the gCore pads, make sure they remain in good contact.  You should see a message
 *      indicating success.  If you see a message indicating an error then probably you don't have a good or
 *      correct connection or gCore isn't powered.
 *   5. You can power everything down and disconnect the Arduino wires from gCore.
 *   6. With a battery attached to gCore press the power button.  It should turn on indicating the EFM8 is running.
 *      Load the gCore Arduino "gcore_reg_explorer" sketch to see the EFM8 version.  It should show "1.2".
 * 
 * Copyright 2023 danjuliodesigns, LLC.  All rights reserved.
 * 
 * This is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 *
 * It is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with this software.  If not, see
 * <https://www.gnu.org/licenses/>.
 *
 */

 
// ==================================================
// Pin Definition
//   Change these if necessary for your board.  Selected pins should be digital IO with nothing else attached
//   (e.g. Don't use the typical pin 13 with LED).
//

const int c2dPin  = 12;  // Connect to "D"
const int c2ckPin = 14;  // Connect to "C"



// ==================================================
// Arduino Entry Points
//
void setup() {
  Serial.begin(115200);

  // Time to open the Serial Monitor
  delay(2000);

  print_hello();
}


void loop() {
  char c;

  if (Serial.available()) {
    c = Serial.read();

    if ((c == 'H') || (c == 'h')) {
      print_hello();
    } else if ((c == 'P') || (c == 'p')) {
      program_device();
    }
  }

  delay(100);
}


// ==================================================
// Subroutines
//
void print_hello() {
  Serial.println("gCore EFM8 version 1.2 firmware updater sketch.");
}
