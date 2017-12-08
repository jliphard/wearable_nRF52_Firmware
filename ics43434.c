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
 *
 * Parts of this software, primarily the basic BLE control code, were derived or 
 * directly copied from the Nordic reference implementations available in their SDK.  
 * For those sections, the following license applies:
 * 
 * Copyright (c) 2010 - 2017, Nordic Semiconductor ASA
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form, except as embedded into a Nordic
 *    Semiconductor ASA integrated circuit in a product or a software update for
 *    such product, must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 *
 * 3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * 4. This software, with or without modification, must only be used with a
 *    Nordic Semiconductor ASA integrated circuit.
 *
 * 5. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 *
 * THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdint.h>
#include <string.h>
#include "nordic_common.h"
#include "nrf.h"
#include "app_error.h"
#include "math.h"
#include "boards.h"
#include "app_timer.h"
#include "nrf_drv_clock.h"
#include "nrf_delay.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "app_pwm.h"
#include "nrf_drv_i2s.h"
#include "ics43434.h"
#include "SEGGER_RTT.h"

struct psd_bin_t
{
  uint16_t  freq;
  float32_t complex_mag;
};

static uint32_t           m_buffer_rx[I2S_BUFFER_SIZE];
static float32_t          m_fft_input_f32[I2S_BUFFER_SIZE];     // FFT input array. Time domain
static float32_t          m_fft_output_f32[I2S_BUFFER_SIZE/2];  // FFT output data; individual scan. Frequency domain
static struct psd_bin_t   psd_f32[I2S_BUFFER_SIZE/2];           // FFT output data; scan average. Frequency domain
static bool               m_error_encountered;                  // I2S data callback error status
static volatile bool      pwm_ready_flag;                       // A PWM ready status
static uint8_t            psd_avg_count = 0;                 // FFT scan counter for scan averaging

//We process every 20 datapackets - no idea how long that actually is.  
static uint8_t            psd_avg_limit = 10; //(uint8_t)sample_time*AUDIO_RATE/1024;
//20 = 1 full second of sampling
//each packet is 50 ms

static uint32_t           m_ifft_flag      = 0;                 // Flag that selects forward (0) or inverse (1) transform.
static uint32_t           m_do_bit_reverse = 1;                 // Flag that enables (1) or disables (0) bit reversal of output.

fft_results_t results;

void ICS_Get_Data(float * dest) {
    
    dest[0] = results.max;
    dest[1] = results.avg;
    dest[2] = results.freq[0];
    dest[3] = results.freq[1];
    dest[4] = results.freq[2];
    dest[5] = results.complex_mag[0];
    dest[6] = results.complex_mag[1];
    dest[7] = results.complex_mag[2];
    
}

APP_PWM_INSTANCE(PWM1,1); // Create the instance "PWM1" using TIMER1.
APP_PWM_INSTANCE(PWM2,2); // Create the instance "PWM2" using TIMER2.

static void pwm_ready_callback(uint32_t pwm_id)                                                            // PWM ready callback function
{
    pwm_ready_flag = true;
}

/**
 * @brief Function for processing generated sine wave samples.
 * @param[in] p_input        Pointer to input data array with complex number samples in time domain.
 * @param[in] p_input_struct Pointer to cfft instance structure describing input data.
 * @param[out] p_output      Pointer to processed data (bins) array in frequency domain.
 * @param[in] output_size    Processed data array size.
 */
static void fft_process(float32_t *                   p_input,
                        const arm_cfft_instance_f32 * p_input_struct,
                        float32_t *                   p_output,
                        uint16_t                      output_size)
{
    arm_cfft_f32(p_input_struct, p_input, m_ifft_flag, m_do_bit_reverse); // Use CFFT module to process the data
    arm_cmplx_mag_f32(p_input, p_output, output_size);                    // Calculate the magnitude at each bin using Complex Magnitude Module function
}

