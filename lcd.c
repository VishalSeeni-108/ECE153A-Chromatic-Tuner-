/*
 * lcd.c
 *
 *  Created on: Oct 21, 2015
 *      Author: atlantis
 */

/*
 UTFT.cpp - Multi-Platform library support for Color TFT LCD Boards
 Copyright (C)2015 Rinky-Dink Electronics, Henning Karlsen. All right reserved

 This library is the continuation of my ITDB02_Graph, ITDB02_Graph16
 and RGB_GLCD libraries for Arduino and chipKit. As the number of
 supported display modules and controllers started to increase I felt
 it was time to make a single, universal library as it will be much
 easier to maintain in the future.

 Basic functionality of this library was origianlly based on the
 demo-code provided by ITead studio (for the ITDB02 modules) and
 NKC Electronics (for the RGB GLCD module/shield).

 This library supports a number of 8bit, 16bit and serial graphic
 displays, and will work with both Arduino, chipKit boards and select
 TI LaunchPads. For a full list of tested display modules and controllers,
 see the document UTFT_Supported_display_modules_&_controllers.pdf.

 When using 8bit and 16bit display modules there are some
 requirements you must adhere to. These requirements can be found
 in the document UTFT_Requirements.pdf.
 There are no special requirements when using serial displays.

 You can find the latest version of the library at
 http://www.RinkyDinkElectronics.com/

 This library is free software; you can redistribute it and/or
 modify it under the terms of the CC BY-NC-SA 3.0 license.
 Please see the included documents for further information.

 Commercial use of this library requires you to buy a license that
 will allow commercial use. This includes using the library,
 modified or not, as a tool to sell products.

 The license applies to all part of the library including the
 examples and tools supplied with the library.
 */

#include "lcd.h"

#define ILI9341_RAMWR   (0x2C) 	/* RAM write command */

// Global variables
int fch;
int fcl;
int bch;
int bcl;
const struct _current_font *cfont;

/* added by Ryan for fancy font support, Fall '24 */
static uint16_t textcolor; // set by setColor(), matches fch & fcl
static int cursor_x = 0;
static int cursor_y = 0;

static int _height = DISP_Y_SIZE;
static int _width = DISP_X_SIZE;
static _Bool wrap = 1;

// Write command to LCD controller
void LCD_Write_COM(char VL) {
	Xil_Out32(SPI_DC, 0x0);
	Xil_Out32(SPI_DTR, VL);

	while (0 == (Xil_In32(SPI_IISR) & XSP_INTR_TX_EMPTY_MASK))
		;
	Xil_Out32(SPI_IISR, Xil_In32(SPI_IISR) | XSP_INTR_TX_EMPTY_MASK);
}

// Write 8-bit data to LCD controller
void LCD_Write_DATA(char VL) {
	Xil_Out32(SPI_DC, 0x01);
	Xil_Out32(SPI_DTR, VL);

	while (0 == (Xil_In32(SPI_IISR) & XSP_INTR_TX_EMPTY_MASK))
		;
	Xil_Out32(SPI_IISR, Xil_In32(SPI_IISR) | XSP_INTR_TX_EMPTY_MASK);
}

// Write 16-bit data to LCD controller
void LCD_Write_DATA16(uint16_t V) {
	LCD_Write_DATA(V >> 8U);
	LCD_Write_DATA(V);
}

