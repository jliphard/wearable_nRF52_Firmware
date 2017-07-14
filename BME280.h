
#ifndef MA_BME280_H__
#define MA_BME280_H__

#ifdef __cplusplus
extern "C" {
#endif

    #include "app_error.h" 		//for APP_ERROR_CHECK(err_code);
    #include "nrf_log.h"   		//for NRF_LOG_DEBUG
    #include "nrf_delay.h"    	//for nrf_delay_ms
    #include "nrf_log_ctrl.h" 	//for NRF_LOG_FLUSH
    
	void BME280_Configure( uint8_t address );
	
	void BME280_Turn_On(void);
	
	void BME280_Read_PTH(int32_t * resultPTH);
	
	int32_t  BME280_Compensate_T(int32_t t_fine);
	uint32_t BME280_Compensate_P(int32_t adc_P, int32_t t_fine);
	uint32_t BME280_Compensate_H(int32_t adc_H, int32_t t_fine);


#ifdef __cplusplus
}
#endif

#endif /* UDA1380_H__ */
