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
//  Based on the LCD5110 Library
//  Created by Rei VILO on 28/05/12
//  Copyright (c) 2012 http://embeddedcomputing.weebly.com
//  Licence CC = BY SA NC
//

#include <msp430.h>
#include "LCD_Sharp128_SPI.h"
#include "SPI.h"

#define PERSIST __attribute__((section(".text")))

uint8_t _pinReset;
uint8_t _pinSerialData;
uint8_t _pinDISP;
uint8_t _pinVCC;
uint8_t _pinChipSelect;
uint8_t _pinSerialClock;
	
// Storm Ninja Sharp 128 (LS013B7DH03)
    // 02 - P2.4 for SPI_CLK mode
    // 19 - P2.5 for SPI_SIMO mode
    // 06 - P1.0 output pin for SPI_CS
    // 11 - P2.2 as output to supply the LCD
    // 11 - P2.2 as output for DISP
    // Set display's VCC and DISP pins to high

static const uint8_t P_CS   = 6;
static const uint8_t P_VCC  = 11;
static const uint8_t P_DISP = 11;

#define SHARP_LCD_TRAILER_BYTE              0x00

#define SHARP_LCD_CMD_CHANGE_VCOM           0x00
#define SHARP_LCD_CMD_CLEAR_SCREEN          0x20
#define SHARP_LCD_CMD_WRITE_LINE            0x80

unsigned char DisplayBuffer[LCD_VERTICAL_MAX][LCD_HORIZONTAL_MAX/8] PERSIST;

unsigned char VCOMbit = 0x40;
#define SHARP_VCOM_TOGGLE_BIT               0x40

unsigned char flagSendToggleVCOMCommand = 0;
#define SHARP_SEND_COMMAND_RUNNING          0x01
#define SHARP_REQUEST_TOGGLE_VCOM           0x02


static void SendToggleVCOMCommand(void);
	
LCD_Sharp128_SPI::LCD_Sharp128_SPI() {
    LCD_Sharp128_SPI(
                 P_CS,    // Chip Select
                 P_VCC,   // Vcc display
                 P_DISP   // DISP
    );
}


LCD_Sharp128_SPI::LCD_Sharp128_SPI(uint8_t pinChipSelect, uint8_t pinDISP, uint8_t pinVCC) {
    _pinChipSelect  = pinChipSelect;
    _pinDISP = pinDISP;
    _pinVCC  = pinVCC;
}

void LCD_Sharp128_SPI::setXY(uint8_t x, uint8_t y, uint8_t  ulValue) {
    if( ulValue != 0)
        DisplayBuffer[y][x>>3] &= ~(0x80 >> (x & 0x7));
    else
        DisplayBuffer[y][x>>3] |= (0x80 >> (x & 0x7));
}

void LCD_Sharp128_SPI::begin() {
    pinMode(_pinChipSelect, OUTPUT);
    pinMode(_pinDISP, OUTPUT);
    pinMode(_pinVCC, OUTPUT);

    digitalWrite(_pinChipSelect, LOW);
    digitalWrite(_pinVCC, HIGH);
    digitalWrite(_pinDISP, HIGH);
    
    TA0_enableVCOMToggle();
    
    clear();
    _font = 0;
}

String LCD_Sharp128_SPI::WhoAmI() {
    return "Sharp128 LCD Shuriken";
}

void LCD_Sharp128_SPI::clear() {
  
  unsigned char command = SHARP_LCD_CMD_CLEAR_SCREEN;
  
  // set flag to indicate command transmit is running
  flagSendToggleVCOMCommand |= SHARP_SEND_COMMAND_RUNNING;
  
  command |= VCOMbit;                    //COM inversion bit

  // Set CS High (select chip)
  digitalWrite(_pinChipSelect, HIGH);

  SPI.transfer(command);
  SPI.transfer(SHARP_LCD_TRAILER_BYTE);

  // Wait for last byte to be sent, then drop SCS
  __delay_cycles(100);

  // Set CS Low (deselect chip)
  digitalWrite(_pinChipSelect, LOW);
  
  // clear flag to indicate command transmit is free
  flagSendToggleVCOMCommand &= ~SHARP_SEND_COMMAND_RUNNING;  
  SendToggleVCOMCommand(); // send toggle if required

  clearBuffer();
  
}

