
#ifndef MA_FDC1004_H_
#define MA_FDC1004_H_

    #include "I2C.h"
    #include "SEGGER_RTT.h"
    
    void  FDC1004_Configure( void );
    void  FDC1004_Turn_On( void );
    void  FDC1004_Get_Data( float * result) ;
    float ConvertFDCToFloat( uint8_t * dM,  uint8_t * dL );
    
#endif /* MA_FDC1004_H_ */
