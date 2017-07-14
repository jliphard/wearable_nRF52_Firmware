
#include "I2C.h"
#include "BME280.h"
#include "nrf_drv_twi.h"

#define NRF_LOG_MODULE_NAME "BME"

#define BME280_ADDRESS_1  0x76   // Address of BMP280 altimeter 1
//#define BME280_ADDRESS_2  0x77   // Address of BMP280 altimeter 2

// BME280 registers
#define BME280_ID         0xD0
#define BME280_HUM_LSB    0xFE
#define BME280_HUM_MSB    0xFD
#define BME280_TEMP_XLSB  0xFC
#define BME280_TEMP_LSB   0xFB
#define BME280_TEMP_MSB   0xFA
#define BME280_PRESS_XLSB 0xF9
#define BME280_PRESS_LSB  0xF8
#define BME280_PRESS_MSB  0xF7
#define BME280_CONFIG     0xF5
#define BME280_CTRL_MEAS  0xF4
#define BME280_STATUS     0xF3
#define BME280_CTRL_HUM   0xF2
#define BME280_RESET      0xE0
#define BME280_CALIB00    0x88
#define BME280_CALIB26    0xE1

enum Posr {P_OSR_00 = 0, /* no op */ P_OSR_01, P_OSR_02, P_OSR_04, P_OSR_08, P_OSR_16};
enum Hosr {H_OSR_00 = 0, /* no op */ H_OSR_01, H_OSR_02, H_OSR_04, H_OSR_08, H_OSR_16};
enum Tosr {T_OSR_00 = 0, /* no op */ T_OSR_01, T_OSR_02, T_OSR_04, T_OSR_08, T_OSR_16};
enum IIRFilter {full = 0,  /* bandwidth at full sample rate */ BW0_223ODR, BW0_092ODR, BW0_042ODR, BW0_021ODR /* bandwidth at 0.021 x sample rate */ };
enum Mode {BME280Sleep = 0, forced, forced2, normal};
enum SBy  {t_00_5ms = 0, t_62_5ms, t_125ms, t_250ms, t_500ms, t_1000ms, t_10ms, t_20ms};

// BME280 compensation parameters
uint8_t  dig_H1, dig_H3, dig_H6;
uint16_t dig_T1, dig_P1, dig_H4, dig_H5;
int16_t  dig_T2, dig_T3, dig_P2, dig_P3, dig_P4, dig_P5, dig_P6, dig_P7, dig_P8, dig_P9, dig_H2;

//uint32_t delt_t = 0, count = 0, sumCount = 0, slpcnt = 0;  // used to control display output rate

// Specify BME280 configuration
// set pressure and temperature output data rate
uint8_t Posr = P_OSR_16, Hosr = H_OSR_16, Tosr = T_OSR_02, Mode = normal, IIRFilter = BW0_021ODR, SBy = t_62_5ms; 

// * @brief Function for setting active
void BME280_Turn_On(void)
{
    //ret_code_t err_code;
    //NRF_LOG_DEBUG("BME_280_set_up(void)\r\n");
    
    uint8_t e = readByte(BME280_ADDRESS_1, BME280_ID);
    
    //NRF_LOG_DEBUG("BME_280_set_mode: readByte returned e: %d\r\n", e);
    //NRF_LOG_DEBUG("Should be 96 (HEX: 60)\r\n");
    NRF_LOG_FLUSH();
                   
    if(e == 0x60) {
        writeByte(BME280_ADDRESS_1, BME280_RESET, 0xB6); // reset BME280 before initialization   
        nrf_delay_ms(100);
        BME280_Configure( BME280_ADDRESS_1 ); // Initialize BME280 altimeter
        nrf_delay_ms(100);
        //NRF_LOG_DEBUG("BMP280 looks good\r\n");
    }
    else {
        //NRF_LOG_DEBUG("BMP280 not responding\r\n");
    }
    NRF_LOG_FLUSH();
}

