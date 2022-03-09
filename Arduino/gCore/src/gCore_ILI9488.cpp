/*
 * Adafruit GFX compatible LCD driver library for danjuliodesigns, LLC gCore
 * development board. Supports 16-bit colors natively.
 *
 * You must also include the Adafruit_GFX library that can be downloaded
 * from https://github/adafruit/Adafruit_GFX.  Thanks to Limor Fried/Ladyada 
 * at Adafruit Industries for all her hard work on these libraries.
 *
 * Written by Dan Julio
 *
 * Copyright 2021 danjuliodesigns, LLC
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this
 * software and associated documentation files (the "Software"), to deal in the Software
 * without restriction, including without limitation the rights to use, copy, modify,
 * merge, publish, distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies
 * or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
 * PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
 * OR OTHER DEALINGS IN THE SOFTWARE.
 *
 *
 * The original Adafruit ILI9488 library had the following header which is being
 * included below:
 *
 *  This is our library for the Adafruit  ILI9488 Breakout and Shield
 * ----> http://www.adafruit.com/products/1651
 *
 *  Check out the links above for our tutorials and wiring diagrams
 * These displays use SPI to communicate, 4 or 5 pins are required to
 * interface (RST is optional)
 * Adafruit invests time and resources providing this open source code,
 * please support Adafruit and open-source hardware by purchasing
 * products from Adafruit!
 *
 * Written by Limor Fried/Ladyada for Adafruit Industries.
 * MIT license, all text above must be included in any redistribution
 */
#include "gCore_ILI9488.h"


extern "C" {



/*
 * Initialization array
 */

typedef struct {
	uint8_t cmd;
	uint8_t data[16];
   	uint8_t databytes; // No of data in data; bit 7 = delay after set; 0xFF = end of cmds.
} lcd_init_cmd_t;

static lcd_init_cmd_t const ili_init_cmds[18] = {
	{ILI9488_CMD_SLEEP_OUT, {0x00}, 0x80},
	{ILI9488_CMD_POSITIVE_GAMMA_CORRECTION, {0x00, 0x03, 0x09, 0x08, 0x16, 0x0A, 0x3F, 0x78, 0x4C, 0x09, 0x0A, 0x08, 0x16, 0x1A, 0x0F}, 15},
	{ILI9488_CMD_NEGATIVE_GAMMA_CORRECTION, {0x00, 0x16, 0x19, 0x03, 0x0F, 0x05, 0x32, 0x45, 0x46, 0x04, 0x0E, 0x0D, 0x35, 0x37, 0x0F}, 15},
	{ILI9488_CMD_POWER_CONTROL_1, {0x17, 0x15}, 2},
	{ILI9488_CMD_POWER_CONTROL_2, {0x41}, 1},
	{ILI9488_CMD_VCOM_CONTROL_1, {0x00, 0x12, 0x80}, 3},
	{ILI9488_CMD_MEMORY_ACCESS_CONTROL, {(0x40 | 0x08)}, 1},
	{ILI9488_CMD_COLMOD_PIXEL_FORMAT_SET, {0x55}, 1}, // 16-bit pixels
	{ILI9488_CMD_INTERFACE_MODE_CONTROL, {0x00}, 1},
	{ILI9488_CMD_FRAME_RATE_CONTROL_NORMAL, {0xA0}, 1},
	{ILI9488_CMD_DISPLAY_INVERSION_CONTROL, {0x02}, 1},
	{ILI9488_CMD_DISPLAY_FUNCTION_CONTROL, {0x02, 0x02}, 2},
	{ILI9488_CMD_SET_IMAGE_FUNCTION, {0x00}, 1},
	{ILI9488_CMD_WRITE_CTRL_DISPLAY, {0x28}, 1},
	{ILI9488_CMD_WRITE_DISPLAY_BRIGHTNESS, {0x7F}, 1},
	{ILI9488_CMD_ADJUST_CONTROL_3, {0xA9, 0x51, 0x2C, 0x02}, 4},
	{ILI9488_CMD_DISPLAY_ON, {0x00}, 0x80},
	{0, {0}, 0xff},
};

}


/*
 * Class Implementation
 */
gCore_ILI9488::gCore_ILI9488() :
	Adafruit_GFX(ILI9488_TFTWIDTH, ILI9488_TFTHEIGHT),
	_dc(27),
	_rst(-1),
	_sclk(18),
	_mosi(23),
	_csn(5)
{
}


gCore_ILI9488::gCore_ILI9488(int8_t _CS, int8_t _DC, int8_t _MOSI, int8_t _SCLK, int8_t _RST) :
	Adafruit_GFX(ILI9488_TFTWIDTH, ILI9488_TFTHEIGHT),
	_dc(_DC),
	_rst(_RST),
	_sclk(_SCLK),
	_mosi(_MOSI),
	_csn(_CS)
{
}


