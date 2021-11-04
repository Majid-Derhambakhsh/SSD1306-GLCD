/*
------------------------------------------------------------------------------
~ File   : ssd1306.c
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

#include "ssd1306.h"

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Variables ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
uint8_t GLCD_Buffer[_GLCD_SCREEN_WIDTH * _GLCD_SCREEN_LINES];

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Struct ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
GLCD_TypeDef GLCD;

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Prototypes ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
static void GLCD_BufferWrite(const uint8_t x, const uint8_t y, const uint8_t data);
static uint8_t GLCD_BufferRead(const uint8_t x, const uint8_t y);
static inline void GLCD_DrawHLine(uint8_t startX, uint8_t endX, const uint8_t y, COLOR_TypeDef color);
static inline void GLCD_DrawVLine(uint8_t startY, uint8_t endY, const uint8_t x, COLOR_TypeDef color);
static void Int2bcd(int32_t value, char BCD[]);

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Functions ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
/* :::::::::::::::::: Transmition ::::::::::::::::: */
void GLCD_TransmitCommand(uint8_t command)
{
	_I2C_MEM_WRITE(0 << _GLCD_BIT_DC, I2C_MEMADD_SIZE_8BIT, &command, 1, _GLCD_COM_TIMEOUT_MS);
}
void GLCD_TransmitData(const uint8_t data)
{
	GLCD_BufferWrite(GLCD.X++, GLCD.Y, data);
}

/* :::::::::::::::::: Initialize :::::::::::::::::: */
void GLCD_Init(void)
{
	
	/* ~~~~~~~~~~~~~~~~~~~~ I2C Init ~~~~~~~~~~~~~~~~~~~~ */
	// I2C_Init();
	
	/* ~~~~~~~~~~~~~~~~~~~~ GLCD Init ~~~~~~~~~~~~~~~~~~~ */
	GLCD_TransmitCommand(_GLCD_CMD_DISP_OFF); // 0xAE
	
	/* ..... Set Clock ..... */
	GLCD_TransmitCommand(_GLCD_CMD_DISP_CLK_DIV_RATIO_SET); // 0xD5
	GLCD_TransmitCommand(_GLCD_SUGGEST_RATIO); // 0xF0
	
	/* ..... Set MUX Ratio ..... */
	GLCD_TransmitCommand(_GLCD_CMD_MULTIPLEX_RATIO_SET); // 0xA8
	GLCD_TransmitCommand(_GLCD_SCREEN_HEIGHT - 1);
	
	/* ..... Set Display Offset ..... */
	GLCD_TransmitCommand(_GLCD_CMD_DISP_START_OFFSET_SET); // 0xD3
	GLCD_TransmitCommand(0x00); //No offset
	
	/* ..... Enable Charge Pump Regulator ..... */
	GLCD_TransmitCommand(_GLCD_CMD_CHARGE_PUMP_SET); // 0x8D
	GLCD_TransmitCommand(_GLCD_CMD_CHARGE_PUMP_ENABLE); // 0x14
	
	/* ..... Set Display Start Line ..... */
	GLCD_TransmitCommand(_GLCD_CMD_DISP_START_LINE_SET | 0x00); // 0x40 | Start line
	
	/* ..... Set Memory Address ..... */
	GLCD_TransmitCommand(_GLCD_CMD_MEM_ADD_SET); // 0x20
	GLCD_TransmitCommand(0x00); // Horizontal Addressing - Operate like KS0108
	
	/* ..... Set Segment re-map ..... */
	GLCD_TransmitCommand(_GLCD_CMD_SEGMENT_REMAP_SET | 0x01); // 0xA0 - Left towards Right
	
	/* ..... Set COM Output Scan Direction ..... */
	GLCD_TransmitCommand(_GLCD_CMD_COM_OUTPUT_SCAN_DEC); // 0xC8 - Up towards Down
	
	/* ..... Set COM Pins hardware configuration ..... */
	GLCD_TransmitCommand(_GLCD_CMD_COM_PINS_SET); // 0xDA
	
	#if (_GLCD_SIZE == _GLCD_SIZE_128x64)
		GLCD_TransmitCommand(_GLCD_CMD_SEQUENTIAL_COM_PIN_CFG); // Sequential COM pin configuration
	#else 
		GLCD_TransmitCommand(_GLCD_CMD_ALTERNATIVE_COM_PIN_CFG); //Alternative COM pin configuration
	#endif
	
	/* ..... Set Contrast Control ..... */
	GLCD_TransmitCommand(_GLCD_CMD_CONTRAST_SET); // 0x81
	GLCD_TransmitCommand(_GLCD_CONTRAST_MAX);
	
	/* ..... Set Precharge Period ..... */
	GLCD_TransmitCommand(_GLCD_CMD_PRECHARGE_PERIOD_SET); // 0xD9
	GLCD_TransmitCommand(_GLCD_PRECHARGE_DEF);
	
	/* ..... Set VCOM ..... */
	GLCD_TransmitCommand(_GLCD_CMD_VCOMH_DESELECT_LEVEL_SET); // 0xDB
	GLCD_TransmitCommand(_GLCD_VCOMH_DESELECT_LEVEL_DEF);
	
	/* ..... Set Display ..... */
	GLCD_TransmitCommand(_GLCD_CMD_DISP_ALL_ON_RESUME); // 0xA4
	GLCD_TransmitCommand(_GLCD_CMD_DISP_NORMAL); // 0xA6
	GLCD_TransmitCommand(_GLCD_CMD_SCROLL_DEACTIVE); // 0x2E
	GLCD_TransmitCommand(_GLCD_CMD_DISP_ON); // 0xAF
	
	/* ~~~~~~~~~~~~~~ Set Display Location ~~~~~~~~~~~~~~ */
	GLCD_GotoXY(0, 0);
	
	/* ..... Reset GLCD structure ..... */
	GLCD.Mode = _GLCD_DISP_NON_INVERTED;
	GLCD.X = GLCD.Y = GLCD.Font.Width = GLCD.Font.Height = GLCD.Font.Lines = 0;
	
}

/* :::::::::::::::::::: Control ::::::::::::::::::: */
void GLCD_Render(void)
{
	/* We have to send buffer as 16-byte packets
	
	Buffer Size:    Width * Height / Line_Height
	Packet Size:    16
	Loop Counter:   Buffer size / Packet Size      =
	                = ((Width * Height) / 8) / 16  =
	                = (Width / 16) * (Height / 8)  =
	                = (Width >> 4) * (Height >> 3)
	
	*/
	
	uint8_t buff_counter = 0;
	uint8_t buff_loop    = (_GLCD_SCREEN_WIDTH >> _BIT_SHIFT_FOR_DEVIDE_BY_16) * (_GLCD_SCREEN_HEIGHT >> _BIT_SHIFT_FOR_DIVIDE_BY_8);
	
	/* ~~~~~~~~~~~~~~~~~~~~ Set Columns Add ~~~~~~~~~~~~~~~~~~~~ */
	GLCD_TransmitCommand(_GLCD_CMD_COLUMN_ADD_SET); // 0x21
	GLCD_TransmitCommand(0x00); // Start
	GLCD_TransmitCommand(_GLCD_SCREEN_WIDTH - 1); // End
	
	/* ~~~~~~~~~~~~~~~~~~~~~~ Set Rows Add ~~~~~~~~~~~~~~~~~~~~~ */
	GLCD_TransmitCommand(_GLCD_CMD_PAGE_ADD_SET); // 0x22
	GLCD_TransmitCommand(0x00); // Start
	GLCD_TransmitCommand(_GLCD_SCREEN_LINES - 1); // End
	
	/* ~~~~~~~~~~~~~~~~~~~~~~ Send Buffer ~~~~~~~~~~~~~~~~~~~~~~ */
	for (; buff_counter < buff_loop; buff_counter++)
	{
		_I2C_MEM_WRITE(1 << _GLCD_BIT_DC, I2C_MEMADD_SIZE_8BIT, &GLCD_Buffer[buff_counter << _GLCD_BUFF_SHIFT_FOR_PACKET], _GLCD_PACKET_SIZE, _GLCD_COM_TIMEOUT_MS);
	}
	
}

