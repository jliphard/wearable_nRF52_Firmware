
#ifndef MA_BME280_H_
#define MA_BME280_H_

#ifdef __cplusplus
extern "C" {
#endif

    #include "I2C.h"
    #include "app_error.h"
    #include "nrf_delay.h"
    #include "SEGGER_RTT.h"
    
    void BME280_Configure( uint8_t address );
    void BME280_Turn_On( void );
    void BME280_Read_PTH(int32_t * resultPTH);
    
    int32_t  BME280_Compensate_T(int32_t t_fine);
    uint32_t BME280_Compensate_P(int32_t adc_P, int32_t t_fine);
    uint32_t BME280_Compensate_H(int32_t adc_H, int32_t t_fine);

#ifdef __cplusplus
}
#endif

#endif /* MA_BME280_H_ */