void BME280_Configure( uint8_t address )
{
  // Configure the BME280
    
  // Set H oversampling rate
  writeByte(address, BME280_CTRL_HUM, 0x07 & Hosr);
  
  // Set T and P oversampling rates and sensor mode
  writeByte(address, BME280_CTRL_MEAS, Tosr << 5 | Posr << 2 | Mode);
  
  // Set standby time interval in normal mode and bandwidth
  writeByte(address, BME280_CONFIG, SBy << 5 | IIRFilter << 2);
  
  // Read and store calibration data
  uint8_t calib[26];
  
  readBytes(address, BME280_CALIB00, 26, &calib[0]);
  
  dig_T1 = (uint16_t)(((uint16_t) calib[ 1] << 8) | calib[ 0]);
  //NRF_LOG_DEBUG("BME280T1:%d\r\n",dig_T1);
  dig_T2 = ( int16_t)((( int16_t) calib[ 3] << 8) | calib[ 2]);
  //NRF_LOG_DEBUG("BME280T2:%d\r\n",dig_T2);
  dig_T3 = ( int16_t)((( int16_t) calib[ 5] << 8) | calib[ 4]);
  //NRF_LOG_DEBUG("BME280T3:%d\r\n",dig_T3);
  dig_P1 = (uint16_t)(((uint16_t) calib[ 7] << 8) | calib[ 6]);
  //NRF_LOG_DEBUG("BME280P1:%d\r\n",dig_P1);
  dig_P2 = ( int16_t)((( int16_t) calib[ 9] << 8) | calib[ 8]);
  dig_P3 = ( int16_t)((( int16_t) calib[11] << 8) | calib[10]);
  dig_P4 = ( int16_t)((( int16_t) calib[13] << 8) | calib[12]);
  dig_P5 = ( int16_t)((( int16_t) calib[15] << 8) | calib[14]);
  dig_P6 = ( int16_t)((( int16_t) calib[17] << 8) | calib[16]);
  dig_P7 = ( int16_t)((( int16_t) calib[19] << 8) | calib[18]);
  dig_P8 = ( int16_t)((( int16_t) calib[21] << 8) | calib[20]);
  dig_P9 = ( int16_t)((( int16_t) calib[23] << 8) | calib[22]);
  //NRF_LOG_DEBUG("BME280P9:%d\r\n",dig_P9);
  dig_H1 = calib[25];
  
  readBytes(address, BME280_CALIB26, 7, &calib[0]);
  
  dig_H2 = ( int16_t)((( int16_t) calib[1] << 8) | calib[0]);
 // NRF_LOG_DEBUG("BME280H2:%d\r\n",dig_H2);
  dig_H3 = calib[2];
  //NRF_LOG_DEBUG("BME280H3:%d\r\n",dig_H3);
  dig_H4 = ( int16_t)(((( int16_t) calib[3] << 8) | (0x0F & calib[4]) << 4) >> 4);
  //NRF_LOG_DEBUG("BME280H4:%d\r\n",dig_H4);
  dig_H5 = ( int16_t)(((( int16_t) calib[5] << 8) | (0xF0 & calib[4]) ) >> 4 );
  //NRF_LOG_DEBUG("BME280H5:%d\r\n",dig_H5);
  dig_H6 = calib[6];
  
  //NRF_LOG_DEBUG("BME280_Configure() completed.\r\n");
  //NRF_LOG_FLUSH();
}

void BME280_Read_PTH(int32_t * resultPTH)
{
  uint8_t rawData[9];  // 20-bit pressure register data stored here
  int32_t result[3];
  int32_t var1, var2, t_fine;
  
  readBytes(BME280_ADDRESS_1, BME280_PRESS_MSB, 9, &rawData[0]);  
  
  //Pressure
  result[0] = (int32_t) (((uint32_t) rawData[0] << 16 | (uint32_t) rawData[1] << 8 | rawData[2]) >> 4);
  //NRF_LOG_DEBUG("BME280P: %d\r\n", result[0]);
  
  //Temperature
  result[1] = (int32_t) (((uint32_t) rawData[3] << 16 | (uint32_t) rawData[4] << 8 | rawData[5]) >> 4);
  //NRF_LOG_DEBUG("BME280T: %d\r\n", result[1]);
   
  //Humidity
  result[2] = (int16_t) (((uint16_t) rawData[6] << 8 | rawData[7]) );
  //NRF_LOG_DEBUG("BME280H: %d\r\n", result[2]);
  
  //Need t_fine for all three compensations
  int32_t adc_T = result[1];
  
  var1 = ((((adc_T >> 3) - ((int32_t)dig_T1 << 1))) * ((int32_t)dig_T2)) >> 11;
  var2 = (((((adc_T >> 4) - ((int32_t)dig_T1)) * ((adc_T >> 4) - ((int32_t)dig_T1))) >> 12) * ((int32_t)dig_T3)) >> 14;
  
  t_fine = var1 + var2;
  
  resultPTH[0] = BME280_Compensate_P(result[0], t_fine);
  resultPTH[1] = BME280_Compensate_T(           t_fine);
  resultPTH[2] = BME280_Compensate_H(result[2], t_fine);
  
  //NRF_LOG_INFO("P: %d\r\n", resultPTH[0]);
  //NRF_LOG_INFO("T: %d\r\n", resultPTH[1]);
  //NRF_LOG_INFO("H: %d\r\n", resultPTH[2]);
  
  //NRF_LOG_FLUSH();
    
}  