void GLCD_SetDisplay(const uint8_t on)
{
	GLCD_TransmitCommand(on ? _GLCD_CMD_DISP_ON : _GLCD_CMD_DISP_OFF);
}

void GLCD_SetContrast(const uint8_t contrast)
{
	GLCD_TransmitCommand(_GLCD_CMD_CONTRAST_SET);
	GLCD_TransmitCommand(contrast);
}

void GLCD_Clear(void)
{
	GLCD_FillScreen(_GLCD_WHITE);
}

void GLCD_ClearLine(const uint8_t line)
{
	
	uint8_t column_counter = 0;
	
	/* ~~~~~~~~~~~~~~~~~~~~ Check Value ~~~~~~~~~~~~~~~~~~~~ */
	if (line < _GLCD_SCREEN_LINES)
	{
		
		/* :::::::::: Goto Line :::::::::: */
		GLCD_GotoXY(0, line * _GLCD_SCREEN_LINE_HEIGHT);
		
		/* ::::::::: Fill Pixels ::::::::: */
		for (column_counter = 0 ; column_counter < _GLCD_SCREEN_WIDTH ; column_counter++)
		{
			GLCD_BufferWrite(column_counter, GLCD.Y, _GLCD_WHITE);
		}
		
	}
	
}

void GLCD_InvertScreen(void)
{
	
	if (GLCD.Mode == _GLCD_DISP_INVERTED)
	{
		GLCD.Mode = _GLCD_DISP_NON_INVERTED;
	}
	else
	{
		GLCD.Mode = _GLCD_DISP_INVERTED;
	}

	GLCD_TransmitCommand(GLCD.Mode);
	
}

void GLCD_InvertRect(uint8_t startX, uint8_t startY, uint8_t endX, uint8_t endY)
{
	
	uint8_t width;
	uint8_t height;
	uint8_t offset;
	uint8_t mask;
	uint8_t h;
	uint8_t i;
	uint8_t data;
	
	/* ~~~~~~~~~~~~~~~~~~~~~ Calculate Parameter ~~~~~~~~~~~~~~~~~~~~~ */
	width   = endX - startX + 1;
	height  = endY - startY + 1;
	offset  = startY % _GLCD_SCREEN_LINE_HEIGHT;
	startY -= offset;
	mask    = 0xFF;
	data    = 0;
	
	/* ~~~~~~~~~~~ Calculate mask for top fractioned region ~~~~~~~~~~ */
	if (height < (_GLCD_SCREEN_LINE_HEIGHT - offset))
	{
		mask >>= (_GLCD_SCREEN_LINE_HEIGHT - height);
		h = height;
	}
	else
	{
		h = _GLCD_SCREEN_LINE_HEIGHT - offset;
	}
	
	mask <<= offset;
	
	/* ~~~~~~~~ Draw fractional rows at the top of the region ~~~~~~~~ */
	GLCD_GotoXY(startX, startY);
	
	for (i = 0; i < width; i++)
	{
		
		data = GLCD_BufferRead(GLCD.X, GLCD.Y);
		data = ((~data) & mask) | (data & (~mask));
		
		GLCD_BufferWrite(GLCD.X++, GLCD.Y, data);
		
	}
	
	/* ~~~~~~~~~~~~~~~~~~~~~~~~~~ Full rows ~~~~~~~~~~~~~~~~~~~~~~~~~~ */
	while ((h + _GLCD_SCREEN_LINE_HEIGHT) <= height)
	{
		
		h      += _GLCD_SCREEN_LINE_HEIGHT;
		startY += _GLCD_SCREEN_LINE_HEIGHT;
		
		GLCD_GotoXY(startX, startY);
		
		for (i=0; i < width; i++)
		{
			
			data = ~GLCD_BufferRead(GLCD.X, GLCD.Y);
			
			GLCD_BufferWrite(GLCD.X++, GLCD.Y, data);
			
		}
		
	}
	
	/* ~~~~~~~~~ Fractional rows at the bottom of the region ~~~~~~~~~ */
	if (h < height)
	{
		
		mask = ~(0xFF << (height - h));
		
		GLCD_GotoXY(startX, (startY + _GLCD_SCREEN_LINE_HEIGHT));

		for (i = 0; i < width; i++)
		{
			
			data = GLCD_BufferRead(GLCD.X, GLCD.Y);
			data = ((~data) & mask) | (data & (~mask));
			
			GLCD_BufferWrite(GLCD.X++, GLCD.Y, data);
			
		}
		
	}
	
}

GLCD_StatusTypeDef GLCD_Status(void)
{
	return (GLCD.Status);
}

/* ::::::::::::::::::: Location ::::::::::::::::::: */
void GLCD_GotoX(const uint8_t x)
{
	if (x < _GLCD_SCREEN_WIDTH)
	{
		GLCD.X = x;
	}
}

void GLCD_GotoY(const uint8_t y)
{
	if (GLCD.Y < _GLCD_SCREEN_HEIGHT)
	{
		GLCD.Y = y;
	}
}

void GLCD_GotoXY(const uint8_t x, const uint8_t y)
{
	GLCD_GotoX(x);
	GLCD_GotoY(y);
}

void GLCD_GotoLine(const uint8_t line)
{
	if (line < _GLCD_SCREEN_LINES)
	{
		GLCD.Y = line * _GLCD_SCREEN_LINE_HEIGHT;
	}
}

uint8_t GLCD_GetX(void)
{
	return GLCD.X;
}

uint8_t GLCD_GetY(void)
{
	return GLCD.Y;
}

uint8_t GLCD_GetLine(void)
{
	return (__GLCD_GetLine(GLCD.Y));
}

/* :::::::::::::::::: Print data :::::::::::::::::: */
uint8_t GLCD_GetWidthChar(const char character)
{
	/* +1 for space after each character */
	return (pgm_read_byte(&(GLCD.Font.Name[(character - 32) * (GLCD.Font.Width * GLCD.Font.Lines + 1)])) + 1);
}

uint16_t GLCD_GetWidthString(const char *text)
{
	
	uint16_t width = 0;

	while (*text)
	{
		width += GLCD_GetWidthChar(*text++);
	}
	
	return width;
	
}

uint16_t GLCD_GetWidthString_P(const char *text)
{
	
	uint16_t width = 0;
	
	/* ~~~~~~~~~~~~~~~~~~~~~ Calculate Parameter ~~~~~~~~~~~~~~~~~~~~~ */
	char r = pgm_read_byte(text++);

	while (r)
	{
		
		width += GLCD_GetWidthChar(r);
		
		r = pgm_read_byte(text++);
		
	}
	
	return width;
	
}

