
#include "ADC.h"

#define ADC_SAMPLES_IN_BUFFER   1
static nrf_saadc_value_t m_buffer[ADC_SAMPLES_IN_BUFFER];
float VBATT                     = 0.0;
static int16_t batteryVoltage   = 0;

/*
 5, // AIN3 (P0.05)
 2, // AIN0 (P0.02)
 VBAT = (127.0f/100.0f) * 3.30f * ((float)analogRead(VbatMon))/4095.0f; //actual battery voltage
 */

void ADC_init(void)
{
    ret_code_t err_code;
    
    nrf_saadc_channel_config_t channel_config =
    NRF_DRV_SAADC_DEFAULT_CHANNEL_CONFIG_SE(NRF_SAADC_INPUT_AIN4);
/*
    nrf_saadc_config_t adc_config;
    adc_config.resolution       = NRF_SAADC_RESOLUTION_8BIT;
    adc_config.buffer           = m_buffer;
    adc_config.buffer_size      = ADC_SAMPLES_IN_BUFFER;
    adc_config.oversample       = ;
  
    nrf_saadc_channel_config_t channel_0_config;
    channel_0_config.resistor_p = NRF_SAADC_RESISTOR_DISABLED;
    channel_0_config.resistor_n = NRF_SAADC_RESISTOR_DISABLED;
    channel_0_config.gain       = NRF_SAADC_GAIN1_6;
    channel_0_config.reference  = NRF_SAADC_REFERENCE_INTERNAL;
    channel_0_config.acq_time   = NRF_SAADC_ACQTIME_10US;
    channel_0_config.mode       = NRF_SAADC_MODE_SINGLE_ENDED;
    channel_0_config.pin_p      = NRF_SAADC_INPUT_VDD;
    channel_0_config.pin_n      = NRF_SAADC_INPUT_DISABLED;
*/    
 
    //channel_config.
    //nrf_saadc_resolution_set( NRF_SAADC_RESOLUTION_8BIT );
    //APP_ERROR_CHECK(err_code);
    //NRF_SAADC_RESOLUTION_12BIT 

    err_code = nrf_drv_saadc_init(NULL, ADC_callback);
    APP_ERROR_CHECK(err_code);
    
    err_code = nrf_drv_saadc_channel_init(0, &channel_config);
    APP_ERROR_CHECK(err_code);
    
    err_code = nrf_drv_saadc_buffer_convert(m_buffer, ADC_SAMPLES_IN_BUFFER);
    APP_ERROR_CHECK(err_code);
}

void ADC_callback(nrf_drv_saadc_evt_t const * p_event)
{
    if (p_event->type == NRF_DRV_SAADC_EVT_DONE)
    {
        ret_code_t err_code;
        
        err_code = nrf_drv_saadc_buffer_convert(p_event->data.done.p_buffer, ADC_SAMPLES_IN_BUFFER);
        APP_ERROR_CHECK(err_code);
        
        VBATT           = ((float)p_event->data.done.p_buffer[0]) / 4096.0f;
        //NRF_LOG_INFO("ADC returned R: %d\r\n", p_event->data.done.p_buffer[0]);
        VBATT           = VBATT * (139.0f/100.0f) * 3.30f;
        batteryVoltage  = (uint16_t)(VBATT * 100.0f);
    }
}

int16_t Current_VBATT( void )
{
    return batteryVoltage;
}
