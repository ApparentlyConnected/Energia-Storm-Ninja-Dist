//
//  Sharp BoosterPackLCD SPI
//  Example for library for Sharp BoosterPack LCD with hardware SPI
//
//
//  Author :  Stefan Schauer
//  Date   :  Jan 29, 2014
//  Version:  1.00
//  File   :  LCD_Sharp128_SPI_main.c
//
//	Modified:	Nathan Crum
//	Date	:	Nov 29, 2014
//	File	:	LCD_Sharp128_SPI.h
//	Changed	:	Settings modified for 128x128 Sharp's LS013B7DH03 Memory LCD
//
//	Modified:	Nathan Crum
//	Date	:	Apr 26, 2015
//	File	:	LCD_Sharp128_SPI.h
//	Changed	:	Settings modified for Prototype2 pin configuration
//
//  Based on the LCD5110 Library
//  Created by Rei VILO on 28/05/12
//  Copyright (c) 2012 http://embeddedcomputing.weebly.com
//  Licence CC = BY SA NC
//

#include <msp430.h>
#include "LCD_Sharp128_SPI.h"
#include "SPI.h"

#define TOGGLE_VCOM	do { this->sharpVCOMbit = (this->sharpVCOMbit ? 0x00 : SHARP_VCOM_TOGGLE_BIT); } while(0);

#define PERSIST __attribute__((section(".text")))
	unsigned char DisplayBuffer[LCD_VERTICAL_MAX][LCD_HORIZONTAL_MAX/8] PERSIST;

// Storm Ninja Sharp 128 (LS013B7DH03) P2
    // 02 - P2.4 for SPI_CLK mode
    // 19 - P2.5 for SPI_SIMO mode
    // 25 - P3.0 output pin for SPI_CS
    // 06 - P1.0 as output to supply the LCD
    // 06 - P1.0 as output for DISP
    // Set display's VCC and DISP pins to high

LCD_Sharp128_SPI::LCD_Sharp128_SPI() {
    this->_pinChipSelect	= P_CS;
    this->_pinDISP 			= P_DISP;
    this->_pinVCC			= P_VCC;
}


LCD_Sharp128_SPI::LCD_Sharp128_SPI(uint8_t pinChipSelect, uint8_t pinDISP, uint8_t pinVCC) {
    this->_pinChipSelect	= pinChipSelect;
    this->_pinDISP 			= pinDISP;
    this->_pinVCC			= pinVCC;
}

void LCD_Sharp128_SPI::setXY(uint8_t x, uint8_t y, uint8_t  ulValue) {
    if( ulValue != 0)
        DisplayBuffer[y][x>>3] &= ~(0x80 >> (x & 0x7));
    else
        DisplayBuffer[y][x>>3] |= (0x80 >> (x & 0x7));
}

void LCD_Sharp128_SPI::begin() {
    pinMode(this->_pinChipSelect, OUTPUT);
    pinMode(this->_pinDISP, OUTPUT);
    pinMode(this->_pinVCC, OUTPUT);

    digitalWrite(this->_pinChipSelect, LOW);
    digitalWrite(this->_pinVCC, HIGH);
    digitalWrite(this->_pinDISP, HIGH);
    
    this->flagSendToggleVCOMCommand = SHARP_SKIP_TOGGLE_VCOM_COMMAND;
    this->sharpVCOMbit 				= SHARP_VCOM_TOGGLE_BIT;
    
    clear();
    this->_font = 0;
}

String LCD_Sharp128_SPI::WhoAmI() {
    return "Sharp128 LCD Shuriken";
}

void LCD_Sharp128_SPI::clear() {
  // Set CS High (select chip)
  digitalWrite(this->_pinChipSelect, HIGH);

  SPI.transfer(this->sharpVCOMbit | SHARP_LCD_CMD_CLEAR_SCREEN);
  SPI.transfer(SHARP_LCD_TRAILER_BYTE);
  TOGGLE_VCOM;

  // Wait for last byte to be sent, then drop SCS
  __delay_cycles(100);

  // Set CS Low (deselect chip)
  digitalWrite(this->_pinChipSelect, LOW);

  clearBuffer();
  
}