void GLCD_PutChar(char character)
{
	
	/* If it doesn't work, replace pgm_read_byte with pgm_read_word */
	uint16_t fontStart    = 0;
	uint16_t fontRead     = 0;
	uint16_t fontReadPrev = 0;
	
	uint8_t x        = 0;
	uint8_t y        = 0;
	uint8_t y2       = 0;
	uint8_t i        = 0;
	uint8_t j        = 0;
	uint8_t width    = 0;
	uint8_t overflow = 0;
	uint8_t data     = 0;
	uint8_t dataPrev = 0;
	
	/* ~~~~~~~~~~~~~~~~~~~~~~~~~~ Save current position ~~~~~~~~~~~~~~~~~~~~~~~~~~ */
	x = GLCD.X;
	y = y2 = GLCD.Y;
	
	/* ~~~~~~~~~~~~~~~~~~~~~ Remove leading empty characters ~~~~~~~~~~~~~~~~~~~~~ */
	character -= 32; // 32 is the ASCII of the first printable character
	
	/* ~~~~~~~~~~~~ Find the start of the character in the font array ~~~~~~~~~~~~ */
	fontStart = character * (GLCD.Font.Width * GLCD.Font.Lines + 1); // +1 due to first byte of each array line being the width
	
	/* ~~~~ Update width-First byte of each line is the width of the character ~~~ */
	width = pgm_read_byte(&(GLCD.Font.Name[fontStart++]));
	
	/* ~~~~~~~~~~~~~~~~~~~~~~~~ Calculate overflowing bits ~~~~~~~~~~~~~~~~~~~~~~~ */
	overflow = GLCD.Y % _GLCD_SCREEN_LINE_HEIGHT;
	
	/* ~~~~~~~~~~~~~~~ Print the character / Scan the lines needed ~~~~~~~~~~~~~~~ */
	for (j = 0; j < GLCD.Font.Lines; j++)
	{
		
		/* ::::::::::: Go to the start of the line ::::::::::: */
		GLCD_GotoXY(x, y);
		
		/* ::::: Update the indices for reading the line ::::: */
		fontRead     = fontStart + j;
		fontReadPrev = fontRead - 1;
		
		/* ::::::::::: Scan bytes of selected line ::::::::::: */
		for (i = 0 ; i < width ; i++)
		{
			
			/* ----------------- Read Byte --------------- */
			data = pgm_read_byte(&(GLCD.Font.Name[fontRead]));
			
			/* ---------------- Shift Byte --------------- */
			data <<= overflow;
			
			/* ------- Merge byte with previous one ------ */
			if (j > 0)
			{
				dataPrev       = pgm_read_byte(&(GLCD.Font.Name[fontReadPrev]));
				dataPrev     >>= _GLCD_SCREEN_LINE_HEIGHT - overflow;
				data          |= dataPrev;
				fontReadPrev  += GLCD.Font.Lines;
			}
			
			/* ----- Edit byte depending on the mode ----- */
			if (GLCD.Font.Mode == _GLCD_PRINT_MODE_MERGE)
			{
				data |= GLCD_BufferRead(GLCD.X, GLCD.Y);
			}
			
			/* ----------------- Send Byte --------------- */
			GLCD_BufferWrite(GLCD.X++, GLCD.Y, data);
			
			/* -------------- Increase index ------------- */
			fontRead += GLCD.Font.Lines;
			
		}
		
		/* :::::: Send an empty column of 1px in the end ::::: */
		if (GLCD.Font.Mode == _GLCD_PRINT_MODE_OVERWRITE)
		{
			GLCD_BufferWrite(GLCD.X, GLCD.Y, _GLCD_WHITE);
		}
		
		/* :::::::::::::: Increase line counter :::::::::::::: */
		y += _GLCD_SCREEN_LINE_HEIGHT;
		
	}
	
	/* ~~~~~~~~~~~~~~~~~~~~~~~ Update last line, if needed  ~~~~~~~~~~~~~~~~~~~~~~ */
	//if (LINE_STARTING != LINE_ENDING)
	if (__GLCD_GetLine(y2) != __GLCD_GetLine(y2 + GLCD.Font.Height))
	{
		
		/* ::::::::::::::: Go to the start of the line ::::::::::::::: */
		GLCD_GotoXY(x, y);
		
		/* :::: pdate the index for reading the last printed line :::: */
		fontReadPrev = fontStart + j - 1;
		
		/* ::::::::::::::: Scan bytes of selected line ::::::::::::::: */
		for (i = 0; i < width; i++)
		{
			
			/* ----------------- Read Byte --------------- */
			data = GLCD_BufferRead(GLCD.X, GLCD.Y);
			
			/* ------- Merge byte with previous one ------ */
			dataPrev   = pgm_read_byte(&(GLCD.Font.Name[fontReadPrev]));
			dataPrev >>= _GLCD_SCREEN_LINE_HEIGHT - overflow;
			data      |= dataPrev;
			
			/* ----- Edit byte depending on the mode ----- */
			if (GLCD.Font.Mode == _GLCD_PRINT_MODE_MERGE)
			{
				data |= GLCD_BufferRead(GLCD.X, GLCD.Y);
			}
			
			/* ----------------- Send Byte --------------- */
			GLCD_BufferWrite(GLCD.X++, GLCD.Y, data);
			
			/* -------------- Increase index ------------- */
			fontReadPrev += GLCD.Font.Lines;
			
		}
		
		/* :::::::::: Send an empty column of 1px in the end ::::::::: */
		if (GLCD.Font.Mode == _GLCD_PRINT_MODE_OVERWRITE)
		{
			GLCD_BufferWrite(GLCD.X, GLCD.Y, _GLCD_WHITE);
		}
		
	}
	
	/* ~~~~~~~~~~~~~ Move cursor to the end of the printed character ~~~~~~~~~~~~~ */
	GLCD_GotoXY(x + width + 1, y2);
	
}

void GLCD_PutString(const char *text)
{
	
	while(*text)
	{
		
		/* ~~~~~~~~~~~~~~~~~~~~~ Check width ~~~~~~~~~~~~~~~~~~~~~ */
		if ((GLCD.X + GLCD.Font.Width) >= _GLCD_SCREEN_WIDTH)
		{
			break;
		}
		
		/* ~~~~~~~~~~~~~~~~~~~~~ Print data ~~~~~~~~~~~~~~~~~~~~~~ */
		GLCD_PutChar(*text++);
		
	}
	
}

void GLCD_PutString_P(const char *text)
{
	
	char r = pgm_read_byte(text++);
	
	/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
	while(r)
	{
		
		/* :::::::::::::::: Check width :::::::::::::::: */
		if ((GLCD.X + GLCD.Font.Width) >= _GLCD_SCREEN_WIDTH)
		{
			break;
		}
		
		/* ::::::::::::::::: Print data :::::::::::::::: */
		GLCD_PutChar(r);
		
		r = pgm_read_byte(text++);
		
	}
	
}

void GLCD_PutInteger(const int32_t value)
{
	
	//int32_max_bytes + sign + null = 12 bytes
	char bcd[12] = {'\0'};
	
	if (value == 0)
	{
		GLCD_PutChar('0');
	}
	else if ((value > INT32_MIN) && (value <= INT32_MAX))
	{
		
		/* ~~~~~~~~~~~~~~ Convert integer to array ~~~~~~~~~~~~~~~ */
		Int2bcd(value, bcd);
		
		/* ~~~~~~~~~~~ Print from first non-zero digit ~~~~~~~~~~~ */
		GLCD_PutString(bcd);
		
	}
	else { }
	
}