void gCore_ILI9488::begin(void)
{
	esp_err_t ret;
	int i;
	uint16_t cmd = 0;
	
	// Initialize the SPI interface
	spi = new SPIClass(VSPI);
	
	// Configure the SPI interface
	spi->begin(_sclk, -1, _mosi, _csn);
	spi->setFrequency(ILI9488_SPI_MHZ);
	spi->setHwCs(true);
	spi->setDataMode(0);
	
	// Initialize DC Output and reset the display if reset is connected
	gpio_set_direction((gpio_num_t) _dc, GPIO_MODE_OUTPUT);
	if (_rst != -1) {
		gpio_set_direction((gpio_num_t) _rst, GPIO_MODE_OUTPUT);
		gpio_set_level((gpio_num_t) _rst, 0);
   		delay(100);
    	gpio_set_level((gpio_num_t) _rst, 1);
    	delay(100);
	}
	
	// Exit sleep
	sendLcdCommand(ILI9488_CMD_SOFTWARE_RESET);
	delay(100);
			
	// Initialize the display controller
	while (ili_init_cmds[cmd].databytes!=0xff) {
		sendLcdCommand(ili_init_cmds[cmd].cmd);
		sendLcdCommandData((uint8_t*) &(ili_init_cmds[cmd].data[0]), ili_init_cmds[cmd].databytes & 0x1F);
		if (ili_init_cmds[cmd].databytes & 0x80) {
			delay(100);
		}
		cmd++;
	}
	
	// Clear the display
	fillScreen(ILI9488_BLACK);
}


void gCore_ILI9488::setAddrWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
	/* Column addresses */
	xb[0] = (x0 >> 8) & 0xFF;
	xb[1] = x0 & 0xFF;
	xb[2] = (x1 >> 8) & 0xFF;
	xb[3] = x1 & 0xFF;
	
	/* Page addresses */
	yb[0] = (y0 >> 8) & 0xFF;
	yb[1] = y0 & 0xFF;
	yb[2] = (y1 >> 8) & 0xFF;
	yb[3] = y1 & 0xFF;

	/* Column addresses */
	sendLcdCommand(ILI9488_CMD_COLUMN_ADDRESS_SET);
	sendLcdCommandData(xb, 4);

	/* Page addresses */
	sendLcdCommand(ILI9488_CMD_PAGE_ADDRESS_SET);
	sendLcdCommandData(yb, 4);
	
	/* Setup subsequent memory write */
	sendLcdCommand(ILI9488_CMD_MEMORY_WRITE);
}


void gCore_ILI9488::drawImage(const uint8_t* img, uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
	// rudimentary clipping (drawChar w/big text requires this)
    if ((x >= _width) || (y >= _height)) return;
    if ((x + w - 1) >= _width)  w = _width  - x;
    if ((y + h - 1) >= _height) h = _height - y;

    setAddrWindow(x, y, x+w-1, y+h-1);
    
    sendLcdImageData((uint8_t*) img, w*h);
}


void gCore_ILI9488::fillScreen(uint16_t color)
{
	setAddrWindow(0, 0, _width-1, _height-1);
	
	sendLcdPixelsColor(color, ILI9488_TFTWIDTH * ILI9488_TFTHEIGHT);
}


void gCore_ILI9488::drawPixel(int16_t x, int16_t y, uint16_t color)
{
	if ((x < 0) ||(x >= _width) || (y < 0) || (y >= _height)) return;
	
	/* Column addresses */
	xb[0] = (x >> 8) & 0xFF;
	xb[1] = x & 0xFF;
	xb[2] = (x >> 8) & 0xFF;
	xb[3] = x & 0xFF;
	
	/* Page addresses */
	yb[0] = (y >> 8) & 0xFF;
	yb[1] = y & 0xFF;
	yb[2] = (y >> 8) & 0xFF;
	yb[3] = y & 0xFF;

	/* Column addresses */
	gpio_set_level((gpio_num_t) _dc, 0);
	spi->write(ILI9488_CMD_COLUMN_ADDRESS_SET);
	gpio_set_level((gpio_num_t) _dc, 1);
	spi->writeBytes(xb, 4);

	/* Page addresses */
	gpio_set_level((gpio_num_t) _dc, 0);
	spi->write(ILI9488_CMD_PAGE_ADDRESS_SET);
	gpio_set_level((gpio_num_t) _dc, 1);
	spi->writeBytes(yb, 4);
	
	/* Setup subsequent memory write */
	gpio_set_level((gpio_num_t) _dc, 0);
	spi->write(ILI9488_CMD_MEMORY_WRITE);
	
	gpio_set_level((gpio_num_t) _dc, 1);
	spi->write16(color);
}


void gCore_ILI9488::drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color)
{
	// Rudimentary clipping
	if ((x >= _width) || (y >= _height)) return;

	if((y+h-1) >= _height)
		h = _height-y;
    
    setAddrWindow(x, y, x, y+h-1);
    
    sendLcdPixelsColor(color, h);
}


