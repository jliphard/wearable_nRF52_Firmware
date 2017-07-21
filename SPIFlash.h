/* SPIFlash.h
Sketch by Kris Winer December 16. 2016

License: Use this sketch any way you choose; if you like it, buy me a beer sometime

Purpose: Checks function of a variety of SPI NOR flash memory chips hosted by the STM32L4
Dragonfly (STM32L476), Butterfly (STM32L433), and Ladybug (STML432) development boards or their variants.

Sketch takes advantage of the SPI.beginTransaction/SPI.EndTransaction protocol for efficiency
and maximum speed.

Sketch based on the work of Pete (El Supremo) as follows:
 * Copyright (c) 2014, Pete (El Supremo), el_supremo@shaw.ca
 *
 * Development of this audio library was funded by PJRC.COM, LLC by sales of
 * Teensy and Audio Adaptor boards.  Please support PJRC's efforts to develop
 * open source software by purchasing Teensy or other PJRC products.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice, development funding notice, and this permission
 * notice shall be included in all copies or substantial portions of the Software.
 */

#ifndef SPIFlash_h
#define SPIFlash_h

#ifdef __cplusplus
extern "C" {
#endif

#include "nrf_drv_spi.h"
#include "app_error.h"
  
void FLASH_Init( void );
void FLASH_Print_ID( void );
uint16_t FLASH_Get_First_Available_Location( void ); 
void FLASH_Write_Record( uint8_t wp[] );
uint8_t * FLASH_Page_Read( uint16_t pageN );
void FLASH_Page_Write( uint16_t pageN, uint8_t *wp );
bool FLASH_Set_Write_Enable( void );
bool FLASH_Page_Is_Empty( uint16_t pageN );
int  page_to_address( int pn );
int  address_to_page( int addr );

#ifdef __cplusplus
}
#endif

#endif /* SPIFlash_h */