void GLCD_PutDouble(double value, const uint32_t tens)
{
	
	if (value == 0)
	{
		
		GLCD_PutChar('0');
		GLCD_PutChar('.');
		GLCD_PutChar('0');
		
	}
	else if ((value >= (-2147483647)) && (value < 2147483648))
	{
		
		/* ~~~~~~~~~~~~~~~~~~ Print sign ~~~~~~~~~~~~~~~~~~ */
		if (value < 0)
		{
			value = -value;
			GLCD_PutChar('-');
		}
		
		/* ~~~~~~~~~~~~~~ Print integer part ~~~~~~~~~~~~~~ */
		GLCD_PutInteger(value);
		
		/* ~~~~~~~~~~~~~~~~~~~ Print dot ~~~~~~~~~~~~~~~~~~ */
		GLCD_PutChar('.');
		
		/* ~~~~~~~~~~~~~~ Print decimal part ~~~~~~~~~~~~~~ */
		GLCD_PutInteger((value - (uint32_t)(value)) * tens);
		
	}
	
}

/* ::::::::::::::::::::: Draw ::::::::::::::::::::: */
void GLCD_SetPixel(const uint8_t x, const uint8_t y, COLOR_TypeDef color)
{
	uint8_t data = 0;
	
	/* ~~~~~~~~~~~~~~~~~~~~ Goto Point ~~~~~~~~~~~~~~~~~~~~ */
	GLCD_GotoXY(x, y);
	
	/* ~~~~~~~~~~~~~~~~~~~~ Read Data ~~~~~~~~~~~~~~~~~~~~~ */
	data = GLCD_BufferRead(GLCD.X, GLCD.Y);
	
	/* ~~~~~~~~~~~~~~~~~~~~ Set Pixel ~~~~~~~~~~~~~~~~~~~~~ */
	if (color == _GLCD_BLACK)
	{
		__BitSet(data, y % 8);
	}
	else
	{
		__BitClear(data, y % 8);
	}
	
	/* ~~~~~~~~~~~~~~~~~~~~ Sent Data ~~~~~~~~~~~~~~~~~~~~~ */
	GLCD_BufferWrite(GLCD.X, GLCD.Y, data);
	
}

void GLCD_SetPixels(const uint8_t startX, uint8_t startY, const uint8_t endX, const uint8_t endY, COLOR_TypeDef color)
{
	
	uint8_t reg_height   = 0;
	uint8_t reg_width    = 0;
	uint8_t reg_offset   = 0;
	uint8_t reg_mask     = 0;
	uint8_t nreg_height  = 0;
	uint8_t pixel_count  = 0;
	uint8_t data         = 0;
	
	/* ~~~~~~~~~~~~~~~~~~~~ Check Value ~~~~~~~~~~~~~~~~~~~~ */
	if ((startX < _GLCD_SCREEN_WIDTH) && (endX < _GLCD_SCREEN_WIDTH) &&
	    (startY < _GLCD_SCREEN_HEIGHT) && (endY < _GLCD_SCREEN_HEIGHT))
	{
		
		reg_height  = endY - startY + 1;
		reg_width   = endX - startX + 1;
		reg_offset  = startY % _GLCD_SCREEN_LINE_HEIGHT;
		startY     -= reg_offset;
		reg_mask    = 0xFF;
		data        = 0;
		
		/* ::::::: Calculate Mask For Top Fractioned Region :::::::: */
		if (reg_height < (_GLCD_SCREEN_LINE_HEIGHT - reg_offset))
		{
			
			reg_mask    >>= (_GLCD_SCREEN_LINE_HEIGHT - reg_height);
			nreg_height   = reg_height;
			
		}
		else
		{
			nreg_height = _GLCD_SCREEN_LINE_HEIGHT - reg_offset;
		}
		
		reg_mask <<= reg_offset;
		
		/* ::::: Draw Fractional Rows At The Top Of The Region ::::: */
		GLCD_GotoXY(startX, startY);
		
		for (pixel_count = 0 ; pixel_count < reg_width ; pixel_count++)
		{
			
			/* ----- Read ----- */
			data = GLCD_BufferRead(GLCD.X, GLCD.Y);
			
			/* ----- Mask ----- */
			data = ((color == _GLCD_BLACK) ? (data | reg_mask) : (data & ~reg_mask));
			
			/* ---- Write ----- */
			GLCD_BufferWrite(GLCD.X++, GLCD.Y, data);
			
		}
		
		/* ::::::::::::::::::::::: Full rows ::::::::::::::::::::::: */
		while ((nreg_height + _GLCD_SCREEN_LINE_HEIGHT) <= reg_height)
		{
			
			/* ----- Set Point ----- */
			nreg_height += _GLCD_SCREEN_LINE_HEIGHT;
			startY      += _GLCD_SCREEN_LINE_HEIGHT;
			
			/* ---- Goto Point ----- */
			GLCD_GotoXY(startX, startY);
			
			/* ---- Write Buff ----- */
			for (pixel_count = 0; pixel_count < reg_width; pixel_count++)
			{
				GLCD_BufferWrite(GLCD.X++, GLCD.Y, color);
			}
			
		}
		
		/* :::::: Fractional Rows At The Bottom Of The Region :::::: */
		if (nreg_height < reg_height)
		{
			
			reg_mask = ~(0xFF << (reg_height - nreg_height));
			
			/* ----- Goto Point ----- */
			GLCD_GotoXY(startX, startY + _GLCD_SCREEN_LINE_HEIGHT);
			
			for (pixel_count = 0; pixel_count < reg_width; pixel_count++)
			{
				
				/* """"" Read """"" */
				data = GLCD_BufferRead(GLCD.X, GLCD.Y);
				
				/* """"" Mask """"" */
				data = ((color == _GLCD_BLACK) ? (data | reg_mask) : (data & ~reg_mask));
				
				/* """" Write """"" */
				GLCD_BufferWrite(GLCD.X++, GLCD.Y, data);
				
			}
			
		}
		
	}
	
}