void gCore_ILI9488::drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color)
{
	// Rudimentary clipping
	if ((x >= _width) || (y >= _height)) return;
	if ((x+w-1) >= _width)
		w = _width-x;
	
	setAddrWindow(x, y, x+w-1, y);
	
    sendLcdPixelsColor(color, w);
}


void gCore_ILI9488::fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color)
{
	// rudimentary clipping (drawChar w/big text requires this)
	if ((x >= _width) || (y >= _height)) return;
	if ((x + w - 1) >= _width)  w = _width  - x;
	if ((y + h - 1) >= _height) h = _height - y;
	
	setAddrWindow(x, y, x+w-1, y+h-1);
	
    sendLcdPixelsColor(color, w*h);
}


void gCore_ILI9488::writePixel(int16_t x, int16_t y, uint16_t color)
{
	if ((x < 0) ||(x >= _width) || (y < 0) || (y >= _height)) return;
	
	/* Column addresses */
	xb[0] = (x >> 8) & 0xFF;
	xb[1] = x & 0xFF;
	xb[2] = (x >> 8) & 0xFF;
	xb[3] = x & 0xFF;
	
	/* Page addresses */
	yb[0] = (y >> 8) & 0xFF;
	yb[1] = y & 0xFF;
	yb[2] = (y >> 8) & 0xFF;
	yb[3] = y & 0xFF;

	/* Column addresses */
	gpio_set_level((gpio_num_t) _dc, 0);
	spi->write(ILI9488_CMD_COLUMN_ADDRESS_SET);
	gpio_set_level((gpio_num_t) _dc, 1);
	spi->writeBytes(xb, 4);

	/* Page addresses */
	gpio_set_level((gpio_num_t) _dc, 0);
	spi->write(ILI9488_CMD_PAGE_ADDRESS_SET);
	gpio_set_level((gpio_num_t) _dc, 1);
	spi->writeBytes(yb, 4);
	
	/* Setup subsequent memory write */
	gpio_set_level((gpio_num_t) _dc, 0);
	spi->write(ILI9488_CMD_MEMORY_WRITE);
	
	gpio_set_level((gpio_num_t) _dc, 1);
	spi->write16(color);
}


void gCore_ILI9488::pushPixels(const void* data, int32_t len)
{
	sendLcdImageData((uint8_t*) data, len);
}


void gCore_ILI9488::setRotation(uint8_t r)
{
	uint8_t d;
	
	sendLcdCommand(ILI9488_CMD_MEMORY_ACCESS_CONTROL);
	
	rotation = r % 4; // can't be higher than 3
	
	switch (rotation) {
		case 0:
			d = (ILI9488_MADCTL_MX | ILI9488_MADCTL_BGR);
			_width  = ILI9488_TFTWIDTH;
			_height = ILI9488_TFTHEIGHT;
			break;
		case 1:
			d = (ILI9488_MADCTL_MV | ILI9488_MADCTL_BGR);
			_width  = ILI9488_TFTHEIGHT;
			_height = ILI9488_TFTWIDTH;
			break;
		case 2:
			d = (ILI9488_MADCTL_MY | ILI9488_MADCTL_BGR);
			 _width  = ILI9488_TFTWIDTH;
			 _height = ILI9488_TFTHEIGHT;
			break;
		case 3:
			d = (ILI9488_MADCTL_MX | ILI9488_MADCTL_MY | ILI9488_MADCTL_MV | ILI9488_MADCTL_BGR);
			_width  = ILI9488_TFTHEIGHT;
			_height = ILI9488_TFTWIDTH;
			break;
	}
	sendLcdCommandData(&d, 1);
}


void gCore_ILI9488::invertDisplay(boolean i)
{
	sendLcdCommand(i ? ILI9488_CMD_DISP_INVERSION_ON : ILI9488_CMD_DISP_INVERSION_OFF);
}


uint16_t gCore_ILI9488::color565(uint8_t r, uint8_t g, uint8_t b)
{
	return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}


void gCore_ILI9488::sendLcdCommand(uint8_t cmd)
{
	gpio_set_level((gpio_num_t) _dc, 0);
	spi->write(cmd);
}


// sendLcdCommandData assumes all data will fit in one transaction
void gCore_ILI9488::sendLcdCommandData(uint8_t* data, uint8_t len)
{
	gpio_set_level((gpio_num_t) _dc, 1);
	spi->writeBytes(data, len);
}


void gCore_ILI9488::sendLcdPixelColor(uint16_t color)
{
	gpio_set_level((gpio_num_t) _dc, 1);
	spi->write16(color);
}


void gCore_ILI9488::sendLcdPixelsColor(uint16_t color, int32_t len)
{
	uint8_t ca[2];
	
	ca[0] = color >> 8;
	ca[1] = color & 0xFF;
	
	gpio_set_level((gpio_num_t) _dc, 1);
	spi->writePattern(ca, 2, len);
}


void gCore_ILI9488::sendLcdImageData(uint8_t* data, int32_t len)
{
	gpio_set_level((gpio_num_t) _dc, 1);
	spi->writeBytes(data, len);
}
