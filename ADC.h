
#ifndef MA_ADC_H__
#define MA_ADC_H__

#ifdef __cplusplus
extern "C" {
#endif
    
    #include "nrf_drv_saadc.h"
    void ADC_init(void);
    int16_t Current_VBATT( void );
    void ADC_callback(nrf_drv_saadc_evt_t const * p_event);
    
#ifdef __cplusplus
}
#endif

#endif
