/* Copyright (c) 2017, Stanford University
 * All rights reserved.
 * 
 * The point of contact for the MENTAID wearables dev team is 
 * Jan Liphardt (jan.liphardt@stanford.edu)
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of STANFORD UNIVERSITY nor the names of its contributors 
 *    may be used to endorse or promote products derived from this software 
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY STANFORD UNIVERSITY "AS IS" AND ANY EXPRESS 
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES 
 * OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL STANFORD UNIVERSITY OR ITS CONTRIBUTORS BE 
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE 
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) 
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT 
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT 
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef SPIFlash_h
#define SPIFlash_h

#ifdef __cplusplus
extern "C" {
#endif

#include <string.h>
#include "nrf_drv_spi.h"
#include "app_error.h"
  
void FLASH_Init( void );
void FLASH_Print_ID( void );
uint16_t FLASH_Get_First_Available_Location( void ); 
void FLASH_Write_Record( uint8_t wp[] );
uint8_t * FLASH_Page_Read( uint16_t pageN );
void FLASH_Line_Read( uint16_t lineN, uint8_t *line );
void FLASH_Erase( void );
void FLASH_Page_Write( uint16_t pageN, uint8_t *wp );
bool FLASH_Set_Write_Enable( void );
bool FLASH_Page_Is_Empty( uint16_t pageN );
int  page_to_address( int pn );
int  address_to_page( int addr );

#ifdef __cplusplus
}
#endif

#endif /* SPIFlash_h */