// Returns humidity in %RH as unsigned 32 bit integer in Q22.10 format (22integer and 10fractional bits).
// Output value of “47445”represents 47445/1024= 46.333%RH
uint32_t BME280_Compensate_H(int32_t adc_H, int32_t t_fine)
{
  int32_t var;
  var = (t_fine - ((int32_t)76800));
  var = (((((adc_H << 14) - (((int32_t)dig_H4) << 20) - (((int32_t)dig_H5) * var)) +
    ((int32_t)16384)) >> 15) * (((((((var * ((int32_t)dig_H6)) >> 10) * (((var *
    ((int32_t)dig_H3)) >> 11) + ((int32_t)32768))) >> 10) + ((int32_t)2097152)) * ((int32_t)dig_H2) + 8192) >> 14));
  var = (var - (((((var >> 15) * (var >> 15)) >> 7) * ((int32_t)dig_H1)) >> 4));
  var = (var < 0 ? 0 : var); 
  var = (var > 419430400 ? 419430400 : var);
  return(uint32_t)(var >> 12);
}

// Returns temperature in DegC, resolution is 0.01 DegC. Output value of
// “5123” equals 51.23 DegC.
int32_t BME280_Compensate_T(int32_t t_fine)
{
  int32_t T;
  //var1 = ((((adc_T >> 3) - ((int32_t)dig_T1 << 1))) * ((int32_t)dig_T2)) >> 11;
  //var2 = (((((adc_T >> 4) - ((int32_t)dig_T1)) * ((adc_T >> 4) - ((int32_t)dig_T1))) >> 12) * ((int32_t)dig_T3)) >> 14;
  //t_fine = var1 + var2;
  T = (t_fine * 5 + 128) >> 8;
  return T;
}

// Returns pressure in Pa as unsigned 32 bit integer. Output value of “96386” equals 96386 Pa = 963.86 hPa
uint32_t BME280_Compensate_P(int32_t adc_P, int32_t t_fine) 
{
    int32_t var1, var2;
    uint32_t P;
    var1 = (t_fine>>1) - (int32_t)64000;
    var2 = (((var1>>2) * (var1>>2)) >> 11 ) * ((int32_t)dig_P6);
    var2 = var2 + ((var1*((int32_t)dig_P5))<<1);
    var2 = (var2>>2)+(((int32_t)dig_P4)<<16);
    var1 = (((dig_P3 * (((var1>>2) * (var1>>2)) >> 13 )) >> 3) + ((((int32_t)dig_P2) * var1)>>1))>>18; var1 =((((32768+var1))*((int32_t)dig_P1))>>15);
    if (var1 == 0) 
    {
        return 0; // avoid exception caused by division by zero 
    }
    P = (((uint32_t)(((int32_t)1048576)-adc_P)-(var2>>12)))*3125; 
    if (P < 0x80000000)
    {
        P = (P << 1) / ((uint32_t)var1); 
    }
    else
    {
        P = (P / (uint32_t)var1) * 2;
    }
    var1 = (((int32_t)dig_P9) * ((int32_t)(((P>>3) * (P>>3))>>13)))>>12; 
    var2 = (((int32_t)(P>>2)) * ((int32_t)dig_P8))>>13;
    P = (uint32_t)((int32_t)P + ((var1 + var2 + dig_P7) >> 4));
    return P;
}



