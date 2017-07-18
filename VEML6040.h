
#ifndef MA_VEML6040_H_
#define MA_VEML6040_H_

#ifdef __cplusplus
extern "C" {
#endif

    #include "I2C.h"
    #include "app_error.h"
    #include "nrf_delay.h"
    #include "SEGGER_RTT.h"
    

    
/*
  uint16_t getRGBWdata(int16_t * destination);
  void enableVEML6040(uint8_t IT);
  void disableVEML6040(uint8_t IT);
  void I2Cscan();
  void writeByte(uint8_t address, uint8_t subAddress, uint8_t data);
  uint8_t readByte(uint8_t address, uint8_t subAddress);
  void readBytes(uint8_t address, uint8_t subAddress, uint8_t count, uint8_t * dest);
*/
    //void BME280_Configure( uint8_t address );
    void VEML6040_Turn_On( void );
    void VEML6040_Turn_Off( void );
    void getRGBWdata(uint16_t * dest);
    //void BME280_Read_PTH(int32_t * resultPTH);
    
    //int32_t  BME280_Compensate_T(int32_t t_fine);
    //uint32_t BME280_Compensate_P(int32_t adc_P, int32_t t_fine);
    //uint32_t BME280_Compensate_H(int32_t adc_H, int32_t t_fine);

#ifdef __cplusplus
}
#endif

#endif /* MA_VEML6040_ */
