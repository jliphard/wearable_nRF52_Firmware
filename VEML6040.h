
#ifndef MA_VEML6040_H_
#define MA_VEML6040_H_

    #include "I2C.h"
    #include "app_error.h"
    #include "nrf_delay.h"
    #include "SEGGER_RTT.h"
    
    void VEML6040_Turn_On( void );
    void VEML6040_Turn_Off( void );
    void VEML6040_Get_Data(uint16_t * dest);

#endif /* MA_VEML6040_ */
