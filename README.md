# lcdmeter
Arduino project - measuring temperature and humidity featuring LC display and simple webserver upload

This project consists of two parts: an Arduino-based module to measure temperature and humidity and sending these data to a webserver, plus a simple server-side php webpage to display and archive the meaurements.

Hardware:
The Lcd-Meter project uses an Arduino UNO or similar, together with an Ethernet shield (W5100 type), a DHT22 temperature/humidity sensor, a DS3231 realtime clock module, and a 16x2 LCD module. The latter two are connected by I2C, while the Ethernet module uses SPI and the DHT22 sensor is connected via 1-wire protocol.

Software:
On the Arduino side the project makes use mostly of standard procedures to read from the sensor and realtime clock, output to the LCD and to create a simple web client in order to deliver the data to the webserver via HTTP GET requests. The Arduino code makes use of the libraries Wire.h, LiquidCrystal_I2C.h, DS3231.h, DHT.h, SPI.h, and Ethernet.h.
On the server side the data are received by sensor.php and written into .csv files. The system is desgined for simple use and therefore does not implement a database. Data can be viewed by navigating to index.php on the webserver. A few settings are available in settings.php and formatting is mostly specified in the style.css file.
