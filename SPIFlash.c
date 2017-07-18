/* 
 * SPIFlash.cpp
 */

#include "SPIFlash.h"
#include "nrf_delay.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"

#define NRF_LOG_MODULE_NAME "FLASH"

#define CMD_PAGE_PROGRAM        ((uint8_t)0x02)
#define CMD_WRITE_ENABLE        ((uint8_t)0x06)
#define CMD_READ_DATA           ((uint8_t)0x03)
#define CMD_READ_ID             ((uint8_t)0x9F)
#define CMD_READ_STATUS_REG     ((uint8_t)0x05)

#define SPI_INSTANCE_ID   1
#define QUEUE_LENGTH     10

//will return either a 1 or 0 if the bit is enabled
#define CHECK_BIT(var,pos) (((var)>>(pos)) & 1)

static const nrf_drv_spi_t m_spi_master_1 = NRF_DRV_SPI_INSTANCE(1);

static uint8_t cmd[1] = { 0 };

static uint8_t tx4[4];

static uint8_t rx2[2];
static uint8_t rx4[4];
static uint8_t rx8[8];
static uint8_t rx256[256];

static uint8_t rxHalf[4+128];
static uint8_t txHalf[4+128];

#define APP_IRQ_PRIORITY_LOW 3  //overrides definition elsewhere

ret_code_t FLASH_Init( void )
{
    nrf_drv_spi_config_t const spi_config =
    {
        .sck_pin        = 7, 
        .mosi_pin       = 2,    
        .miso_pin       = 27,
        .ss_pin         = 26,
        .irq_priority   = APP_IRQ_PRIORITY_LOW,
        .orc            = 0xFF,
        .frequency      = NRF_DRV_SPI_FREQ_4M,
        .mode           = NRF_DRV_SPI_MODE_0,
        .bit_order      = NRF_DRV_SPI_BIT_ORDER_MSB_FIRST, 
    };
    
    ret_code_t err_code;
    
    err_code = nrf_drv_spi_init(&m_spi_master_1, &spi_config, NULL, NULL);
     
    if ( err_code != NRF_SUCCESS ) {
        //NRF_LOG_DEBUG("FLASH_Init Fail:%d\r\n", err_code);
    }
    
    return err_code;
}

ret_code_t FLASH_Get_ID( void )
{
    ret_code_t err_code;
    cmd[0] = CMD_READ_ID; //Get JEDEC ID
    memset(rx4, 0, sizeof(rx4));
    err_code = nrf_drv_spi_transfer(&m_spi_master_1, cmd, sizeof(cmd), rx4, sizeof(rx4));
    //NRF_LOG_DEBUG("MemID: %d Type: %d CAP: %d\r\n", rx4[1], rx4[2], rx4[3]); 
    return err_code;
}

uint8_t FLASH_Read_Status( void )
{
    memset(rx2, 0, sizeof(rx2));
    cmd[0] = CMD_READ_STATUS_REG;
    nrf_drv_spi_transfer(&m_spi_master_1, cmd, sizeof(cmd), rx2, sizeof(rx2));
    
    /* S0; 1 = busy
       S1; Write Enable Latch (WEL) is a read only bit in the status register 
       that is set to 1 after executing a Write Enable Instruction. 
       The WEL status bit is cleared to 0 when the device is write disabled
    */

    if ( CHECK_BIT(rx2[1],0) ) {  
        //NRF_LOG_DEBUG("SR: Busy\r\n")
    } else {
        //NRF_LOG_DEBUG("SR: Ready\r\n")
    };
   
    if ( CHECK_BIT(rx2[1],1) ) {
        //NRF_LOG_DEBUG("SR: WEL\r\n")
    } else {
        //NRF_LOG_DEBUG("SR: NoWEL\r\n")
    };
    
    return rx2[1];
}

