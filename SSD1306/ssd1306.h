/*
------------------------------------------------------------------------------
~ File   : ssd1306.h
~ Author : Majid Derhambakhsh
~ Version: V1.0.0
~ Created: 02/14/2020 06:00:00 AM
~ Brief  :
~ Support:
		   E-Mail : Majid.do16@gmail.com (subject : Embedded Library Support)

		   Github : https://github.com/Majid-Derhambakhsh
------------------------------------------------------------------------------
~ Description:

~ Attention  :

~ Changes    :
------------------------------------------------------------------------------
*/

#ifndef __SSD1306_H_
#define __SSD1306_H_

/*----------------------------------------------------------*/
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Include ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#include <stdint.h>
#include "ssd1306_conf.h"

/* ------------------------------------------------------------------ */

#ifdef __CODEVISIONAVR__  /* Check compiler */

	#pragma warn_unref_func- /* Disable 'unused function' warning */

	#ifndef __I2C_UNIT_H_
		#include "I2C_UNIT/i2c_unit.h" /* Import i2c lib */
	#endif

	#include <delay.h>       /* Import delay library */

/* ------------------------------------------------------------------ */

#elif defined(__GNUC__) && !defined(USE_HAL_DRIVER)  /* Check compiler */

	#pragma GCC diagnostic ignored "-Wunused-function" /* Disable 'unused function' warning */

	#ifndef __I2C_UNIT_H_
		#include "I2C_UNIT/i2c_unit.h" /* Import i2c lib */
	#endif

	#include <util/delay.h>  /* Import delay library */

/* ------------------------------------------------------------------ */

#elif defined(USE_HAL_DRIVER)  /* Check driver */

/* ------------------------------------------------------- */

	#if defined ( __ICCARM__ ) /* ICCARM Compiler */

		#pragma diag_suppress=Pe177   /* Disable 'unused function' warning */

	#elif defined   (  __GNUC__  ) /* GNU Compiler */

	#endif /* __ICCARM__ */

/* ------------------------------------------------------- */

	#ifndef __STM32_I2C_H_
		#include "STM32_I2C/stm32_i2c.h" /* Import i2c lib */
	#endif

/* ------------------------------------------------------------------ */

#else                     /* Compiler not found */

	#error Chip or I2C Library not supported  /* Send error */

#endif /* __CODEVISIONAVR__ */

/* ------------------------------------------------------------------ */

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Defines ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ------------------------ Public ------------------------- */
/* ----------------------- GLCD Param ---------------------- */
#define _GLCD_DEV_ADDRESS      0x78 // 0x3C << 1

#define _GLCD_SUGGEST_RATIO    0xF0

/* GLCD Bits */
#define _GLCD_BIT_RW  0
#define _GLCD_BIT_SA0 1
#define _GLCD_BIT_DC  6
#define _GLCD_BIT_CO  7

/* Packet */
#define _GLCD_PACKET_SIZE              16
#define _GLCD_BUFF_SHIFT_FOR_PACKET    4

#define _GLCD_CONTRAST_MAX             UINT8_MAX
#define _GLCD_PRECHARGE_DEF            0xF1
#define _GLCD_VCOMH_DESELECT_LEVEL_DEF 0x20

/* ----------------------- GLCD Size ----------------------- */
#define _GLCD_SIZE_96x16  0
#define _GLCD_SIZE_128x32 1
#define _GLCD_SIZE_128x64 2

/* ......................... */
#ifndef _GLCD_SIZE
	#define _GLCD_SIZE _GLCD_SIZE_128x64
#endif

#if (_GLCD_SIZE == 0)

	#define _GLCD_SCREEN_WIDTH    96
	#define _GLCD_SCREEN_HEIGHT   16

#elif (_GLCD_SIZE == 1)

	#define _GLCD_SCREEN_WIDTH    128
	#define _GLCD_SCREEN_HEIGHT   32

#elif (_GLCD_SIZE == 2)

	#define _GLCD_SCREEN_WIDTH    128
	#define _GLCD_SCREEN_HEIGHT   64