// Initialize LCD controller
void initLCD(void) {
	int i;

	// Reset
	LCD_Write_COM(0x01);
	for (i = 0; i < 500000; i++)
		; //Must wait > 5ms

	LCD_Write_COM(0xCB);
	LCD_Write_DATA(0x39);
	LCD_Write_DATA(0x2C);
	LCD_Write_DATA(0x00);
	LCD_Write_DATA(0x34);
	LCD_Write_DATA(0x02);

	LCD_Write_COM(0xCF);
	LCD_Write_DATA(0x00);
	LCD_Write_DATA(0XC1);
	LCD_Write_DATA(0X30);

	LCD_Write_COM(0xE8);
	LCD_Write_DATA(0x85);
	LCD_Write_DATA(0x00);
	LCD_Write_DATA(0x78);

	LCD_Write_COM(0xEA);
	LCD_Write_DATA(0x00);
	LCD_Write_DATA(0x00);

	LCD_Write_COM(0xED);
	LCD_Write_DATA(0x64);
	LCD_Write_DATA(0x03);
	LCD_Write_DATA(0X12);
	LCD_Write_DATA(0X81);

	LCD_Write_COM(0xF7);
	LCD_Write_DATA(0x20);

	LCD_Write_COM(0xC0);   //Power control
	LCD_Write_DATA(0x23);  //VRH[5:0]

	LCD_Write_COM(0xC1);   //Power control
	LCD_Write_DATA(0x10);  //SAP[2:0];BT[3:0]

	LCD_Write_COM(0xC5);   //VCM control
	LCD_Write_DATA(0x3e);  //Contrast
	LCD_Write_DATA(0x28);

	LCD_Write_COM(0xC7);   //VCM control2
	LCD_Write_DATA(0x86);  //--

	LCD_Write_COM(0x36);   // Memory Access Control
	LCD_Write_DATA(0x48);

	LCD_Write_COM(0x3A);
	LCD_Write_DATA(0x55);

	LCD_Write_COM(0xB1);
	LCD_Write_DATA(0x00);
	LCD_Write_DATA(0x18);

	LCD_Write_COM(0xB6);   // Display Function Control
	LCD_Write_DATA(0x08);
	LCD_Write_DATA(0x82);
	LCD_Write_DATA(0x27);

	LCD_Write_COM(0x11);   //Exit Sleep
	for (i = 0; i < 100000; i++)
		;

	LCD_Write_COM(0x29);   //Display on
	LCD_Write_COM(0x2c);

	//for (i = 0; i < 100000; i++);

	// Default color
	setColor(255, 255, 255);
	setColorBg(0, 0, 0);
}

// Set boundary for drawing
void setXY(int x1, int y1, int x2, int y2) {
	LCD_Write_COM(0x2A);
	LCD_Write_DATA16(x1);
	LCD_Write_DATA16(x2);
	LCD_Write_COM(0x2B);
	LCD_Write_DATA16(y1);
	LCD_Write_DATA16(y2);
	LCD_Write_COM(0x2C);
}

// Remove boundary
void clrXY(void) {
	setXY(0, 0, DISP_X_SIZE, DISP_Y_SIZE);
}

// Set foreground RGB color for next drawing
void setColor(u8 r, u8 g, u8 b) {
	// 5-bit r, 6-bit g, 5-bit b
	fch = (r & 0x0F8) | g >> 5;
	fcl = (g & 0x1C) << 3 | b >> 3;
	textcolor = (fch << 8U) | (fcl & 8U);
}

// Set background RGB color for next drawing
void setColorBg(u8 r, u8 g, u8 b) {
	// 5-bit r, 6-bit g, 5-bit b
	bch = (r & 0x0F8) | g >> 5;
	bcl = (g & 0x1C) << 3 | b >> 3;
}

// Clear display
void clrScr(void) {
	// Black screen
	setColor(0, 0, 0);

	fillRect(0, 0, DISP_X_SIZE, DISP_Y_SIZE);
}

// Draw horizontal line
void drawHLine(int x, int y, int l) {
	int i;

	if (l < 0) {
		l = -l;
		x -= l;
	}

	setXY(x, y, x + l, y);
	for (i = 0; i < l + 1; i++) {
		LCD_Write_DATA(fch);
		LCD_Write_DATA(fcl);
	}

	clrXY();
}

// Fill a rectangular 
void fillRect(int x1, int y1, int x2, int y2) {
	int i;

	if (x1 > x2)
		swap(int, x1, x2);

	if (y1 > y2)
		swap(int, y1, y2);

	setXY(x1, y1, x2, y2);
	for (i = 0; i < (x2 - x1 + 1) * (y2 - y1 + 1); i++) {
		LCD_Write_DATA(fch);
		LCD_Write_DATA(fcl);
	}

	clrXY();
}

void setFont(const ILI9341_t3_font_t *f)
{
	cfont = f;
}


/* Font subsystem ported from Paul Stoffregen's driver for ILI9341
 *
 * Download fonts from https://github.com/PaulStoffregen/ILI9341_fonts
 */

static inline uint32_t fetchbit(const uint8_t *p, uint32_t index)
{
	return (p[index >> 3] & (0x80 >> (index & 7)));
}