bool FLASH_Is_Write_Enabled( void ) 
{
    memset(rx2, 0, sizeof(rx2));
    cmd[0] = CMD_READ_STATUS_REG;
    nrf_drv_spi_transfer(&m_spi_master_1, cmd, sizeof(cmd), rx2, sizeof(rx2));
    
    //bool retval = true;
    
    //S1 == 1 Write Enable Latch (WEL) is a read only bit in the status register 
    if ( CHECK_BIT(rx2[1],1) != 1) return false;
    //{
        
        //NRF_LOG_DEBUG("FLASH_Is_Write_Enabled: False\r\n"); 
    //}

    return true;
}

bool FLASH_Is_Busy( void ) 
{
    memset(rx2, 0, sizeof(rx2));
    cmd[0] = CMD_READ_STATUS_REG;
    nrf_drv_spi_transfer(&m_spi_master_1, cmd, sizeof(cmd), rx2, sizeof(rx2));
    
    //bool retval = false;
    
    if ( CHECK_BIT(rx2[1],0) == 1 ) return true;
    //{
    //    retval = true;
        //NRF_LOG_DEBUG("FLASH_Is_Busy!\r\n"); 
    //} 

    return false;
}

uint8_t * FLASH_Page_Read( uint16_t pageN )
{
  
  memset(   tx4, 0, sizeof(   tx4));
  memset( rx256, 0, sizeof( rx256));
  memset(rxHalf, 0, sizeof(rxHalf));
  
  if ( pageN >  4095 ) { //memory has 4095 pages 
      //NRF_LOG_DEBUG("FLASH_Page_Read: Out of bounds!\r\n")
      return rx256;
  };
  
  int address = page_to_address( pageN );
  
  //read the first 1/2
  tx4[0] = CMD_READ_DATA;
  tx4[1] = (address >> 16) & 0xFF;
  tx4[2] = (address >>  8) & 0xFF;
  tx4[3] =  address        & 0xFF;
  
  uint16_t i = 0;
  nrf_drv_spi_transfer(&m_spi_master_1, tx4, sizeof(tx4), rxHalf, sizeof(rxHalf));
  for(i =   0; i < 128; i++) { rx256[i] = rxHalf[i+4]; };  
  
  tx4[3] = 128;
  memset(rxHalf, 0, sizeof(rxHalf));
  
  nrf_drv_spi_transfer(&m_spi_master_1, tx4, sizeof(tx4), rxHalf, sizeof(rxHalf));
  for(i = 128; i < 256; i++) { rx256[i] = rxHalf[(i-128)+4]; };  
  
  NRF_LOG_HEXDUMP_DEBUG((uint8_t *)rx256, 256);
  
  return rx256;

}

void FLASH_Page_WriteTest( uint16_t pageN )
{
    
  //memory has 0-4095 pages, and each page has length 256  
        
  if ( pageN > 4095 ) { //memory has 4095 pages 
      //NRF_LOG_DEBUG("FLASH_Page_Write: Out of bounds!\r\n")
      return;
  };
    
  if ( !FLASH_Set_Write_Enable() ) {
      //NRF_LOG_DEBUG("FLASH_Page_Write: Write not enabled!\r\n")
      return;
  };
  
  if ( FLASH_Is_Busy() ) {
      //NRF_LOG_DEBUG("FLASH_Page_Write: Busy!\r\n")
      return;
  };
  
  if ( FLASH_Page_Is_Empty( pageN) == false ) {
      //NRF_LOG_DEBUG("FLASH_Page_Write: Page already contains data!\r\n")
      return;
  };
 
  //NRF_LOG_DEBUG("Write looking good\r\n");
 
  //the command and address
  int address = page_to_address( pageN );
  
  memset(txHalf, 0, sizeof(txHalf));
  
  txHalf[0] = CMD_PAGE_PROGRAM;
  txHalf[1] = (address >> 16) & 0xFF;
  txHalf[2] = (address >> 8)  & 0xFF;
  txHalf[3] =  address        & 0xFF;

  uint16_t i = 0;
  
  //write the first 128 bytes
  //NRF_LOG_DEBUG("Write first part\r\n")
  for( i = 4; i < (128+4); i++ ) { txHalf[i] = i-4; };  
  nrf_drv_spi_transfer(&m_spi_master_1, txHalf, sizeof(txHalf), NULL, 0);
  
  if ( !FLASH_Set_Write_Enable() ) {
    //NRF_LOG_DEBUG("FLASH_Page_Write: Write not enabled!\r\n")
    return;
  };
  
  //write the next 128 bytes
  txHalf[3] = 128;
  //NRF_LOG_DEBUG("Write second part\r\n")
  for( i = 4; i < (128+4); i++ ) { txHalf[i] = i-4; };  
  nrf_drv_spi_transfer(&m_spi_master_1, txHalf, sizeof(txHalf), NULL, 0);
  
};
 