#else

#endif

#define	_GLCD_SCREEN_LINE_HEIGHT  8
#define _GLCD_SCREEN_LINES        _GLCD_SCREEN_HEIGHT / _GLCD_SCREEN_LINE_HEIGHT

/* ------------------------ GLCD CMD ----------------------- */
/* Charge Pump Command Table */
#define _GLCD_CMD_CHARGE_PUMP_SET         0x8D
#define _GLCD_CMD_CHARGE_PUMP_ENABLE      0x14

#define _GLCD_CMD_SEQUENTIAL_COM_PIN_CFG  0x12
#define _GLCD_CMD_ALTERNATIVE_COM_PIN_CFG 0x02

/* ------------------------- Macro ------------------------- */
#define __GLCD_GetLine(Y)			 (Y / _GLCD_SCREEN_HEIGHT)
#define __GLCD_Min(X, Y)			 ((X < Y) ? X : Y)
#define __GLCD_AbsDiff(X, Y)		 ((X > Y) ? (X - Y) : (Y - X))
#define __GLCD_Swap(X, Y)			 do { typeof(X) t = X; X = Y; Y = t; } while (0)
#define __GLCD_Byte2ASCII(Value)	 (Value = Value + '0')
#define __GLCD_Pointer(X, Y)		 (X + ((Y / _GLCD_SCREEN_LINE_HEIGHT) *_GLCD_SCREEN_WIDTH))

#define __BitSet(x, y)               (x |= (1UL<<y))
#define __BitClear(x, y)             (x &= (~(1UL<<y)))
#define __BitToggle(x, y)            (x ^= (1UL<<y))
#define __BitCheck(x, y)             (x &  (1UL<<y) ? 1 : 0)

/* ------------------------ Library ------------------------ */
#define _SSD1306_LIBRARY_VERSION 1.0.0

/* ------------------------ Timing ------------------------- */
/* :::::::::: u second :::::::::: */

/* :::::::::: m second :::::::::: */
#define _GLCD_DELAY_MS_1     1
#define _GLCD_DELAY_MS_10    10

#define _GLCD_COM_TIMEOUT_MS 100

/* ------------------------ Public ------------------------- */
#define _GLCD_COM_TRIALS             10 /* Number of test */

#define pgm_read_byte(addr)          (*(const unsigned char *)(addr))
#define pgm_read_word(addr)          (*(const unsigned short *)(addr))

#define _BIT_SHIFT_FOR_DIVIDE_BY_8   3 // 2^3 = 8
#define _BIT_SHIFT_FOR_DEVIDE_BY_16  4 // 2^4 = 16

/* ---------------------- By compiler ---------------------- */
#ifdef __CODEVISIONAVR__  /* Check compiler */

#define _ERROR_VAL                        _STAT_ERROR /* OK status value */
#define _OK_VAL                           _STAT_OK /* OK status value */
#define I2C_MEMADD_SIZE_8BIT              _I2C_MEMADD_SIZE_8BIT /* Memory Address Size */
#define I2C_MEMADD_SIZE_16BIT             _I2C_MEMADD_SIZE_16BIT /* Memory Address Size */

#define _I2C_MEM_READY(tr,tim)            I2C_IsDeviceReady(_GLCD_DEV_ADDRESS,(tr),(tim)) /* Change function */
#define _I2C_MEM_WRITE(ma,mas,md,qu,tim)  I2C_Mem_Write(_GLCD_DEV_ADDRESS,(ma),(mas),(md),(qu),(tim)) /* Change function */
#define _I2C_MEM_READ(ma,mas,md,qu,tim)   I2C_Mem_Read(_GLCD_DEV_ADDRESS,(ma),(mas),(md),(qu),(tim)) /* Change function */
#define _I2C_MEM_ERASE(ma,mas,qu,tim)     I2C_Mem_Erase(_GLCD_DEV_ADDRESS,(ma),(mas),(qu),(tim)) /* Change function */

#ifndef _DELAY_MS
#define _DELAY_MS(t)                      delay_ms((t)) /* Change function */
#endif /* _DELAY_MS */

