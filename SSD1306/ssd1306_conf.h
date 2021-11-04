/*
------------------------------------------------------------------------------
~ File   : ssd1306_conf.h
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

#ifndef __SSD1306_CONF_H_
#define __SSD1306_CONF_H_

/* ~~~~~~~~~~~~~~ Required Headers ~~~~~~~~~~~~~ */
/* Driver-library for AVR */
//#include "i2c_unit.h"

/* Driver-library for STM32 */
#include "stm32_i2c.h"

/* ~~~~~~~~~~~~~~~~ SSD1306 I2C ~~~~~~~~~~~~~~~~ */
#define _SSD1306_I2C hi2c1

/* ~~~~~~~~~~~~~~~~~ GLCD Size ~~~~~~~~~~~~~~~~~ */
#define _GLCD_SIZE _GLCD_SIZE_128x64

/* ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */

#endif /* __SSD1306_CONF_H_ */