void FLASH_Page_Write( uint16_t pageN, uint8_t *wp )
{
    
  //memory has 0-4095 pages, and each page has length 256    
  if ( pageN > 4095 ) {
      //NRF_LOG_DEBUG("FLASH_Page_Write: Out of bounds!\r\n")
      return;
  };
    
  if ( !FLASH_Set_Write_Enable() ) {
      //NRF_LOG_DEBUG("FLASH_Page_Write: Write not enabled!\r\n")
      return;
  };
  
  if ( FLASH_Is_Busy() ) {
      //NRF_LOG_DEBUG("FLASH_Page_Write: Busy!\r\n")
      return;
  };
  
  if ( FLASH_Page_Is_Empty( pageN) == false ) {
      //NRF_LOG_DEBUG("FLASH_Page_Write: Page already contains data!\r\n")
      return;
  };
 
  //NRF_LOG_DEBUG("Write looking good\r\n");
 
  //the command and address
  int address = page_to_address( pageN );
  
  memset(txHalf, 0, sizeof(txHalf));
  
  txHalf[0] = CMD_PAGE_PROGRAM;
  txHalf[1] = (address >> 16) & 0xFF;
  txHalf[2] = (address >> 8)  & 0xFF;
  txHalf[3] =  address        & 0xFF;

  uint16_t i = 0;
  
  //write the first 128 bytes
  //NRF_LOG_DEBUG("Write first part\r\n")
  for( i = 4; i < (128+4); i++ ) { txHalf[i] = wp[i-4]; };
  //NRF_LOG_HEXDUMP_DEBUG((uint8_t *)txHalf, 128+4);
  nrf_drv_spi_transfer(&m_spi_master_1, txHalf, sizeof(txHalf), NULL, 0);
 
  //wait for at most 200 ms to make sure write completes
  for( i = 0; i < 10; i++ ) {
      nrf_delay_ms(20); 
      if ( FLASH_Is_Busy() ) {
          //NRF_LOG_DEBUG("Erase: FLASH is still busy\r\n");
      } else {
          break;   
      }
  }

  //wait for at most 200 ms to make sure we are good for next write
  for( i = 0; i < 10; i++ ) {
      nrf_delay_ms(20); 
      if ( FLASH_Set_Write_Enable() ) {
          break;
      } else {
          //NRF_LOG_DEBUG("Erase: Not able to write\r\n");
      }
  }
  
  //write the next 128 bytes
  txHalf[3] = 128;
  //NRF_LOG_DEBUG("Write second part\r\n")
  for( i = 4; i < (128+4); i++ ) { txHalf[i] = wp[i-4+128]; };
  //NRF_LOG_HEXDUMP_DEBUG((uint8_t *)txHalf, 128+4);
  nrf_drv_spi_transfer(&m_spi_master_1, txHalf, sizeof(txHalf), NULL, 0);
  
};