void LCD_Sharp128_SPI::clearBuffer() {
    unsigned int i=0,j=0;
    for(i =0; i< LCD_VERTICAL_MAX; i++)
    for(j =0; j< (LCD_HORIZONTAL_MAX>>3); j++)
       DisplayBuffer[i][j] = 0xff;
}

void LCD_Sharp128_SPI::setFont(uint8_t font) {
    this->_font = font;
}

void LCD_Sharp128_SPI::text(uint8_t x, uint8_t y, String s) {
    uint8_t i;
    uint8_t j;
    int8_t k;
    uint8_t offset = 0;;
    
    if (this->_font==0) {
        for (j=0; j<s.length(); j++) {
            for (i=0; i<5; i++){ 
                for (k=7; k>=0; k--) setXY(x+offset, y+k, Terminal6x8[s.charAt(j)-' '][i]&(1<<k));
                offset += 1;
            }
            offset += 1;  // spacing
        }
    }
    else if (this->_font==1) {
        for (j=0; j<s.length(); j++) {
            for (i=0; i<11; i++){
                for (k=7; k>=0; k--){ 
                    setXY(x+offset, y+k,   Terminal11x16[s.charAt(j)-' '][2*i]&(1<<k));
                    setXY(x+offset, y+k+8, Terminal11x16[s.charAt(j)-' '][2*i+1]&(1<<k));
                }
                offset += 1;
            }
            offset += 1;  // spacing
        }
    }
}

/* Compatible with Texas Instruments Image Reformer tool: http://www.ti.com/tool/MSP430-GRLIB */
void LCD_Sharp128_SPI::image(uint8_t x, uint8_t y, const tImage *img) {
	uint8_t i, j;
	uint8_t imask;
	uint8_t xmax = img->XSize;
	uint8_t ymax = img->YSize;
	const uint8_t *idata = (const uint8_t*)img->pPixel;
	uint8_t xcols = xmax >> 3;
	
	if(xmax & 0x7)
		xcols++;
	
	for(i=0; i<xmax; i++ ) {
		for(j=0; j<ymax; j++ )  {
			imask = (0x80 >> (i & 0x7));
			if(imask & idata[j*xcols + (i>>3)])
				DisplayBuffer[y+j][((x+i)>>3)] |= (0x80 >> ((x+i) & 0x7));
			else
				DisplayBuffer[y+j][((x+i)>>3)] &= ~(0x80 >> ((x+i) & 0x7));
		}
	}	
}

const uint8_t referse_data[] = {0x0, 0x8, 0x4, 0xC, 0x2, 0xA, 0x6, 0xE, 0x1, 0x9, 0x5, 0xD, 0x3, 0xB, 0x7, 0xF};
uint8_t reverse(uint8_t x)
{
  uint8_t b = 0;
  
  b  = referse_data[x & 0xF]<<4;
  b |= referse_data[(x & 0xF0)>>4];
  return b;
}

void LCD_Sharp128_SPI::flush (void)
{
    unsigned char *pucData = &DisplayBuffer[0][0];
    long xi =0;
    long xj = 0;

    // Set CS High
    digitalWrite(this->_pinChipSelect, HIGH);

    SPI.transfer(SHARP_LCD_CMD_WRITE_LINE | this->sharpVCOMbit);
    TOGGLE_VCOM;

    for(xj=0; xj<LCD_VERTICAL_MAX; xj++)
    {
        SPI.transfer((char)reverse(xj + 1));		// Send current line

        for(xi=0; xi<(LCD_HORIZONTAL_MAX>>3); xi++)
        {
            SPI.transfer((char)*(pucData++));		// Send line data
        }
        SPI.transfer(SHARP_LCD_TRAILER_BYTE);		// Send end-of-line trailer byte
    }

    SPI.transfer((char)SHARP_LCD_TRAILER_BYTE);		// Send another trailer byte

    __delay_cycles(100);

    // Set CS Low
    digitalWrite(this->_pinChipSelect, LOW);
}
