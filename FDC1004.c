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

/*
 Datasheet - note that TI datasheet has key typo/error re. single/repeated measurement setup.
 http://www.ti.com/lit/ds/symlink/fdc1004.pdf
*/

#include "FDC1004.h"

#define FDC1004_ADDRESS  0x50

// * @brief Function for setting active
void FDC1004_Turn_On(void)
{
    
    uint8_t devID[2];
    readBytes(FDC1004_ADDRESS, 0xFF, devID, 2);
    
    uint16_t devID_16;
    devID_16 = (uint16_t) devID[0] << 8 | devID[1];
            
    SEGGER_RTT_printf(0, "FDC1004 ID:%d Should be = 4100\n", devID_16);
                       
    if( devID_16 == 4100 ) FDC1004_Configure( );
        
}

void FDC1004_Configure( void )
{
    uint8_t CH_config[3];
        
    //CH_config[2] = 0x40; //= 6.4 pF
    //CH_config[2] = 0x20; //= 3.175 pF
    CH_config[2] = 0x00; //= 0 pF
        
    CH_config[0] = 0x08; //Measurement 1 Configuration
    CH_config[1] = 0x10; //000 100 00
    writeBytes( FDC1004_ADDRESS, CH_config, 3);
        
    CH_config[0] = 0x09; //Measurement 2 Configuration
    CH_config[1] = 0x30; //001 100 00
    writeBytes( FDC1004_ADDRESS, CH_config, 3);
        
    CH_config[0] = 0x0A; //Measurement 3 Configuration
    CH_config[1] = 0x50; //010 100 00
    writeBytes( FDC1004_ADDRESS, CH_config, 3);
        
    CH_config[0] = 0x0B; //Measurement 4 Configuration
    CH_config[1] = 0x70; //011 100 00
    writeBytes( FDC1004_ADDRESS, CH_config, 3);
        
/*
The FDC1004 can trigger a new measurement on the completion of the previous measurement (repeated measurements). 
This is setup by:
1. Setting REPEAT(Register0x0C:bit[8]) to 1.
2. Setting the corresponding MEAS_x field (Register0x0C:bit[7:4]) to 1.
When the FDC1004 is setup for repeated measurements, multiple configured measurements 
(up to a maximum of 4) can be performed in this manner, 
but Register 0x0C must be written in a single transaction.
*/ 
        
//note the the data sheet is wrong. 0540 is repeated measurement, not single measurement!
    
    CH_config[0] = 0x0C; // FDC Register Description
  //CH_config[1] = 0x11; // 0 00 10 0 0 1 // 200S/s, with repeat == 0x11
    CH_config[1] = 0x19; // 0 00 11 0 0 1 // 400S/s, with repeat == 0x19
    CH_config[2] = 0xF0; // 1111 0000 = all 4 channels

    //let's go!
    writeBytes( FDC1004_ADDRESS, CH_config, 3);
}

float ConvertFDCToFloat( uint8_t * dM,  uint8_t * dL )
{
    uint32_t resultR   = (uint32_t)dM[0] << 16 | (uint32_t)dM[1] << 8 | dL[0];
    int8_t   negative  = (resultR & (1 << 22)) != 0;
    int32_t  nativeInt = 0;

    if (negative)
        nativeInt = resultR | ~((1 << 23) - 1);
    else
        nativeInt = resultR;
  
    return ((float)nativeInt / 524288.0);
}

void FDC1004_Get_Data( float * result )
{

  uint8_t dM[2];
  uint8_t dL[2];
  
  readBytes(FDC1004_ADDRESS, 0x00, dM, 2);
  readBytes(FDC1004_ADDRESS, 0x01, dL, 2);
  result[0] = ConvertFDCToFloat( dM,  dL );

  if( SEGGER_FDC )
  {
    //SEGGER_RTT_printf(0, "FDC1004.1:%d %d %d\n", dM[0], dM[1], dL[0], dL[1]);
    SEGGER_RTT_printf(0, "FDC:%d ", (int32_t)(result[0]*1000));
  }
    
  readBytes(FDC1004_ADDRESS, 0x02, dM, 2);
  readBytes(FDC1004_ADDRESS, 0x03, dL, 2); 
  result[1] = ConvertFDCToFloat( dM,  dL );
  
  if( SEGGER_FDC )
  {
    //SEGGER_RTT_printf(0, "FDC1004.2:%d %d %d\n", dM[0], dM[1], dL[0], dL[1]);
    SEGGER_RTT_printf(0, "%d ", (int32_t)(result[1]*1000));
  }
  
  readBytes(FDC1004_ADDRESS, 0x04, dM, 2);
  readBytes(FDC1004_ADDRESS, 0x05, dL, 2); 
  result[2] = ConvertFDCToFloat( dM,  dL );
  
  if( SEGGER_FDC )
  {
    //SEGGER_RTT_printf(0, "FDC1004.3:%d %d %d\n", dM[0], dM[1], dL[0], dL[1]);
    SEGGER_RTT_printf(0, "%d ", (int32_t)(result[2]*1000));
  }
  
  readBytes(FDC1004_ADDRESS, 0x06, dM, 2);
  readBytes(FDC1004_ADDRESS, 0x07, dL, 2); 
  result[3] = ConvertFDCToFloat( dM,  dL );
  
  if( SEGGER_FDC )
  {
    //SEGGER_RTT_printf(0, "FDC1004.4:%d %d %d\n", dM[0], dM[1], dL[0], dL[1]);
    SEGGER_RTT_printf(0, "%d\n", (int32_t)(result[3]*1000));
  }   
  
}  



