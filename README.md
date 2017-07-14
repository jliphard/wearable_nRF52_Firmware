# MENTAID

1/ Right now, code does not use the softdevice, since it's very hard to debug with the softdevice running.
2/ The ADC, I2C/TWI, BME280, and the SPI FLASH are functional and tested. 
3/ The code should also work with the softdevice/bluetooth, but i've been focussing on basic functionality.
4/ The goal right now is to have the device:

_Goal 1_ - Baseline/robust datalogging
*Sample all sensors once per min
*Write the data to flash in units of 256 bytes 
*Sleep for most of the time (i.e. between the sampling events)
*Do that until it drains the battery. 

_Goal 2_ - Upon system start, advertize BLE for 100 secs. If connect, then allow iOS BLE app to send commands, such as Erase Memory, or Upload all data. 