/* ------------------------------------------------------------------ */
#elif defined(__GNUC__) && !defined(USE_HAL_DRIVER)  /* Check compiler */

#define _ERROR_VAL                        _STAT_ERROR /* OK status value */
#define _OK_VAL                           _STAT_OK /* OK status value */
#define I2C_MEMADD_SIZE_8BIT              _I2C_MEMADD_SIZE_8BIT /* Memory Address Size */
#define I2C_MEMADD_SIZE_16BIT             _I2C_MEMADD_SIZE_16BIT /* Memory Address Size */

#define _I2C_MEM_READY(tr,tim)            I2C_IsDeviceReady(_GLCD_DEV_ADDRESS,(tr),(tim)) /* Change function */
#define _I2C_MEM_WRITE(ma,mas,md,qu,tim)  I2C_Mem_Write(_GLCD_DEV_ADDRESS,(ma),(mas),(md),(qu),(tim)) /* Change function */
#define _I2C_MEM_READ(ma,mas,md,qu,tim)   I2C_Mem_Read(_GLCD_DEV_ADDRESS,(ma),(mas),(md),(qu),(tim)) /* Change function */
#define _I2C_MEM_ERASE(ma,mas,qu,tim)     I2C_Mem_Erase(_GLCD_DEV_ADDRESS,(ma),(mas),(qu),(tim)) /* Change function */

#ifndef _DELAY_MS
#define _DELAY_MS(t)                      _delay_ms((t)) /* Change function */
#endif /* _DELAY_MS */

/* ------------------------------------------------------------------ */
#elif defined(USE_HAL_DRIVER)  /* Check driver */

#define _ERROR_VAL                          HAL_ERROR /* OK status value */
#define _OK_VAL                             HAL_OK /* OK status value */

#define _I2C_MEM_READY(tr,tim)              HAL_I2C_IsDeviceReady(&_SSD1306_I2C,_GLCD_DEV_ADDRESS,(tr),(tim)) /* Change function */
#define _I2C_MEM_WRITE(ma,mas,md,qu,tim)    HAL_I2C_Mem_Write2(&_SSD1306_I2C,_GLCD_DEV_ADDRESS,(ma),(mas),(md),(qu),(tim)) /* Change function */
#define _I2C_MEM_READ(ma,mas,md,qu,tim)     HAL_I2C_Mem_Read2(&_SSD1306_I2C,_GLCD_DEV_ADDRESS,(ma),(mas),(md),(qu),(tim)) /* Change function */
#define _I2C_MEM_ERASE(ma,mas,qu,tim)       HAL_I2C_Mem_Erase(&_SSD1306_I2C,_GLCD_DEV_ADDRESS,(ma),(mas),(qu),_MEM_STWC,(tim)) /* Change function */

#ifndef _DELAY_MS
#define _DELAY_MS(t)                        HAL_Delay((t)) /* Change function */
#endif /* _DELAY_MS */

/* ------------------------------------------------------------------ */
#else
#endif /* __CODEVISIONAVR__ */

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Types ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
typedef enum /* GLCD Display CMD */
{
	
	_GLCD_CMD_DISP_ON            = 0xAF,
	_GLCD_CMD_DISP_OFF           = 0xAE,
	_GLCD_CMD_CONTRAST_SET       = 0x81,
	_GLCD_CMD_DISP_ALL_ON_RESUME = 0xA4,
	_GLCD_CMD_DISP_ALL_ON        = 0xA5,
	_GLCD_CMD_DISP_NORMAL        = 0xA6,
	_GLCD_CMD_DISP_INVERSE       = 0xA7
	
}GLCD_FCTypeDef;

typedef enum /* GLCD Scroll CMD */
{
	
	_GLCD_CMD_SCROLL_ACTIVE    = 0x2F,
	_GLCD_CMD_SCROLL_DEACTIVE  = 0x2E,
	_GLCD_CMD_SCROLL_LEFT      = 0x27,
	_GLCD_CMD_SCROLL_RIGHT     = 0x26,
	_GLCD_CMD_SCROLL_VLEFT     = 0x2A,
	_GLCD_CMD_SCROLL_VRIGHT    = 0x29,
	_GLCD_CMD_SCROLL_VAREA_SET = 0xA3
	
}GLCD_SCTypeDef;