void LCD_Sharp128_SPI::clearBuffer() {
    unsigned int i=0,j=0;
    for(i =0; i< LCD_VERTICAL_MAX; i++)
    for(j =0; j< (LCD_HORIZONTAL_MAX>>3); j++)
       DisplayBuffer[i][j] = 0xff;
}

void LCD_Sharp128_SPI::setFont(uint8_t font) {
    _font = font;
}

void LCD_Sharp128_SPI::text(uint8_t x, uint8_t y, String s) {
    uint8_t i;
    uint8_t j;
    int8_t k;
    uint8_t offset = 0;;
    
    if (_font==0) {
        for (j=0; j<s.length(); j++) {
            for (i=0; i<5; i++){ 
                for (k=7; k>=0; k--) setXY(x+offset, y+k, Terminal6x8[s.charAt(j)-' '][i]&(1<<k));
                offset += 1;
            }
            offset += 1;  // spacing
        }
    }
    else if (_font==1) {
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
	uint8_t imask, fmask;
	uint16_t iindex;
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
    //image update mode(1X000000b)
    unsigned char command = SHARP_LCD_CMD_WRITE_LINE;

    // set flag to indicate command transmit is running
    flagSendToggleVCOMCommand |= SHARP_SEND_COMMAND_RUNNING;
    //COM inversion bit
    command |= VCOMbit;
    // Set CS High
    digitalWrite(_pinChipSelect, HIGH);

    SPI.transfer((char)command);
    for(xj=0; xj<LCD_VERTICAL_MAX; xj++)
    {
        SPI.transfer((char)reverse(xj + 1));

        for(xi=0; xi<(LCD_HORIZONTAL_MAX>>3); xi++)
        {
            SPI.transfer((char)*(pucData++));
        }
        SPI.transfer(SHARP_LCD_TRAILER_BYTE);
    }

    SPI.transfer((char)SHARP_LCD_TRAILER_BYTE);

    __delay_cycles(100);

    // Set CS Low
    digitalWrite(_pinChipSelect, LOW);
    // clear flag to indicate command transmit is free
    flagSendToggleVCOMCommand &= ~SHARP_SEND_COMMAND_RUNNING;  
    SendToggleVCOMCommand(); // send toggle if required
}

static void SendToggleVCOMCommand(void)
{
    if(!(flagSendToggleVCOMCommand & SHARP_REQUEST_TOGGLE_VCOM)){ // no request pending ?
        VCOMbit ^= SHARP_VCOM_TOGGLE_BIT;                 // Toggle VCOM Bit
    }

    if(flagSendToggleVCOMCommand & SHARP_SEND_COMMAND_RUNNING){
        // set request flag
        flagSendToggleVCOMCommand |= SHARP_REQUEST_TOGGLE_VCOM;
    }else{  // if no communication to LCD -> send toggle sequence now
        unsigned char command = SHARP_LCD_CMD_CHANGE_VCOM;
        command |= VCOMbit;                    //COM inversion bit

        // Set P2.4 High for CS
        digitalWrite(_pinChipSelect, HIGH);

        SPI.transfer((char)command);
        SPI.transfer((char)SHARP_LCD_TRAILER_BYTE);

        // Wait for last byte to be sent, then drop SCS
        __delay_cycles(100);
        // Set P2.4 High for CS
        digitalWrite(_pinChipSelect, LOW);
        // clear request flag
        flagSendToggleVCOMCommand &= ~SHARP_REQUEST_TOGGLE_VCOM;
    }
}

// the part below is MSP430 specific and would need modifications for other platforms
#ifdef __MSP430__
void LCD_Sharp128_SPI::TA0_enableVCOMToggle()
{
    // generate Int. each 4096*8*32768Hz = 1 sec
//  TA0CTL = TASSEL__ACLK | ID__8 | TACLR | MC__UP;
    TA0CTL = TASSEL_1 | ID_3 | TACLR | MC_1;
    TA0CCTL0 = CCIE;
    TA0CCR0 = 4096;
}


void LCD_Sharp128_SPI::TA0_turnOff()
{
    TA0CTL = 0;
    TA0CCTL0 = 0;
}

#pragma vector=TIMER0_A0_VECTOR
__interrupt void TA0_ISR(void)
{
    SendToggleVCOMCommand();
}
#else
#error Platform not supported
#endif
