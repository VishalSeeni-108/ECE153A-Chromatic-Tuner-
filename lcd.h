/*
 * lcd.h
 *
 *  Created on: Oct 21, 2015
 *      Author: atlantis
 */

/*
  UTFT.h - Multi-Platform library support for Color TFT LCD Boards
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

#ifndef LCD_H_
#define LCD_H_

#include "xparameters.h"
#include "xil_io.h"
#include "xil_types.h"
#include "xspi_l.h"
#include "xil_printf.h"

#define SPI_DC          XPAR_SPI_DC_BASEADDR
#define B_RS            0x00000001

#define SPI_DTR         XPAR_SPI_BASEADDR + XSP_DTR_OFFSET
#define SPI_TFO         XPAR_SPI_BASEADDR + XSP_TFO_OFFSET
#define SPI_DRR         XPAR_SPI_BASEADDR + XSP_DRR_OFFSET
#define SPI_IISR        XPAR_SPI_BASEADDR + XSP_IISR_OFFSET
#define SPI_SR          XPAR_SPI_BASEADDR + XSP_SR_OFFSET
#define SPI_SS			XPAR_SPI_BASEADDR + XSP_SSR_OFFSET

#define cbi(reg, bitmask)       Xil_Out32(reg, Xil_In32(reg) & ~(u32)bitmask)
#define sbi(reg, bitmask)       Xil_Out32(reg, Xil_In32(reg) |= (u32)bitmask)
#define swap(type, i, j)        {type t = i; i = j; j = t;}

#define DISP_X_SIZE     240
#define DISP_Y_SIZE     320


typedef struct _current_font {
	const unsigned char *index;
	const unsigned char *unicode;
	const unsigned char *data;
	unsigned char version;
	unsigned char reserved;
	unsigned char index1_first;
	unsigned char index1_last;
	unsigned char index2_first;
	unsigned char index2_last;
	unsigned char bits_index;
	unsigned char bits_width;
	unsigned char bits_height;
	unsigned char bits_xoffset;
	unsigned char bits_yoffset;
	unsigned char bits_delta;
	unsigned char line_space;
	unsigned char cap_height;
} ILI9341_t3_font_t;

extern int fch; // Foreground color upper byte
extern int fcl; // Foreground color lower byte
extern int bch; // Background color upper byte
extern int bcl; // Background color lower byte

extern const struct _current_font *cfont;


u32 LCD_Read(char VL);
void LCD_Write_COM(char VL);  
void LCD_Write_DATA(char VL);
void LCD_Write_DATA16(uint16_t V);
//void LCD_Write_DATA_(char VH, char VL);

void initLCD(void);
void setXY(int x1, int y1, int x2, int y2);
void setColor(u8 r, u8 g, u8 b);
void setColorBg(u8 r, u8 g, u8 b);
void clrXY(void);
void clrScr(void);

void drawHLine(int x, int y, int l);
void fillRect(int x1, int y1, int x2, int y2);

void setFont(const ILI9341_t3_font_t *f);

void printChar(u8 c, int x, int y);
void lcdPrint(char *str, int x, int y);

// get dimensions of a text string under the current font in pixels
uint16_t measureTextWidth(const char* text);
uint16_t measureTextHeight(const char* text);

void drawBackground();

#endif /* LCD_H_ */
