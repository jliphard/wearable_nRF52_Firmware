#include "VEML6040.h"

// http://www.mouser.com/pdfdocs/veml6040.PDF
////////////////////////////
// VEML6040 Command Codes //
////////////////////////////
#define  VEML6040_CONF            0x00 // command codes
#define  VEML6040_R_DATA          0x08  
#define  VEML6040_G_DATA          0x09 
#define  VEML6040_B_DATA          0x0A
#define  VEML6040_W_DATA          0x0B

#define VEML6040_ADDRESS          0x10

#define  IT_40   0  //   40 ms
#define  IT_80   1  //   80 ms
#define  IT_160  2  //  160 ms
#define  IT_320  3  //  320 ms
#define  IT_640  4  //  640 ms
#define  IT_1280 5  // 1280 ms

void VEML6040_Turn_On( void )
{
    uint8_t packet[3]; 
    packet[0] = (uint8_t)VEML6040_CONF;
    packet[1] = (uint8_t)IT_1280 << 4; // Bit 3 must be 0, bit 0 is 0 for run and 1 for shutdown, LS Byte
    packet[2] = (uint8_t)0x00;
    writeBytes(VEML6040_ADDRESS, packet, 3);
}   

void VEML6040_Turn_Off( void )
{
    uint8_t packet[3]; 
    packet[0] = (uint8_t)VEML6040_CONF;
    packet[1] = (uint8_t)(IT_1280 << 4 | 0x01); // Bit 3 must be 0, bit 0 is 0 for run and 1 for shutdown, LS Byte
    packet[2] = (uint8_t)0x00;
    writeBytes(VEML6040_ADDRESS, packet, 3);
}    

void VEML6040_Get_Data(uint16_t * dest)
{
    uint8_t rawData[2] = {0, 0};
    
    readBytes(VEML6040_ADDRESS, VEML6040_R_DATA, rawData, 2);  
    dest[0] = ((int16_t) rawData[1] << 8) | rawData[0];
    
    readBytes(VEML6040_ADDRESS, VEML6040_G_DATA, rawData, 2);  
    dest[1] = ((int16_t) rawData[1] << 8) | rawData[0];
    
    readBytes(VEML6040_ADDRESS, VEML6040_B_DATA, rawData, 2);  
    dest[2] = ((int16_t) rawData[1] << 8) | rawData[0];
    
    readBytes(VEML6040_ADDRESS, VEML6040_W_DATA, rawData, 2);  
    dest[3] = ((int16_t) rawData[1] << 8) | rawData[0];

    if( SEGGER_VEML )
        SEGGER_RTT_printf(0, "VEML6040:%d %d %d %d\n", dest[0], dest[1], dest[2], dest[3]);
}