typedef enum /* GLCD Address CMD */
{
	
	_GLCD_CMD_PAGE_ADD_COLUMN_LOWER_SET  = 0x00,
	_GLCD_CMD_PAGE_ADD_COLUMN_HIGHER_SET = 0x10,
	_GLCD_CMD_PAGE_ADD_PAGE_START_SET    = 0xB0,
	_GLCD_CMD_PAGE_ADD_SET               = 0x22,
	_GLCD_CMD_MEM_ADD_SET                = 0x20,
	_GLCD_CMD_COLUMN_ADD_SET             = 0x21,
	
}GLCD_ACTypeDef;

typedef enum /* GLCD HW CMD */
{
	
	_GLCD_CMD_DISP_START_LINE_SET   = 0x40,
	_GLCD_CMD_DISP_START_OFFSET_SET = 0xD3,
	_GLCD_CMD_SEGMENT_REMAP_SET     = 0xA0,
	_GLCD_CMD_MULTIPLEX_RATIO_SET   = 0xA8,
	_GLCD_CMD_COM_OUTPUT_SCAN_INC   = 0xC0,
	_GLCD_CMD_COM_OUTPUT_SCAN_DEC   = 0xC8,
	_GLCD_CMD_COM_PINS_SET          = 0xDA
	
}GLCD_HWCTypeDef;

typedef enum /* Timing and Driving Scheme Setting Command */
{
	
	_GLCD_CMD_DISP_CLK_DIV_RATIO_SET   = 0xD5,
	_GLCD_CMD_DISP_OSC_FREQ_SET        = 0xD5,
	_GLCD_CMD_PRECHARGE_PERIOD_SET     = 0xD9,
	_GLCD_CMD_VCOMH_DESELECT_LEVEL_SET = 0xDB,
	_GLCD_CMD_NOP                      = 0xE3
	
}GLCD_TDCTypeDef;

typedef enum /* GLCD Status */
{
	
	_GLCD_OK     = 0,
	_GLCD_ERROR  = 1
	
}GLCD_StatusTypeDef;

typedef enum /* GLCD Colors */
{
	
	_GLCD_WHITE = 0xFF,
	_GLCD_BLACK = 0x00
	
}COLOR_TypeDef;

typedef enum /* GLCD Display Mode */
{
	
	_GLCD_DISP_INVERTED		= _GLCD_CMD_DISP_INVERSE,
	_GLCD_DISP_NON_INVERTED	= _GLCD_CMD_DISP_NORMAL
	
}GLCD_ModeTypeDef;

typedef enum /* GLCD Print Mode */
{
	
	_GLCD_PRINT_MODE_OVERWRITE = 0,
	_GLCD_PRINT_MODE_MERGE     = 1
	
}GLCD_PrintModeTypeDef;


typedef struct
{
	
	uint8_t *Name;
	uint8_t Width;
	uint8_t Height;
	uint8_t Lines;
	
	GLCD_PrintModeTypeDef Mode;
	
}GLCD_FontTypeDef;

typedef struct
{
	
	uint8_t X;
	uint8_t Y;
	
	GLCD_StatusTypeDef Status;
	GLCD_ModeTypeDef   Mode;
	GLCD_FontTypeDef   Font;
	
}GLCD_TypeDef;

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Variables ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Enum ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Struct ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Class ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototype ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* :::::::::::::::::: Transmition ::::::::::::::::: */
void GLCD_TransmitCommand(uint8_t command);
void GLCD_TransmitData(const uint8_t data);

/* :::::::::::::::::: Initialize :::::::::::::::::: */
void GLCD_Init(void);

/* :::::::::::::::::::: Control ::::::::::::::::::: */
void GLCD_Render(void);

