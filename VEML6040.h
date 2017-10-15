/* Copyright (c) 2017, Stanford University
 * All rights reserved.
 * 
 * The point of contact for the MENTAID wearables dev team is 
 * Jan Liphardt (jan.liphardt@stanford.edu)
 * 
 * The code is modified from a reference implementation by Kris Winer
 * (tleracorp@gmail.com)
 * Copyright (c) 2017, Tlera Corporation
 * The license terms of the TLERACORP material are: 
 * "Library may be used freely and without limit with attribution."
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

#ifndef MA_VEML6040_H_
#define MA_VEML6040_H_

    #include "I2C.h"
    #include "app_error.h"
    #include "nrf_delay.h"
    #include "SEGGER_RTT.h"
    
    void VEML6040_Turn_On( void );
    void VEML6040_Turn_Off( void );
    void VEML6040_Get_Data(int16_t * dest);

#endif /* MA_VEML6040_ */
