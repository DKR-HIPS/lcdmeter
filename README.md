# Lcd-Meter
<b>Arduino project - measuring temperature and humidity featuring LC display and simple webserver upload</b>

This project consists of two parts: an Arduino-based module to measure temperature and humidity and sending these data to a webserver, plus a simple server-side php webpage to display and archive the measurements.

<b>Hardware:</b>
The Lcd-Meter project uses an Arduino UNO or similar, together with an Ethernet shield (W5100 type), a DHT22 temperature/humidity sensor, a DS3231 realtime clock module, and a 16x2 LCD module. The latter two are connected by I2C, while the Ethernet module uses SPI and the DHT22 sensor is connected via 1-wire protocol.

<img src=lcdmeter_picture.jpg width=60%>

<b>Software:</b>
On the Arduino side the project implements mostly standard procedures to read from the sensor and realtime clock, output to the LCD and to create a simple web client in order to deliver the data to the webserver via HTTP GET requests. The Arduino code makes use of the libraries Wire.h, LiquidCrystal_I2C.h, DS3231.h, DHT.h, SPI.h, and Ethernet.h.
On the server side the data are received by sensor.php and written into .csv files. The system is desgined for simple use and therefore does not implement a database. Data can be viewed by navigating to index.php on the webserver. A few settings are available in settings.php and formatting is mostly specified in the style.css file.

A demonstration website - which can also be used for initial test-driving when you have assembled your own device - is available here: https://mackrug.de/lcdmeter

## How to get started

<b>Hardware setup:</b> Attach the Ethernet shield to the Arduino board, and wire all modules with +5V and GND. The I2C data wires from RTC and LCD modules are connected to pins A4 (SDA) and A5 (SCL). In addition, the sensor data pin is connected to Arduino pin 2, and a 10K resistor is needed as pull-up between this pin and +5V. Two buttons are optionally connected between Arduino pins 6 and 7 to GND.

<br><img src=LcdMeter_wiring.png width=75%>

<b>First startup:</b> Before loading the program code to the Arduino for the first time, adjust the SENDSPEED parameter (default: every 10 minutes) as you like. Also configure the Ethernet shield MAC address (has to be unique inside your network) and the static IP fallback address (has to match your local network settings). During reset (i.e. upon power-up) hold the "light" button for more than two seconds to enter the date/time setup function. This initializes the RTC module and it will keep the correct time from then on, provided that the module is equipped with battery power. 

<b>Server side installation:</b> Create a new directory on your php-enabled webserver, e.g. "lcdmeter", and copy the four files index.php, sensor.php, settings.php and style.css into this directory. Also create two subfolders named "files" and "deleted". The first one will later contain the .csv files with incoming data, and deleted files will be moved to the second folder. Make sure that access permissions are set correctly, so that php can read/write the directories and files as needed. You can then use your measurements archive like in the example provided (https://mackrug.de/lcdmeter). Remove the "example" notice four your own version and change whatever you need to work differently.

<b>Operation:</b> A working serverpath and security code is pre-configured already for testing purposes. For your own permament setup, make up a new 8-digit security code and enter it into the Arduino program (char security[] = "...";) as well as in the settings.php on the server ($code = '...' ;). You could also adjust the interval for sending data to the server (default is every 10 minutes), apply a positive or negative temperature correction value if needed, and set the times when the LCD backlight will autmatically switch on and off (default is 7 h and 23 h). The function of the two buttons is to manually trigger the sending of data (button on pin 6) and to toggle the LCD backlight on/off (button on pin 7). Note that the interval for reading new values from the sensor is 10 seconds, so that up-to-date values are always shown on the LCD. A small "arrow left" symbol will shortly blink in the top-right corner of the display whenever an actual measurement happens. If you hold down the "light"button during power-up for more than 2 seconds this will enter the date/time setup function. Adjust the date and time using the button (holding it will jump to the next setting). Use the "send" button to cancel date/time setup.

## Known issues

<b>Network connectivity:</b> In this project it is assumed that a stable network configuration is always available, so that the module can reach the webserver anytime using the DHCP-acquired or static IP address. That means, there is no particular network error handling in the code. If the network is not accessible, it may show "please wait" or "sending failed" for longer times and intervals between new measurements may be rather long because it attempts to connect to the network for extended timespans. If you need up-to-date messurement display under unsafe network conditions, you will have to add error-handling code for this. Activating the built-in watchdog of the Arduino module could be a possibility also.

<b>Realtime clock:</b> The DS3231 module does not handle daylight savings time. If you want the device to accurately reflect this, you'd have to power-down/power-up and access the date/time setup function to manually adjust the time on the device, using the "light" push button. Note that the sensor.php script always replaces the date/time received from the module with the current server date/time, so that inaccurate time values will not affect the recorded data. If this is unwanted, change the php code accordingly. On the hardware side there is a known issue with some modules having circuitry to charge the battery. If not using a re-chargeable battery, you need to deactive this (the common fix is to remove a tiny diode from the module by destroying it).

<b>Sensor accuracy:</b> The DHT22 sensor is specified for a precision of +/-0.5 °C. Please note, if you pack the hardware into a very compact case, that a small temperature increase inside the case which mainly comes from the Ethernet shield may cause the sensor to read unexpectedly higher values. Therefore, make sure that the sensor is placed outside and away from the case.

<b>Data storage:</b> A new .csv file is created for every day and files are not automatically deleted. Using the "delete" action will move them simply into the "deleted" folder - this is out of caution to not loose any measurements accidentially. Since the .csv files are very compact they are unlikely to fill-up webspace even after several years. However, if you want files to be deleted automatically after some time, you will have to add such functionality to the php scripts.

