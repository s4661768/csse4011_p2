### Thingy52 Sensor and Bluetooth Publisher
This code allows the Thingy52 to read its inbuilt temperature, humidity, pressure, and air quality sensors. Once it reads in this data it mulitples the values by 10 or 100 to preserver decimals and then broadcasts them as uint16_t in the Major and Minor part of an iBeacon packet. 