int psd_bin_comp(const void *elem1, const void *elem2) // PSD complex mag float comparison function for qsort
{
    struct psd_bin_t *e1 = (struct psd_bin_t *)elem1; // Cast pointers to the PSD data structure
    struct psd_bin_t *e2 = (struct psd_bin_t *)elem2; // Sort WRT the complex mag
    if(e1->complex_mag < e2->complex_mag)             // Returns -1 if elem1 < elem2
        return -1;
    return e1->complex_mag > e2->complex_mag;         // Returns 0 if elem1 = elem2 and 1 if elem1 > elem2
}

static bool copy_samples(uint32_t const * p_buffer, uint16_t number_of_words) // I2S data callback read and FFT buffer write function
{
    
    for(uint32_t i=0; i<(I2S_BUFFER_SIZE/2); i++)
    {
        m_fft_input_f32[2*i]       = (float32_t)((int32_t)p_buffer[i]);
	m_fft_input_f32[(2*i + 1)] = 0.0f;
    }
    
    //SEGGER_RTT_WriteString(0, "bool copy_samples\n");
    //SEGGER_RTT_printf(0, "samples: %d\n", psd_avg_count);
    //SEGGER_RTT_printf(0, "%f %f %f %f\r\n", m_fft_input_f32[0], m_fft_input_f32[2], m_fft_input_f32[4], m_fft_input_f32[6]);
  
    //here, we are adding new data
    fft_process(m_fft_input_f32, &arm_cfft_sR_f32_len1024, m_fft_output_f32, I2S_BUFFER_SIZE/2);
		
    /* Clear FPSCR register and clear pending FPU interrupts. This code is based on
    nRF5x_release_notes.txt in documentation folder. It is a necessary part of code when
    application using power saving mode and after handling FPU errors in polling mode. */
    __set_FPSCR(__get_FPSCR() & ~(FPU_EXCEPTION_MASK)); // Clear any exceptions generated the FFT analysis
            
    (void) __get_FPSCR();
            
    NVIC_ClearPendingIRQ(FPU_IRQn);
            
    for(uint32_t i=0; i<I2S_BUFFER_SIZE/4; i++)
    {
        psd_f32[i].complex_mag += m_fft_output_f32[i]; // Add new FFT value to the sum array element-by-element
    }
            
    psd_avg_count++;
            
    //ok - we have enough data - let's process it
    if(psd_avg_count >= psd_avg_limit)
    {
        //SEGGER_RTT_WriteString(0, "Let's process!\n");
        
        for(uint32_t i=0; i<I2S_BUFFER_SIZE/4; i++)
        {
            psd_f32[i].complex_mag /= (float)psd_avg_limit; // Divide PSD sum array element by the number of scans
        
            if(i > 0)
            {
                psd_f32[i].complex_mag *= 2.0f;
            }
        }
        
        psd_avg_count = 0;
  
        qsort(psd_f32, 512, sizeof(struct psd_bin_t), psd_bin_comp); // Sorts in ascending order of complex mag
        results.max = psd_f32[511].complex_mag;                      // Maximum is the last element in the sort
        results.avg = 0.0f;
    
        for(uint32_t i=0; i<512; i++)
        {
            results.avg += psd_f32[i].complex_mag;
        }
    
        results.avg /= 512.0f;
    
        for(uint8_t i=0; i<3; i++) // Load the four strongest PSD peaks into the results structure variable
        {
            results.freq[i] = (float)AUDIO_RATE*(float)psd_f32[511 - i].freq/1022.0f;
            results.complex_mag[i] = psd_f32[511 - i].complex_mag;
        }
    
        for(uint32_t i=0; i<512; i++) // Reset PSD results arrays for the next call
        {
            psd_f32[i].complex_mag = 0.0f;
            psd_f32[i].freq = i;
        }
        
        if ( SEGGER_MIC == 1 )
        {
            SEGGER_RTT_printf(0, "Audio max: %d avg: %d\n", (uint32_t)(results.max), (uint32_t)(results.avg));
            SEGGER_RTT_printf(0, "Audio f: %d %d %d\n", (uint32_t)(results.freq[0]), (uint32_t)(results.freq[1]), (uint32_t)(results.freq[2]));
            SEGGER_RTT_printf(0, "Audio m: %d %d %d\n", (uint32_t)(results.complex_mag[0]), (uint32_t)(results.complex_mag[1]), (uint32_t)(results.complex_mag[2]));
        }
        
        //we are done, turn system off.
        nrf_drv_i2s_stop();
        
    }    
  
  return true;
}