void GLCD_DrawBitmap(const uint8_t *bitmap, uint8_t width, const uint8_t height, GLCD_PrintModeTypeDef mode)
{
	
	uint16_t lines              = 0;
	uint16_t line_counter       = 0;
	uint16_t line_pixels_ounter = 0;
	
	uint16_t bmpRead     = 0;
	uint16_t bmpReadPrev = 0;
	uint16_t data        = 0;
	uint16_t dataPrev    = 0;
	uint16_t overflow    = 0;
	
	/* ~~~~~~~~~~~~~~~~~~~~ Save Current Position ~~~~~~~~~~~~~~~~~~~~ */
	uint8_t  x_pos           = GLCD.X;
	uint16_t y_pos           = GLCD.Y;
	uint16_t y_pos_for_draw  = GLCD.Y;
	
	/* ~~~~~~~~~~~~~~~~ Read Width - First Two Bytes ~~~~~~~~~~~~~~~~~ */
	data = GLCD.X + width; // "data" is used temporarily
	
	/* .....  Reduce Bitmap Data For Screen Size ..... */
	if (data >= _GLCD_SCREEN_WIDTH)
	{
		width -= (data - _GLCD_SCREEN_WIDTH);
	}
	
	/* ~~~~~~ Read Height - Second Two Bytes - Convert To Lines ~~~~~~ */
	lines = (height + _GLCD_SCREEN_LINE_HEIGHT - 1) / _GLCD_SCREEN_LINE_HEIGHT; // lines = Ceiling(A/B) = (A+B-1)/B
	data  = GLCD.Y / _GLCD_SCREEN_LINE_HEIGHT + lines; // "data" is used temporarily
	
	/* .....  Reduce Bitmap Data For Screen Size ..... */
	if (data > _GLCD_SCREEN_LINES)
	{
		lines -= data - _GLCD_SCREEN_LINES;
	}
	
	/* ~~~~~~~~~~~~~~~~~ Calculate Overflowing Bits ~~~~~~~~~~~~~~~~~~ */
	overflow = GLCD.Y % _GLCD_SCREEN_LINE_HEIGHT;
	
	/* ~~~~~~~~~~~~~~~~~~~~~ Print The Character ~~~~~~~~~~~~~~~~~~~~~ */
	/* ..... Scan The Lines Needed ..... */
	for (line_counter = 0; line_counter < lines; line_counter++)
	{
		
		/* ::::::::::: Goto The Start Of The Line :::::::::::: */
		GLCD_GotoXY(x_pos, y_pos_for_draw);
		
		/* ::::: Update The Indices For Reading The Line ::::: */
		bmpRead     = line_counter * width;
		bmpReadPrev = bmpRead - width; // Previous = 4 + (j - 1) * width = Current - width
		
		/* ::::::::::: Scan Bytes Of Selected Line ::::::::::: */
		for (line_pixels_ounter = 0 ; line_pixels_ounter < width ; line_pixels_ounter++)
		{
			
			/* ----------------- Read Byte --------------- */
			data = pgm_read_byte(&(bitmap[bmpRead++]));
			
			/* ---------------- Shift Byte --------------- */
			data <<= overflow;
			
			/* ------- Merge Byte With Previous One ------ */
			if (line_counter > 0)
			{
				
				dataPrev   = pgm_read_byte( &(bitmap[bmpReadPrev++]) );
				dataPrev >>= _GLCD_SCREEN_LINE_HEIGHT - overflow;
				data      |= dataPrev;
				
			}
			
			/* ----- Edit Byte Depending On The Mode ----- */
			if (mode == _GLCD_PRINT_MODE_MERGE)
			{
				data |= GLCD_BufferRead(GLCD.X, GLCD.Y);
			}
			
			/* ---------------- Send Byte ---------------- */
			GLCD_BufferWrite(GLCD.X++, GLCD.Y, data);
			
		}
		
		/* :::::: Send An Empty Column Of 1px In The End ::::: */
		if (GLCD.Font.Mode == _GLCD_PRINT_MODE_OVERWRITE)
		{
			data = _GLCD_WHITE;
		}
		else
		{
			data = GLCD_BufferRead(GLCD.X, GLCD.Y);
		}
		
		GLCD_BufferWrite(GLCD.X, GLCD.Y, data);
		
		/* :::::::::::::: Increase Line Counter :::::::::::::: */
		y_pos_for_draw += _GLCD_SCREEN_LINE_HEIGHT;
		
	}
	
	/* ~~~~~~~~~~~~~~~~~ Update Last Line, If Needed ~~~~~~~~~~~~~~~~~ */
	if (lines > 1)
	{
		
		/* :::::::::::::::::: Goto The Start Of The Line :::::::::::::::::: */
		GLCD_GotoXY(x_pos, y_pos_for_draw);
		
		/* ::::: Update The Indices For Reading The Last Printed Line ::::: */
		bmpReadPrev = (line_counter - 1) * width;
		
		/* ::::::::::::::::: Scan Bytes Of Selected Line :::::::::::::::::: */
		for (line_pixels_ounter = 0 ; line_pixels_ounter < width ; line_pixels_ounter++)
		{
			
			/* ----------------- Read Byte --------------- */
			data = GLCD_BufferRead(GLCD.X, GLCD.Y);
			
			/* ------- Merge Byte With Previous One ------ */
			dataPrev   = pgm_read_byte( &(bitmap[bmpReadPrev++]) );
			dataPrev >>= _GLCD_SCREEN_LINE_HEIGHT - overflow;
			data      |= dataPrev;
			
			/* ----- Edit Byte Depending On The Mode ----- */
			if (mode == _GLCD_PRINT_MODE_MERGE)
			{
				data |= GLCD_BufferRead(GLCD.X, GLCD.Y);
			}
			
			/* ---------------- Send Byte ---------------- */
			GLCD_BufferWrite(GLCD.X++, GLCD.Y, data);
			
		}
		
		/* :::::::::::: Send An Empty Column Of 1px In The End :::::::::::: */
		if (GLCD.Font.Mode == _GLCD_PRINT_MODE_OVERWRITE)
		{
			data = _GLCD_WHITE;
		}
		else if (GLCD.Font.Mode == _GLCD_PRINT_MODE_MERGE)
		{
			data = GLCD_BufferRead(GLCD.X, GLCD.Y);
		}
		else
		{
			data = ~GLCD_BufferRead(GLCD.X, GLCD.Y);
		}
		
		GLCD_BufferWrite(GLCD.X++, GLCD.Y,data);
		
	}
	
	/* ~~~~~~ Goto The Upper-Right Corner Of The Printed Bitmap ~~~~~~ */
	GLCD_GotoXY(GLCD_GetX(), y_pos);
	
}

void GLCD_DrawLine(uint8_t startX, uint8_t startY, uint8_t endX, uint8_t endY, COLOR_TypeDef color)
{
	
	int8_t error   = 0;
	int8_t y_step  = 0;
	
	uint8_t deltax = 0;
	uint8_t deltay = 0;
	uint8_t x_pos  = 0;
	uint8_t y_pos  = 0;
	uint8_t slope  = 0;
	
	/* ~~~~~~~~~~~~~~~~~~~~~~~~~ Check Value ~~~~~~~~~~~~~~~~~~~~~~~~~ */
	if ((startX < _GLCD_SCREEN_WIDTH) && (endX < _GLCD_SCREEN_WIDTH) &&
	   (startY < _GLCD_SCREEN_HEIGHT) && (endY < _GLCD_SCREEN_HEIGHT))
	{
		
		/* :::::::::: Check Line Pos :::::::::: */
		if (startX == endX)
		{
			GLCD_DrawVLine(startY, endY, startX, color);
		}
		else if (startY == endY)
		{
			GLCD_DrawHLine(startX, endX, startY, color);
		}
		else
		{
			
			/* ------------------ Check Slope ------------------ */
			slope = ((__GLCD_AbsDiff(startY, endY) > __GLCD_AbsDiff(startX,endX)) ? 1 : 0);
			
			if (slope)
			{
				
				/* """""""""" Swap startX, startY """""""""" */
				__GLCD_Swap(startX, startY);
				
				/* """""""""""" Swap endX, endY """""""""""" */
				__GLCD_Swap(endX, endY);
				
			}
			
			if (startX > endX)
			{
				
				/* """"""""""" Swap startX, endX """"""""""" */
				__GLCD_Swap(startX, endX);
				
				/* """"""""""" Swap startY, endY """"""""""" */
				__GLCD_Swap(startY, endY);
				
			}
			
			/* ---------------- Calculate Param ---------------- */
			deltax  = endX - startX;
			deltay  = __GLCD_AbsDiff(endY, startY);
			error   = deltax / 2;
			y_pos   = startY;
			y_step  = ((startY < endY) ? 1 : -1);
			endX   += 1;
			
			/* ------------------ Set Pixels ------------------- */
			for (x_pos = startX ; x_pos < endX ; x_pos++)
			{
				
				if (slope)
				{
					GLCD_SetPixel(y_pos, x_pos, color);
				}
				else
				{
					GLCD_SetPixel(x_pos, y_pos, color);
				}
				
				error -= deltay;
				
				if (error < 0)
				{
					y_pos = y_pos + y_step;
					error = error + deltax;
				}
				
			}
			
		}
		
	}
	
}

