
#ifndef MA_I2C_H__
#define MA_I2C_H__

#ifdef __cplusplus
extern "C" {
#endif

    #include "nrf_drv_twi.h"
    #include "app_error.h" //for APP_ERROR_CHECK(err_code);
    #include "SEGGER_RTT.h"

    void I2C_init( void );
    
    void writeByte( uint8_t address, uint8_t subAddress, uint8_t data);
    
    void writeBytes(uint8_t address, uint8_t * data, uint8_t n_bytes);
    
    void readBytes( uint8_t address, uint8_t subAddress, uint8_t * dest, uint8_t n_bytes);
    
    uint8_t readByte(uint8_t address, uint8_t subAddress);
    
    void I2C_handler(nrf_drv_twi_evt_t const * p_event, void * p_context);

#ifdef __cplusplus
}
#endif

#endif /* MA_I2C_H__ */