static void check_rx_data(uint32_t const * p_buffer, uint16_t number_of_words)
{

    if (!m_error_encountered)
    {
        m_error_encountered = !copy_samples(p_buffer, number_of_words);
    }
}

// This is the I2S data handler - all data exchange related to the I2S transfers is done here.
static void data_handler(uint32_t const * p_data_received,
                         uint32_t       * p_data_to_send,
                         uint16_t         number_of_words)
{
    // Non-NULL value in 'p_data_received' indicates that a new portion of
    // data has been received and should be processed.
    if (p_data_received != NULL)
    {
        check_rx_data(p_data_received, number_of_words);
    }

}

void ICS_Sample(void) 
{
    ret_code_t err_code;
    
    err_code = nrf_drv_i2s_start(m_buffer_rx, NULL, I2S_BUFFER_SIZE, 0); // RX only; "NULL" XMIT buffer
       
    APP_ERROR_CHECK(err_code);
    
    memset(m_buffer_rx, 0xCC, sizeof(m_buffer_rx)); // Initialize I2S data callback buffer
    
    for(uint32_t i=0; i<I2S_BUFFER_SIZE/2; i++)     // Initialize the PSD averaging array
    {
      psd_f32[i].complex_mag = 0.0f;
      psd_f32[i].freq        = i;
    }
    
    if ( SEGGER_MIC == 1 )
        SEGGER_RTT_WriteString(0, "Turned on mic - sampling now\n");
    
}

void ICS_Configure(void) 
{
    ret_code_t err_code;
    
    //Digital mic
    //Define the I2S configuration; running in slave mode and using PWM outputs to provide synthetic SCK and LRCK
    nrf_drv_i2s_config_t config = NRF_DRV_I2S_DEFAULT_CONFIG;
    config.sdin_pin             = I2S_SDIN_PIN;
    config.sdout_pin            = I2S_SDOUT_PIN;
    config.mode                 = NRF_I2S_MODE_SLAVE;
    config.mck_setup            = NRF_I2S_MCK_DISABLED;
    config.sample_width         = NRF_I2S_SWIDTH_24BIT;
    config.channels             = NRF_I2S_CHANNELS_LEFT;                                                   // Set the I2S microphone to output on the left channel
    config.format               = NRF_I2S_FORMAT_I2S;
    err_code                    = nrf_drv_i2s_init(&config, data_handler);                                 // Initialize the I2S driver
    
    APP_ERROR_CHECK(err_code);
	
    // 1-channel PWM; 16MHz clock and period set in ticks.
    // The user is responsible for selecting the periods to give the correct ratio for the I2S frame length
    app_pwm_config_t pwm1_cfg = APP_PWM_DEFAULT_CONFIG_1CH(16000000/(64*AUDIO_RATE), PWM_I2S_SCK_PIN);    // SCK; pick a convenient gpio pin
    app_pwm_config_t pwm2_cfg = APP_PWM_DEFAULT_CONFIG_1CH(16000000L/AUDIO_RATE, PWM_I2S_WS_PIN);         // LRCK; pick a convenient gpio pin. LRCK period = 64X SCK period

    // Initialize and enable PWM's
    err_code = app_pwm_ticks_init(&PWM1,&pwm1_cfg,pwm_ready_callback);
    APP_ERROR_CHECK(err_code);
    
    err_code = app_pwm_ticks_init(&PWM2,&pwm2_cfg,pwm_ready_callback);
    APP_ERROR_CHECK(err_code);
    
    app_pwm_enable(&PWM1);
    app_pwm_enable(&PWM2);
    
    app_pwm_channel_duty_set(&PWM1, 0, 50);                                                                // Set at 50% duty cycle for square wave
    app_pwm_channel_duty_set(&PWM2, 0, 50);
    
}