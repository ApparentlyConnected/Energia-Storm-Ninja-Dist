//
//  Sharp BoosterPackLCD SPI
//  Example for library for Sharp BoosterPack LCD with hardware SPI
//
//
//  Author :  Stefan Schauer
//  Date   :  Jan 29, 2014
//  Version:  1.00
//  File   :  LCD_SharpBoosterPack_SPI_main.h
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

#ifndef LCD_Sharp128_SPI_h
#define LCD_Sharp128_SPI_h

#include "Energia.h"
#include "Terminal6.h"
#include "Terminal12.h"
#include "SPI.h"

#define LCD_VERTICAL_MAX    		128 	// 128 for LS013B7DH03
#define LCD_HORIZONTAL_MAX  		128 	// 128 for LS013B7DH03

#define IMAGE_FMT_1BPP_UNCOMP   	0x01

#define SHARP_SEND_TOGGLE_VCOM_COMMAND		0x01
#define SHARP_SKIP_TOGGLE_VCOM_COMMAND 		0x00

#define SHARP_LCD_TRAILER_BYTE				0x00

#define SHARP_VCOM_TOGGLE_BIT 		   		0x40

#define SHARP_LCD_CMD_CHANGE_VCOM			0x00
#define SHARP_LCD_CMD_CLEAR_SCREEN			0x20
#define SHARP_LCD_CMD_WRITE_LINE			0x80

#define P_CS   14
#define P_VCC  7
#define P_DISP 7

typedef struct 
{
    uint8_t BPP; 					// Bits per pixel and Compressed/Uncompressed
    uint16_t XSize;  				// X size
    uint16_t YSize;   				// Y size            
    uint16_t NumColors;     		// Number of Colors in Palette
    const uint32_t * pPalette;   	// Pointer to Palette
    const unsigned char * pPixel;   // Pointer to pixel data  
}
tImage;

class LCD_Sharp128_SPI {
public:
//
    LCD_Sharp128_SPI();
    LCD_Sharp128_SPI(uint8_t pinChipSelect, uint8_t pinDISP, uint8_t pinVCC);
    void begin();
    String WhoAmI();
    void clear();
    void clearBuffer();
    void setFont(uint8_t font=0);
    void setXY(uint8_t x, uint8_t y, uint8_t ulValue);
    void text(uint8_t x, uint8_t y, String s);
	void image(uint8_t x, uint8_t y, const tImage *img);
    void flush();
//
private:
//
    uint8_t _font;

	uint8_t _pinDISP;
	uint8_t _pinVCC;
	uint8_t _pinChipSelect;

	uint8_t flagSendToggleVCOMCommand;
	uint8_t sharpVCOMbit;
//
};

#endif