static uint32_t fetchbits_unsigned(const uint8_t *p, uint32_t index, uint32_t required)
{
	uint32_t val;
	uint8_t *s = (uint8_t *)&p[index>>3];

	val = s[0] << 24;
	val |= (s[1] << 16);
	val |= (s[2] << 8);
	val |= s[3];
	val <<= (index & 7); // shift out used bits
	if (32 - (index & 7) < required) { // need to get more bits
		val |= (s[4] >> (8 - (index & 7)));
	}
	val >>= (32-required); // right align the bits
	return val;
}

static uint32_t fetchbits_signed(const uint8_t *p, uint32_t index, uint32_t required)
{
	uint32_t val = fetchbits_unsigned(p, index, required);
	if (val & (1 << (required - 1))) {
		return (int32_t)val - (1 << required);
	}
	return (int32_t)val;
}

static void drawFontBits(uint32_t bits, uint32_t numbits, uint32_t x, uint32_t y, uint32_t repeat)
{
	if (bits == 0) return;
	uint32_t w;
	bits <<= (32-numbits); // left align bits
	do {
		w = __builtin_clz(bits); // skip over zeros
		if (w > numbits) w = numbits;
		numbits -= w;
		x += w;
		bits <<= w;
		bits = ~bits; // invert to count 1s as 0s
		w = __builtin_clz(bits);
		if (w > numbits) w = numbits;
		numbits -= w;
		bits <<= w;
		bits = ~bits; // invert back to original polarity
		if (w > 0) {
			x += w;
			setXY(x-w, y, x-1, y+repeat-1); // write a block of pixels w x repeat sized
			LCD_Write_COM(ILI9341_RAMWR); // write to RAM
			w *= repeat;
			while (w-- > 1) { // draw line
				LCD_Write_DATA16(textcolor);
			}
			LCD_Write_DATA16(textcolor);
		}
	} while (numbits > 0);
}

void drawFontChar(unsigned int c)
{
	uint32_t bitoffset;
	const uint8_t *data;

//	xil_printf("drawFontChar %c\n\r", c);

	if (c >= cfont->index1_first && c <= cfont->index1_last) {
		bitoffset = c - cfont->index1_first;
		bitoffset *= cfont->bits_index;
	} else if (c >= cfont->index2_first && c <= cfont->index2_last) {
		bitoffset = c - cfont->index2_first + cfont->index1_last - cfont->index1_first + 1;
		bitoffset *= cfont->bits_index;
	} else if (cfont->unicode) {
		return; // TODO: implement sparse unicode
	} else {
		return;
	}
	data = cfont->data + fetchbits_unsigned(cfont->index, bitoffset, cfont->bits_index);

	uint32_t encoding = fetchbits_unsigned(data, 0, 3);
	if (encoding != 0) return;
	uint32_t width = fetchbits_unsigned(data, 3, cfont->bits_width);
	bitoffset = cfont->bits_width + 3;
	uint32_t height = fetchbits_unsigned(data, bitoffset, cfont->bits_height);
	bitoffset += cfont->bits_height;

	int32_t xoffset = fetchbits_signed(data, bitoffset, cfont->bits_xoffset);
	bitoffset += cfont->bits_xoffset;
	int32_t yoffset = fetchbits_signed(data, bitoffset, cfont->bits_yoffset);
	bitoffset += cfont->bits_yoffset;

	uint32_t delta = fetchbits_unsigned(data, bitoffset, cfont->bits_delta);
	bitoffset += cfont->bits_delta;

	// horizontally, we draw every pixel, or none at all
	if (cursor_x < 0) cursor_x = 0;
	int32_t origin_x = cursor_x + xoffset;
	if (origin_x < 0) {
		cursor_x -= xoffset;
		origin_x = 0;
	}
	if (origin_x + (int)width > _width) {
		if (!wrap) return;
		origin_x = 0;
		if (xoffset >= 0) {
			cursor_x = 0;
		} else {
			cursor_x = -xoffset;
		}
		cursor_y += cfont->line_space;
	}
	if (cursor_y >= _height) return;
	cursor_x += delta;

	// vertically, the top and/or bottom can be clipped
	int32_t origin_y = cursor_y + cfont->cap_height - height - yoffset;

	// TODO: compute top skip and number of lines
	int32_t linecount = height;
	//uint32_t loopcount = 0;
	uint32_t y = origin_y;
	while (linecount) {
		uint32_t b = fetchbit(data, bitoffset++);
		if (b == 0) {
			uint32_t x = 0;
			do {
				uint32_t xsize = width - x;
				if (xsize > 32) xsize = 32;
				uint32_t bits = fetchbits_unsigned(data, bitoffset, xsize);
				drawFontBits(bits, xsize, origin_x + x, y, 1);
				bitoffset += xsize;
				x += xsize;
			} while (x < width);
			y++;
			linecount--;
		} else {
			uint32_t n = fetchbits_unsigned(data, bitoffset, 3) + 2;
			bitoffset += 3;
			uint32_t x = 0;
			do {
				uint32_t xsize = width - x;
				if (xsize > 32) xsize = 32;
				uint32_t bits = fetchbits_unsigned(data, bitoffset, xsize);
				drawFontBits(bits, xsize, origin_x + x, y, n);
				bitoffset += xsize;
				x += xsize;
			} while (x < width);
			y += n;
			linecount -= n;
		}
	}
}