void GLCD_SetDisplay(const uint8_t on);
void GLCD_SetContrast(const uint8_t contrast);

void GLCD_Clear(void);
void GLCD_ClearLine(const uint8_t line);

void GLCD_InvertScreen(void);
void GLCD_InvertRect(uint8_t startX, uint8_t startY, uint8_t endX, uint8_t endY);

GLCD_StatusTypeDef GLCD_Status(void);

/* ::::::::::::::::::: Location ::::::::::::::::::: */
void GLCD_GotoX(const uint8_t x);
void GLCD_GotoY(const uint8_t y);
void GLCD_GotoXY(const uint8_t x, const uint8_t y);
void GLCD_GotoLine(const uint8_t line);

uint8_t GLCD_GetX(void);
uint8_t GLCD_GetY(void);
uint8_t GLCD_GetLine(void);

/* :::::::::::::::::: Print data :::::::::::::::::: */
uint8_t GLCD_GetWidthChar(const char character);
uint16_t GLCD_GetWidthString(const char *text);
uint16_t GLCD_GetWidthString_P(const char *text);

void GLCD_PutChar(char character);
void GLCD_PutString(const char *text);
void GLCD_PutString_P(const char *text);
void GLCD_PutInteger(const int32_t value);
void GLCD_PutDouble(double value, const uint32_t tens);

/* ::::::::::::::::::::: Draw ::::::::::::::::::::: */
void GLCD_SetPixel(const uint8_t x, const uint8_t y, COLOR_TypeDef color);
void GLCD_SetPixels(const uint8_t startX, uint8_t startY, const uint8_t endX, const uint8_t endY, COLOR_TypeDef color);
void GLCD_DrawBitmap(const uint8_t *bitmap, uint8_t width, const uint8_t height, GLCD_PrintModeTypeDef mode);
void GLCD_DrawLine(uint8_t startX, uint8_t startY, uint8_t endX, uint8_t endY, COLOR_TypeDef color);
void GLCD_DrawRectangle(const uint8_t startX, const uint8_t startY, const uint8_t endX, const uint8_t endY, COLOR_TypeDef color);
void GLCD_DrawRoundRectangle(const uint8_t startX, const uint8_t startY, const uint8_t endX, const uint8_t endY, const uint8_t radius, COLOR_TypeDef color);
void GLCD_DrawTriangle(const uint8_t x1, const uint8_t y1, const uint8_t x2, const uint8_t y2, const uint8_t x3, const uint8_t y3, COLOR_TypeDef color);
void GLCD_DrawCircle(const uint8_t centerX, const uint8_t centerY, const uint8_t radius, COLOR_TypeDef color);

void GLCD_FillScreen(COLOR_TypeDef color);
void GLCD_DrawFilledRectangle(const uint8_t startX, const uint8_t startY, const uint8_t endX, const uint8_t endY, COLOR_TypeDef color);
void GLCD_DrawFilledRoundRectangle(const uint8_t startX, const uint8_t startY, const uint8_t endX, const uint8_t endY, const uint8_t radius, COLOR_TypeDef color);
void GLCD_DrawFilledTriangle(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t x3, uint8_t y3, COLOR_TypeDef color);
void GLCD_DrawFilledCircle(const uint8_t centerX, const uint8_t centerY, const uint8_t radius, COLOR_TypeDef color);

/* :::::::::::::::::::: Scroll :::::::::::::::::::: */
void GLCD_ScrollLeft(const uint8_t start, const uint8_t end);
void GLCD_ScrollRight(const uint8_t start, const uint8_t end);
void GLCD_ScrollDiagonalLeft(const uint8_t start, const uint8_t end);
void GLCD_ScrollDiagonalRight(const uint8_t start, const uint8_t end);
void GLCD_ScrollStop(void);

/* ::::::::::::::::::::: Font ::::::::::::::::::::: */
void GLCD_SetFont(const uint8_t *name, const uint8_t width, const uint8_t height, GLCD_PrintModeTypeDef mode);

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ End of the program ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#endif /* __SSD1306_H_ */