void GLCD_DrawRectangle(const uint8_t startX, const uint8_t startY, const uint8_t endX, const uint8_t endY, COLOR_TypeDef color)
{
	
	if ((startX < _GLCD_SCREEN_WIDTH) && (endX < _GLCD_SCREEN_WIDTH) &&
	    (startY < _GLCD_SCREEN_HEIGHT) && (endY < _GLCD_SCREEN_HEIGHT))
	{
		
		GLCD_DrawHLine(startX, endX, startY, color);
		GLCD_DrawHLine(startX, endX, endY, color);
		GLCD_DrawVLine(startY, endY, startX, color);
		GLCD_DrawVLine(startY, endY, endX, color);
		
	}
	
}

void GLCD_DrawRoundRectangle(const uint8_t startX, const uint8_t startY, const uint8_t endX, const uint8_t endY, const uint8_t radius, COLOR_TypeDef color)
{
	
	if ((startX<_GLCD_SCREEN_WIDTH) && (endX<_GLCD_SCREEN_WIDTH) && (startY<_GLCD_SCREEN_HEIGHT) && (endY<_GLCD_SCREEN_HEIGHT))
	{
		
		uint8_t width;
		uint8_t height;
		uint8_t x;
		uint8_t y;
		
		/* ~~~~~~~~~~~~~~~~~~~~~ Calculate Parameter ~~~~~~~~~~~~~~~~~~~~~ */
		int16_t tSwitch = 3 - 2 * radius;
		
		width  = endX - startX;
		height = endY - startY;
		x      = 0;
		y      = radius;
		
		/* ~~~~~~~~~~~~~~~~~~~~~~~ Draw Perimeter ~~~~~~~~~~~~~~~~~~~~~~~~ */
		GLCD_DrawHLine(startX + radius, endX - radius, startY, color); // Top
		GLCD_DrawHLine(startX + radius, endX - radius, endY, color); // Bottom
		GLCD_DrawVLine(startY + radius, endY - radius, startX, color); // Left
		GLCD_DrawVLine(startY + radius, endY - radius, endX, color); // Right
		
		while (x <= y)
		{
			
			/* :::::::::::: Upper left corner :::::::::::: */
			GLCD_SetPixel(startX + radius - x, startY + radius - y, color);
			GLCD_SetPixel(startX + radius - y, startY + radius - x, color);

			/* :::::::::::: Upper right corner ::::::::::: */
			GLCD_SetPixel(startX + width - radius + x, startY + radius - y, color);
			GLCD_SetPixel(startX + width - radius + y, startY + radius - x, color);

			/* :::::::::::: Lower left corner :::::::::::: */
			GLCD_SetPixel(startX + radius - x, startY + height - radius + y, color);
			GLCD_SetPixel(startX + radius - y, startY + height - radius + x, color);

			/* :::::::::::: Lower right corner ::::::::::: */
			GLCD_SetPixel(startX + width - radius + x, startY + height - radius + y, color);
			GLCD_SetPixel(startX + width - radius + y, startY + height - radius + x, color);

			if (tSwitch < 0)
			{
				tSwitch += 4 * x + 6;
			}
			else
			{
				tSwitch += 4 * (x - y) + 10;
				y--;
			}
			
			x++;
			
		}
		
	}
	
}

void GLCD_DrawTriangle(const uint8_t x1, const uint8_t y1, const uint8_t x2, const uint8_t y2, const uint8_t x3, const uint8_t y3, COLOR_TypeDef color)
{
	
	if (((x1 < _GLCD_SCREEN_WIDTH) && (x2 < _GLCD_SCREEN_WIDTH) && (x3 < _GLCD_SCREEN_WIDTH) &&
		(y1 < _GLCD_SCREEN_HEIGHT) && (y2 < _GLCD_SCREEN_HEIGHT) && (y3 < _GLCD_SCREEN_HEIGHT)))
	{
		
		GLCD_DrawLine(x1, y1, x2, y2, color);
		GLCD_DrawLine(x2, y2, x3, y3, color);
		GLCD_DrawLine(x3, y3, x1, y1, color);
		
	}
	
}

void GLCD_DrawCircle(const uint8_t centerX, const uint8_t centerY, const uint8_t radius, COLOR_TypeDef color)
{
	if (((centerX + radius) < _GLCD_SCREEN_WIDTH) && ((centerY + radius) < _GLCD_SCREEN_HEIGHT))
	{
		
		uint8_t  x;
		uint8_t  y;
		
		uint16_t yChange;
		
		int16_t  xChange;
		int16_t  radiusError;
		
		/* ~~~~~~~~~~~~~~~~~~~~~ Calculate Parameter ~~~~~~~~~~~~~~~~~~~~~ */
		x           = radius;
		y           = 0;
		xChange     = 1 - 2 * radius;
		yChange     = 1;
		radiusError = 0;
		
		while (x >= y)
		{
			
			GLCD_SetPixel(centerX + x, centerY + y, color);
			GLCD_SetPixel(centerX - x, centerY + y, color);
			GLCD_SetPixel(centerX - x, centerY - y, color);
			GLCD_SetPixel(centerX + x, centerY - y, color);
			GLCD_SetPixel(centerX + y, centerY + x, color);
			GLCD_SetPixel(centerX - y, centerY + x, color);
			GLCD_SetPixel(centerX - y, centerY - x, color);
			GLCD_SetPixel(centerX + y, centerY - x, color);
			
			/* ::::::::::::::::::::::::::::::::::::::::::: */
			y++;
			radiusError += yChange;
			yChange     += 2;
			
			if ((2 * radiusError + xChange) > 0)
			{
				
				x--;
				radiusError += xChange;
				xChange     += 2;
				
			}
			
		}
		
	}
	
}

void GLCD_FillScreen(COLOR_TypeDef color)
{
	
	uint8_t i;
	uint8_t j;
	
	/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
	for (j = 0 ; j < _GLCD_SCREEN_HEIGHT ; j += _GLCD_SCREEN_LINE_HEIGHT)
	{
		
		for (i = 0 ; i < _GLCD_SCREEN_WIDTH ; i++)
		{
			GLCD_BufferWrite(i, j, color);
		}
		
	}
			
}

void GLCD_DrawFilledRectangle(const uint8_t startX, const uint8_t startY, const uint8_t endX, const uint8_t endY, COLOR_TypeDef color)
{
	GLCD_SetPixels(startX, startY, endX, endY, color);
}