static size_t write(u8 c)
{
	// make sure you have called setFont before using this function!

	if (c == '\n') {
		cursor_y += cfont->line_space; // Fix linefeed.
		cursor_x = 0;
	} else {
		drawFontChar(c);
	}
	return 1;
}

void printChar(u8 c, int x, int y)
{
	cursor_x = x;
	cursor_y = y;
	write(c);
}

void lcdPrint(char *str, int x, int y)
{
	cursor_x = x;
	cursor_y = y;

	while (*str) {
		if (!write(*str++))
			return;
	}
}


// measure the height & width of a specific character
static void measureChar(u8 c, u16 *w, u16 *h)
{
	// Treat non-breaking space as normal space
	if (c == 0xa0) {
		c = ' ';
	}

	// ILI9341_T3 font

	*h = cfont->cap_height;
	*w = 0;

	uint32_t bitoffset;
	const uint8_t *data;

	if (c >= cfont->index1_first && c <= cfont->index1_last) {
		bitoffset = c - cfont->index1_first;
		bitoffset *= cfont->bits_index;
	}
	else if (c >= cfont->index2_first && c <= cfont->index2_last) {
		bitoffset = c - cfont->index2_first + cfont->index1_last - cfont->index1_first + 1;
		bitoffset *= cfont->bits_index;
	}
	else if (cfont->unicode) {
		return; // TODO: implement sparse unicode
	}
	else {
		return;
	}

	data = cfont->data + fetchbits_unsigned(cfont->index, bitoffset, cfont->bits_index);

	uint32_t encoding = fetchbits_unsigned(data, 0, 3);

	if (encoding != 0) return;

	//uint32_t width =
	fetchbits_unsigned(data, 3, cfont->bits_width);
	bitoffset = cfont->bits_width + 3;

	//uint32_t height =
	fetchbits_unsigned(data, bitoffset, cfont->bits_height);
	bitoffset += cfont->bits_height;

	//int32_t xoffset =
	fetchbits_signed(data, bitoffset, cfont->bits_xoffset);
	bitoffset += cfont->bits_xoffset;

	//int32_t yoffset =
	fetchbits_signed(data, bitoffset, cfont->bits_yoffset);
	bitoffset += cfont->bits_yoffset;

	uint32_t delta = fetchbits_unsigned(data, bitoffset, cfont->bits_delta);
	*w = delta;
}


/// get width of a given text string
uint16_t measureTextWidth(const char* text) {
	uint16_t maxH = 0;
	uint16_t currH = 0;

	uint16_t n = strlen(text);

	for (int i = 0; i < n; i++) {
		if (text[i] == '\n') {
			// For multi-line strings, retain max width
			if (currH > maxH)
				maxH = currH;
			currH = 0;
		}
		else {
			uint16_t h, w;
			measureChar(text[i], &w, &h);
			currH += w;
		}
	}
	uint16_t h = maxH > currH ? maxH : currH;
	return h;
}


/// get height of a given text string
uint16_t measureTextHeight(const char* text) {
	int lines = 1;
	uint16_t n = strlen(text);

	for (int i = 0; i < n; i++) {
		if (text[i] == '\n') {
			lines++;
		}
	}
	return ((lines - 1) * cfont->line_space + cfont->cap_height);
}

void drawBackground()
{
	clrScr();

	//Background
	setColor(0, 0, 0);
	fillRect(0, 0, 240, 320);

//	//Triangles
//	for(int x = 0; x < 240; x += 40)
//	{
//		for(int y = 0; y < 320; y += 40)
//		{
//			fortyTri(x, y);
//		}
//	}
}

