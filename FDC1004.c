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
        
    CH_config[2] = 0x00; //we are not using the CAPDAC    
        
    CH_config[0] = 0x08;
    CH_config[1] = 0x1C;
    writeBytes( FDC1004_ADDRESS, CH_config, 3);
        
    CH_config[0] = 0x09;
    CH_config[1] = 0x3C;
    writeBytes( FDC1004_ADDRESS, CH_config, 3);
        
    CH_config[0] = 0x0A;
    CH_config[1] = 0x5C;
    writeBytes( FDC1004_ADDRESS, CH_config, 3);
        
    CH_config[0] = 0x0B;
    CH_config[1] = 0x7C;
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
//we should oversample by 2x, so 200 s/s probably ok
//0x5C = 0 000 01 0 1 = 100 s/s, repeat on
//0x04 = 0 000 01 0 0 = 100 s/s, repeat off
        
    CH_config[0] = 0x0C;
    CH_config[1] = 0x05; 

    CH_config[2] = 0xF0; //1111 0000 = all 4 channels

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
    SEGGER_RTT_printf(0, "FDC1004.1:%d %d %d\n", dM[0], dM[1], dL[0], dL[1]);
    SEGGER_RTT_printf(0, "FDC1004.1:%d\n", (int32_t)(result[0]*1000));
  }
    
  readBytes(FDC1004_ADDRESS, 0x02, dM, 2);
  readBytes(FDC1004_ADDRESS, 0x03, dL, 2); 
  result[1] = ConvertFDCToFloat( dM,  dL );
  
  if( SEGGER_FDC )
  {
    SEGGER_RTT_printf(0, "FDC1004.2:%d %d %d\n", dM[0], dM[1], dL[0], dL[1]);
    SEGGER_RTT_printf(0, "FDC1004.2:%d\n", (int32_t)(result[1]*1000));
  }
  
  readBytes(FDC1004_ADDRESS, 0x04, dM, 2);
  readBytes(FDC1004_ADDRESS, 0x05, dL, 2); 
  result[2] = ConvertFDCToFloat( dM,  dL );
  
  if( SEGGER_FDC )
  {
    SEGGER_RTT_printf(0, "FDC1004.3:%d %d %d\n", dM[0], dM[1], dL[0], dL[1]);
    SEGGER_RTT_printf(0, "FDC1004.3:%d\n", (int32_t)(result[2]*1000));
  }
  
  readBytes(FDC1004_ADDRESS, 0x06, dM, 2);
  readBytes(FDC1004_ADDRESS, 0x07, dL, 2); 
  result[3] = ConvertFDCToFloat( dM,  dL );
  
  if( SEGGER_FDC )
  {
    SEGGER_RTT_printf(0, "FDC1004.4:%d %d %d\n", dM[0], dM[1], dL[0], dL[1]);
    SEGGER_RTT_printf(0, "FDC1004.4:%d\n", (int32_t)(result[3]*1000));
  }   
  
}  



