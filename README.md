## gCore - gadget Core

![gCore Top](Pictures/gcore_rev3.png)
 
This repository contains documentation and example code for an ESP32-based development board I designed to enable building portable GUI-based gadgets (*).  It provides a rich feature set and high performance operation enabling, I hope, all kinds of cool and useful devices such as remote controls, instrument controllers and game platforms.  Code may be written using the ESP32 Arduino environment or with Espressif's IDF.  Both Adafruit-compatible and [LVGL](https://lvgl.io) LCD drivers are available.

![gCore based tCam](Pictures/tcam_iron.png)
(gCore based tCam thermal imaging camera)

![gCore Controlling LIFX bulbs](Pictures/gcore_lifx_remote.png)
(gCore controlling LIFX LED light bulbs via Wifi)

### Supporting documentation and code
This repository contains documentation, an Arduino library with example code, and designs for 3D printed enclosures.

![gCore Features and Benefits](Pictures/gcore_feat_benefits.png)

### Support in other Github repositories
Additional demos and ports of other software to gCore are stored in the following additional repositories to reduce the size of this repository.

1. [TFT_eSPI](https://github.com/danjulio/TFT_eSPI) - A fork of Bodmer's LCD driver library ported to gCore to maximize LCD drawing performance for Arduino programs.  Very fast.
2. [tCam](https://github.com/danjulio/tCam) - Assembly instructions and firmware for the tCam thermal imaging camera.
3. [gcore_life](https://github.com/danjulio/gcore_life) - Life running on gCore using LVGL.  Shows how to integrate control for gCore into an ESP32 IDF program.
4. [gcore\_lv\_port_esp32](https://github.com/danjulio/gcore_lv_port_esp32) - The LVGL v7 demo with gCore optimized LCD and touch drivers useful for for ESP32 IDF programs.
5. [gcore_tank](https://github.com/danjulio/gcore_tank) - An easy-to-build remote controlled vehicle with video feed using gCore and the remote control.

![Life!](Pictures/life.png)

![Remote Controlled Tank](Pictures/tank_gcore.png)

### (*) Historical Note
This is actually my second design.  The original gCore design was based around a "feather" PCB designed to fit under an Adafruit LCD module.  It is documented in the [Original gCore](https://github.com/danjulio/gCore-featherwing) repository.
