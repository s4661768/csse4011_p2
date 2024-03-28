### Wireless Sensor Monitor --CSSE4011 Practical 2
Students built on functionality developed in the first assignment. For this second assignment student worked in pairs to read data and manage bluetooth, and display the received data. I managed the data reading and bluetooth side of the project. The three controllers used for this project were:
-	 NRF52840DK: The main processor that handled reading bluetooth from the the Thingy52, taking user input from the zephyr shell, and sending data to my team mates controller via UART connection.
-	Thing52: Used to read onboard temperature, humidity, pressure, and air quality sensors as well as broadcast the data using the iBeacon protocol of bluetooth.
-	NRF Dongle: Used as a packet sniffer in combination with WireShark
Zephyr was used as the RTOS for this project.
