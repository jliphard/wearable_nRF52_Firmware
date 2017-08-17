
#ifndef MA_AD5593R_H_
#define MA_AD5593R_H_

    #include "I2C.h"
    #include "SEGGER_RTT.h"

    void  AD5593R_Turn_On( void );
    void  AD5593R_Configure( void );
    void  AD5593R_Get_Data( float * result) ;

#endif /* MA_AD5593R_H_ */