void GLCD_DrawFilledRoundRectangle(const uint8_t startX, const uint8_t startY, const uint8_t endX, const uint8_t endY, const uint8_t radius, COLOR_TypeDef color)
{
	
	if ((startX < _GLCD_SCREEN_WIDTH) && (endX < _GLCD_SCREEN_WIDTH) && (startY < _GLCD_SCREEN_HEIGHT) && (endY < _GLCD_SCREEN_HEIGHT))
	{
		
		uint8_t width;
		uint8_t height;
		uint8_t x;
		uint8_t y;
		
		/* ~~~~~~~~~~~~~~~~~~~~~ Calculate Parameter ~~~~~~~~~~~~~~~~~~~~~ */
		int16_t tSwitch = 3 - 2 * radius;
		
		width  = endX - startX;
		height = endY - startY;
		x      = 0;
		y      = radius;
		
		/* ~~~~~~~~~~~~~~~~~~~~~~ Fill Center Block ~~~~~~~~~~~~~~~~~~~~~~ */
		GLCD_DrawFilledRectangle(startX + radius, startY, endX - radius, endY, color);
		
		while (x <= y)
		{
			
			/* :::::::::::::::: Left side :::::::::::::::: */
			GLCD_DrawLine(startX + radius - x, startY + radius - y, // Upper left corner upper half
			              startX + radius - x, startY + height - radius + y, // Lower left corner lower half
			              color);
						  
			GLCD_DrawLine(startX + radius - y, startY + radius - x, // Upper left corner lower half
			              startX + radius - y, startY + height - radius + x, // Lower left corner upper half
			              color);
						
			/* :::::::::::::::: Right side ::::::::::::::: */
			GLCD_DrawLine(startX + width - radius	+ x, startY + radius - y, // Upper right corner upper half
			              startX + width - radius + x, startY + height - radius + y, // Lower right corner lower half
			              color);
			
			GLCD_DrawLine(startX + width - radius + y, startY + radius - x, // Upper right corner lower half
			              startX + width - radius + y, startY + height - radius + x, // Lower right corner upper half
			              color);
			
			/* ::::::::::::::::::::::::::::::::::::::::::: */
			if (tSwitch < 0)
			{
				tSwitch += 4 * x +6;
			}
			else
			{
				tSwitch += 4 * (x - y) + 10;
				y--;
			}
			
			x++;
			
		}
		
	}
	
}

void GLCD_DrawFilledTriangle(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t x3, uint8_t y3, COLOR_TypeDef color)
{
	if (((x1 < _GLCD_SCREEN_WIDTH) && (x2 < _GLCD_SCREEN_WIDTH) && (x3 < _GLCD_SCREEN_WIDTH) &&
		(y1 < _GLCD_SCREEN_HEIGHT) && (y2 < _GLCD_SCREEN_HEIGHT) && (y3 < _GLCD_SCREEN_HEIGHT)))
	{
		
		uint8_t sl  = 0;
		uint8_t sx1 = 0;
		uint8_t sx2 = 0;
		
		double  m1  = 0;
		double  m2  = 0;
		double  m3  = 0;
		
		/* ~~~~~~~~~~~~~~~~~~~~~ Calculate Parameter ~~~~~~~~~~~~~~~~~~~~~ */	
		if (y2 > y3)
		{
			__GLCD_Swap(x2, x3);
			__GLCD_Swap(y2, y3);
		}
		
		if (y1 > y2)
		{
			__GLCD_Swap(x1, x2);
			__GLCD_Swap(y1, y2);
		}
		
		m1 = (double)(x1 - x2) / (y1 - y2);
		m2 = (double)(x2 - x3) / (y2 - y3);
		m3 = (double)(x3 - x1) / (y3 - y1);
		
		/* ~~~~~~~~~~~~~~~~~~~~~~~ Draw In Display ~~~~~~~~~~~~~~~~~~~~~~~ */
		for(sl = y1; sl <= y2; sl++)
		{
			
			/* :::::::::::::::: Calculate :::::::::::::::: */
			sx1 = m1 * (sl - y1) + x1;
			sx2 = m3 * (sl - y1) + x1;
			
			if (sx1 > sx2)
			{
				__GLCD_Swap(sx1, sx2);
			}
			
			/* :::::::::::::::: Draw line :::::::::::::::: */
			GLCD_DrawLine(sx1, sl, sx2, sl, color);
			
		}
		
		for (sl = y2; sl <= y3; sl++)
		{
			
			/* :::::::::::::::: Calculate :::::::::::::::: */
			sx1 = m2 * (sl - y3) + x3;
			sx2 = m3 * (sl - y1) + x1;
			
			if (sx1 > sx2)
			{
				__GLCD_Swap(sx1, sx2);
			}
			
			/* :::::::::::::::: Draw line :::::::::::::::: */
			GLCD_DrawLine(sx1, sl, sx2, sl, color);
			
		}
		
	}
	
}

void GLCD_DrawFilledCircle(const uint8_t centerX, const uint8_t centerY, const uint8_t radius, COLOR_TypeDef color)
{
	
	if (((centerX + radius) < _GLCD_SCREEN_WIDTH) && ((centerY + radius) < _GLCD_SCREEN_HEIGHT))
	{
		
		uint8_t x;
		uint8_t y;
		
		int8_t  f;
		int8_t  ddF_x;
		int8_t  ddF_y;
		
		/* ~~~~~~~~~~~~~~~~~~~~~ Calculate Parameter ~~~~~~~~~~~~~~~~~~~~~ */	
		f     = 1 - radius;
		ddF_x = 1;
		ddF_y = -2 * radius;
		x     = 0;
		y     = radius;
		
		/* ~~~~~~~~~~ Fill in the center between the two halves ~~~~~~~~~~ */
		GLCD_DrawLine(centerX, centerY - radius, centerX, centerY + radius, color);

		while(x < y)
		{
			
			//ddF_x = 2 * x + 1;
			//ddF_y = -2 * y;
			//f     = x * x + y * y - radius * radius + 2 * x - y + 1;
			
			/* :::::::::::::::: Calculate :::::::::::::::: */
			if (f >= 0)
			{
				
				y--;
				ddF_y += 2;
				f     += ddF_y;
				
			}
			
			x++;
			ddF_x += 2;
			f     += ddF_x;
			
			/* :::::::::::::::: Draw line :::::::::::::::: */
			/*
			  Now draw vertical lines between the points on the circle rather than
			  draw the points of the circle. This draws lines between the
			  perimeter points on the upper and lower quadrants of the 2 halves of the circle.
			*/
			GLCD_DrawVLine(centerY + y, centerY - y, centerX + x, color);
			GLCD_DrawVLine(centerY + y, centerY - y, centerX - x, color);
			GLCD_DrawVLine(centerY + x, centerY - x, centerX + y, color);
			GLCD_DrawVLine(centerY + x, centerY - x, centerX - y, color);
			
		}
		
	}
	
}

/* :::::::::::::::::::: Scroll :::::::::::::::::::: */
void GLCD_ScrollLeft(const uint8_t start, const uint8_t end)
{
	
	// The display is 16 rows tall. To scroll the whole display, run:
	GLCD_TransmitCommand(_GLCD_CMD_SCROLL_LEFT);
	GLCD_TransmitCommand(0x00); //Dummy

	GLCD_TransmitCommand(start); //Start
	GLCD_TransmitCommand(0x00); //Frames: 5
	GLCD_TransmitCommand(end); //End

	GLCD_TransmitCommand(0x00); //Dummy
	GLCD_TransmitCommand(UINT8_MAX); //Dummy
	GLCD_TransmitCommand(_GLCD_CMD_SCROLL_ACTIVE);
	
}

