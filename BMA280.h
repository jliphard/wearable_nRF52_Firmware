
#ifndef MA_BMA280_H_
#define MA_BMA280_H_

#ifdef __cplusplus
extern "C" {
#endif

    #include "I2C.h"
    #include "app_error.h"
    #include "nrf_delay.h"
    #include "SEGGER_RTT.h"
    
    void BMA280_Turn_On_Fast( void );
    void BMA280_Turn_On_Slow( void );
    void BMA280_Turn_Off( void );
    void BMA280_Get_Data(int16_t * dest);
    void BMA280_Calibrate( void );

#ifdef __cplusplus
}
#endif

#endif /* MA_BMA280_H_ */
