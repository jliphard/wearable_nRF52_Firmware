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
Datasheet -
http://www.analog.com/media/en/technical-documentation/data-sheets/AD5593R.pdf
*/

#include "AD5593R.h"

#define AD5593R_ADDRESS  0x10

// * @brief Function for setting active
void AD5593R_Turn_On(void)
{
    
    //to confirm ID let's read back the pull down configuration register = default is 0x00FF
    //AD5593R
    //readback = 0111____
    //so command is
    //0111 0110 = 0x76
    //all other registers are 0 by default; only the PDCR has nonzero default useful for checcking
    //chip ID
    
    uint8_t devID[2];

    readBytes(AD5593R_ADDRESS, 0x76, devID, 2);

    uint16_t devID_16;
    
    devID_16 = (uint16_t) devID[0] << 8 | devID[1];
            
    SEGGER_RTT_printf(0, "AD5593R ID:%d Should be = 255\n", devID_16);
                 
    if( devID_16 == 255 )
        AD5593R_Configure();
        
}

void AD5593R_Configure( void )
{
    //Depends on what you are trying to do - thousands of choices
}

void AD5593R_Get_Data( float * result )
{
    //Depends on what you are trying to do - thousands of choices
}  