void GLCD_ScrollRight(const uint8_t start, const uint8_t end)
{
	
	// The display is 16 rows tall. To scroll the whole display, run:
	GLCD_TransmitCommand(_GLCD_CMD_SCROLL_RIGHT);
	GLCD_TransmitCommand(0x00); //Dummy

	GLCD_TransmitCommand(start); //Start
	GLCD_TransmitCommand(0x00); //Frames: 5
	GLCD_TransmitCommand(end); //End

	GLCD_TransmitCommand(0x00); //Dummy
	GLCD_TransmitCommand(UINT8_MAX); //Dummy
	GLCD_TransmitCommand(_GLCD_CMD_SCROLL_ACTIVE);
	
}

void GLCD_ScrollDiagonalLeft(const uint8_t start, const uint8_t end)
{
	
	// The display is 16 rows tall. To scroll the whole display, run:
	GLCD_TransmitCommand(_GLCD_CMD_SCROLL_VAREA_SET);
	GLCD_TransmitCommand(0x00);
	GLCD_TransmitCommand(_GLCD_SCREEN_HEIGHT);

	GLCD_TransmitCommand(_GLCD_CMD_SCROLL_VLEFT);
	GLCD_TransmitCommand(0x00); //Dummy
	GLCD_TransmitCommand(start); //Start
	GLCD_TransmitCommand(0x00); //Frames: 5
	GLCD_TransmitCommand(end); //End
	GLCD_TransmitCommand(0x01); //Vertical offset: 1

	GLCD_TransmitCommand(_GLCD_CMD_SCROLL_ACTIVE);
	
}

void GLCD_ScrollDiagonalRight(const uint8_t start, const uint8_t end)
{
	
	// The display is 16 rows tall. To scroll the whole display, run:
	GLCD_TransmitCommand(_GLCD_CMD_SCROLL_VAREA_SET);
	GLCD_TransmitCommand(0x00);
	GLCD_TransmitCommand(_GLCD_SCREEN_HEIGHT);

	GLCD_TransmitCommand(_GLCD_CMD_SCROLL_VRIGHT);
	GLCD_TransmitCommand(0x00); //Dummy
	GLCD_TransmitCommand(start); //Start
	GLCD_TransmitCommand(0x00); //Frames: 5
	GLCD_TransmitCommand(end); //End
	GLCD_TransmitCommand(0x01); //Vertical offset: 1

	GLCD_TransmitCommand(_GLCD_CMD_SCROLL_ACTIVE);
	
}

void GLCD_ScrollStop(void)
{
	GLCD_TransmitCommand(_GLCD_CMD_SCROLL_DEACTIVE);
}

/* ::::::::::::::::::::: Font ::::::::::::::::::::: */
void GLCD_SetFont(const uint8_t *name, const uint8_t width, const uint8_t height, GLCD_PrintModeTypeDef mode)
{
	
	if ((width < _GLCD_SCREEN_WIDTH) && (height < _GLCD_SCREEN_HEIGHT) && ((mode == _GLCD_PRINT_MODE_OVERWRITE) || (mode == _GLCD_PRINT_MODE_MERGE)))
	{
		
		/* ~~~~~~~~~~~~~~~~~ Change font pointer to new font ~~~~~~~~~~~~~~~~~ */
		GLCD.Font.Name = (uint8_t *)(name);
		
		/* ~~~~~~~~~~~~~~~~~~~~~~~~ Update font's size ~~~~~~~~~~~~~~~~~~~~~~~ */
		GLCD.Font.Width  = width;
		GLCD.Font.Height = height;
		
		/* ~~~ Update lines required for a character to be fully displayed ~~~ */
		GLCD.Font.Lines = (height - 1) / _GLCD_SCREEN_LINE_HEIGHT + 1;
		
		/* ~~~~~~~~~~~~~~~~~~~~~~~ Update blending mode ~~~~~~~~~~~~~~~~~~~~~~ */
		GLCD.Font.Mode = mode;
		
	}
	
}

/* :::::::::::::::::::::::::::::::::::::::::::::::: */
static void GLCD_BufferWrite(const uint8_t x, const uint8_t y, const uint8_t data)
{
	GLCD_Buffer[__GLCD_Pointer(x, y)] = data;
}

static uint8_t GLCD_BufferRead(const uint8_t x, const uint8_t y)
{
	// y >> 3 = y / 8
	return (GLCD_Buffer[__GLCD_Pointer(x, y)]);
}

static inline void GLCD_DrawHLine(uint8_t startX, uint8_t endX, const uint8_t y, COLOR_TypeDef color)
{
	
	if (startX > endX)
	{
		__GLCD_Swap(startX, endX);
	}
	
	GLCD_SetPixels(startX, y, endX, y, color);

}

static inline void GLCD_DrawVLine(uint8_t startY, uint8_t endY, const uint8_t x, COLOR_TypeDef color)
{
	
	if (startY > endY)
	{
		__GLCD_Swap(startY, endY);
	}

	GLCD_SetPixels(x, startY, x, endY, color);
	
}

static void Int2bcd(int32_t value, char BCD[])
{
	
	uint8_t indx       = 0;
	uint8_t isNegative = 0;
	uint8_t end        = 0;
	uint8_t offset     = 0;
	
	/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Reset to zero ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
	BCD[0] = BCD[1]  = BCD[2] =
	BCD[3] = BCD[4]  = BCD[5] =
	BCD[6] = BCD[7]  = BCD[8] =
	BCD[9] = BCD[10] = '0';
	
	/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Modify sign ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
	if (value < 0)
	{
		isNegative = 1;
		value      = -value;
	}
	
	/* ~~~~~~~~~~~~~~~~~~~~~~~ Calculate and set character ~~~~~~~~~~~~~~~~~~~~~~~ */
	while (value > 1000000000)
	{
		value -= 1000000000;
		BCD[1]++;
	}
	
	while (value >= 100000000)
	{
		value -= 100000000;
		BCD[2]++;
	}
		
	while (value >= 10000000)
	{
		value -= 10000000;
		BCD[3]++;
	}
	
	while (value >= 1000000)
	{
		value -= 1000000;
		BCD[4]++;
	}
	
	while (value >= 100000)
	{
		value -= 100000;
		BCD[5]++;
	}

	while (value >= 10000)
	{
		value -= 10000;
		BCD[6]++;
	}

	while (value >= 1000)
	{
		value -= 1000;
		BCD[7]++;
	}
	
	while (value >= 100)
	{
		value -= 100;
		BCD[8]++;
	}
	
	while (value >= 10)
	{
		value -= 10;
		BCD[9]++;
	}

	while (value >= 1)
	{
		value -= 1;
		BCD[10]++;
	}
	
	/* ~~~~~~~~~~~~~~~~~~~~~~~~ Find first non zero digit ~~~~~~~~~~~~~~~~~~~~~~~~ */
	while (BCD[indx] == '0')
	{
		indx++;
	}
	
	/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Add sign ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
	if (isNegative)
	{
		indx--;
		BCD[indx] = '-';
	}
	
	/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Shift array ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
	end    = 10 - indx;
	offset = indx;
	indx   = 0;
	
	while (indx <= end)
	{
		BCD[indx] = BCD[indx + offset];
		indx++;
	}
	
	BCD[indx] = '\0';
	
}
