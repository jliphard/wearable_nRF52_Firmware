# MENTAID firmware

This is the firmware for a Stanford University-funded wearable. The wearable collects information from about 10 sensors, logs this information, and can send it into the cloud if and when the wearer wishes to do so. The iOS code is also available, please see https://github.com/jliphard/wearable_nRF52_iOS. The code is under very active development, so expect things to change rapidly without notice. 

1/ The ADC, I2C/TWI, BME280, and the SPI FLASH are functional and tested. 

2/ The goal right now is to have the device:

_Goal 1_ - Baseline/robust datalogging
*Sample all sensors once per min
*Write the data to flash in units of 256 bytes 
*Sleep for most of the time (i.e. between the sampling events)
*Do that until it drains the battery. 

_Goal 2_ - Minimal BLE functionality. Upon system start, advertize BLE for 100 secs. If connect, then allow iOS BLE app to send commands, such as Erase Memory, or Upload all data. If not connect, just datalog as in use case 1. Once BLE connection drops, begin to datalog.

_Goal 3_ - Get time/date/velocity from iPhone once in while to facilitate data analysis. Key for the GPS is velocity, since that helps us to understand if the wearer is cycling, driving, walking and so forth. Out of privacy concerns, we do not save or transmit GPS lat/lon location data.