bool FLASH_Page_Is_Empty( uint16_t pageN )
{

  if ( pageN > 4095 ) { //memory has 4096 pages 
      //NRF_LOG_DEBUG("FLASH_PE: Out of bounds!\r\n")
      return false; //err on the side of safety
  };
  
  memset(tx4, 0, sizeof(tx4));
  memset(rx8, 0, sizeof(rx8));
  
  int address = page_to_address( pageN );
  
  tx4[0] = CMD_READ_DATA;
  tx4[1] = (address >> 16) & 0xFF;
  tx4[2] = (address >>  8) & 0xFF;
  tx4[3] =  address        & 0xFF;
  
  //NRF_LOG_HEXDUMP_DEBUG((uint8_t *)send4, 4);
  nrf_drv_spi_transfer(&m_spi_master_1, tx4, sizeof(tx4), rx8, sizeof(rx8));
       
  uint16_t check = rx8[4] + rx8[5] + rx8[6] + rx8[7];
  //empty = 255, so 255+255+255+255 = 1020
  //NRF_LOG_INFO("FLASH_PE Page:%d contains:%d\r\n", pageN, check);
  //NRF_LOG_HEXDUMP_DEBUG((uint8_t *)rx8, 8);
  
  if ( check == 1020 ) {
    //NRF_LOG_DEBUG("FLASH_PE: Page:%d is empty.\r\n", pageN);
    return true;
  } else {
    //NRF_LOG_DEBUG("FLASH_PE: Page:%d contains:%d\r\n", pageN, check);
    return false;
  }
  
}

uint16_t FLASH_Get_First_Available_Location( void )
{
   //this assumes that data were written contiguously
   uint16_t ds; 
        
   for(ds = 0; ds < 4096; ds++)
   {
       //NRF_LOG_DEBUG("Testing page %d\r\n", ds);
       
       if( FLASH_Page_Is_Empty( ds ) )
       {
          //NRF_LOG_DEBUG("Page %d is empty.\r\n", ds);
          return ds;
       } else {
          //keep searching 
          //NRF_LOG_DEBUG("Page %d is full.\r\n", ds);
       };
   };
   
   //should never really get here 
   //NRF_LOG_DEBUG("No memory available!\r\n", ds);
   return 4096;
}

//=====================================
// convert a page number to a 24-bit address
int page_to_address(int pn) 
{
  return(pn << 8);
}

//=====================================
// convert a 24-bit address to a page number
int address_to_page(int addr) 
{
  return(addr >> 8);
}

//=====================================
void FLASH_Reset( void )
{
    cmd[0] = 0xF0;
    nrf_drv_spi_transfer(&m_spi_master_1, cmd, sizeof(cmd), NULL, 0); 
}

//=====================================
void FLASH_Erase( void )
{
  if( !FLASH_Set_Write_Enable() ) {
      //NRF_LOG_DEBUG("Erase: Write not enabled\r\n");
  }
  
  if ( FLASH_Is_Busy() ) {
      //NRF_LOG_DEBUG("Erase: FLASH is busy\r\n");
  }
  
  NRF_LOG_DEBUG("Erase: Erasing Flash\r\n");
  
  cmd[0] = 0xC7;
  nrf_drv_spi_transfer(&m_spi_master_1, cmd, sizeof(cmd), NULL, 0); 
  
  //wait for some
  while ( FLASH_Is_Busy() ) {
      nrf_delay_ms(1500);
      //NRF_LOG_DEBUG("Erase: FLASH is still busy\r\n");
  }
}

//=====================================
bool FLASH_Set_Write_Enable( void )
{
  bool enabled = false;
  cmd[0] = CMD_WRITE_ENABLE;
  nrf_drv_spi_transfer(&m_spi_master_1, cmd, sizeof(cmd), NULL, 0);
  
  if( FLASH_Is_Write_Enabled() ) {
    enabled = true;
  } else {
    enabled = false;
    //NRF_LOG_DEBUG("FLASH_Set_Write_Enable Fail\r\n");
  };
    
  return enabled;
}