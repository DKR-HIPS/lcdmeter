/*********************

Lcd-Meter Version: 2022-01-03
Arduino for temperature and humidity measurement, display on LCD and send data to web server.
Parts: DHT22 sensor via 1-wire protocol, DS3231 realtime clock and 16x2 LC-display via I2C, 5100-type Ethernet shield via SPI 
(not using the SD card on the Ethernet module)

**********************/

#include <Wire.h>
#include <LiquidCrystal_I2C.h> 
#include <DS3231.h>
#include <DHT.h>
#include <SPI.h>
#include <Ethernet.h>

#define LCDADDR 0x27      // I2C-address for LCD module (e.g. joy-it brand used in the example project) 
#define DHTPIN 2          // The sensor is connected to PIN 2 for 1-wire protocol    
#define DHTTYPE DHT22     // Specify the DHT22 sensor type

#define LIGHTBUTTON 7     // Pin 7 input for button to toggle LCD backlight on/off
#define SENDBUTTON 6      // Pin 6 input for button to trigger data sending manually

#define NIGHT 23          // the hour when LCD backlight should automatically switch off
#define MORNING 7         // the hour when LCD backlight is automatically switched on

bool initclock = false;  // Initialize the RTC with initclock = true (one-time, afterwards set to false and upload to board again)
#define SETYEAR 21       // Enter date and time values to set RTC module and upload in the right moment...
#define SETMONTH 12
#define SETDAY 31
#define SETDOW 5
#define SETHOUR 15
#define SETMINUTE 30
#define SETSECOND 10

char server[] = "mackrug.de";     // Name of webserver, where the measurement data will be sent (note: a working test server is configured here)
char serverpath[] = "/lcdmeter/sensor.php";  // path to PHP script on webserver, which is called via http. Starts with "/"
char security[] = "8a7b6c5d";     // 8 hex digits security code for this device, used in the HTTP GET request and checked by PHP script on server
char marker = char('|');          // recognition sign "|" used in the 21-characters date/time response string "|Y-m-d|H:i:s|" from the server

int sendspeed = 10 ;               // Interval (in minutes) for sending data to the Webserver
float tempcorr = 0.0 ;            // This correction value is applied to measured temperature (default: 0.0)

// MAC address from label on the ethernet shield or simply made up (but unique inside network)
byte mac[] = { 0xA0, 0xB1, 0xC2, 0xD3, 0xE4, 0xF5 };
// fallback IP address to use if DHCP was not successful, must match your local network settings
IPAddress ip(192, 168, 1, 101);   

EthernetClient webclient;

LiquidCrystal_I2C lcd(LCDADDR, 16, 2);  // the 16x2 LCD module, called as "lcd"

DHT dht(DHTPIN, DHTTYPE);    // the sensor module called as "dht"

DS3231 myRTC;   // the RTC module called as "myRTC"
bool Century=false;
bool h12;
bool PM;

void setup() {

  pinMode(LIGHTBUTTON, INPUT_PULLUP);
  pinMode(SENDBUTTON, INPUT_PULLUP);
  
  Wire.begin();
  // Serial Monitor only for testing purposes if needed
  // Serial.begin(19200);

  // optional: intialize the RTC with new date/time (obviously makes no sense to do this on every reboot!)
  if(initclock == true)
  {
  myRTC.setClockMode(false);
  myRTC.setYear(SETYEAR);
  myRTC.setMonth(SETMONTH);
  myRTC.setDate(SETDAY);
  myRTC.setDoW(SETDOW);
  myRTC.setHour(SETHOUR);
  myRTC.setMinute(SETMINUTE);
  myRTC.setSecond(SETSECOND);
  }
  
  lcd.init();
  // switch LCD backlight on (lcd.noBacklight(); switches it off).
  lcd.backlight(); 
  lcd.print("Lcd-Meter    IP:");
  lcd.setCursor(0,1);
  lcd.print("please wait");

  dht.begin(); //DHT22 sensor start

  if (Ethernet.begin(mac) == false)   // try to get IP-address via DHCP
    {
     Ethernet.begin(mac, ip);     // if not successful, use the specified static fallback address
    }  
  delay(1000);
  
  // Serial.print("IP = ");
  // Serial.println(Ethernet.localIP());
  
  lcd.setCursor(0,1);
  lcd.print(Ethernet.localIP());
  delay(1000);  
}

String thisdate; // formatted date variable
String thistime; // formatted time variable
String lasttime;
float humidity;
float temperature;

bool displaylight = true;
bool lightswitch = true;
bool lbuttondown = false;
bool sbuttondown = false;
int timecounter = 1;
int sensorcounter = 1;
int displaycounter = 10;
int sendcounter = 1;

void loop() {

if(digitalRead(LIGHTBUTTON) == LOW && lbuttondown == false)    // when lightbutton is pressed
  {
    lbuttondown = true;
    if(displaylight == true)
      {
       displaylight = false;
       lcd.noBacklight();
      }
    else
      {
       displaylight = true;
       lcd.backlight();
      }
  }
else if(digitalRead(LIGHTBUTTON) == HIGH && lbuttondown == true)
  { lbuttondown = false; }

if(digitalRead(SENDBUTTON) == LOW && sbuttondown == false)    // when sendbutton is pressed
  {
   sbuttondown = true;
   displaycounter = 1;
   sendcounter = 1;
  }
else if(digitalRead(SENDBUTTON) == HIGH && sbuttondown == true)
  { sbuttondown = false; }

timecounter--;
if (timecounter < 1)
{
  timecounter = 10;   // 10 x 20 ms = ca. 0.2 second interval
  thistime = nice(myRTC.getHour(h12,PM))+":"+nice(myRTC.getMinute())+":"+nice(myRTC.getSecond());
}

if (thistime != lasttime)
{
  lasttime = thistime;
  thisdate = String(myRTC.getYear()+2000)+"-"+nice(myRTC.getMonth(Century))+"-"+nice(myRTC.getDate());
    
  displaycounter--;
  if (displaycounter > 5) 
   {
    lcd.setCursor(0,0);
    lcd.print(thisdate+"      ");
    lcd.setCursor(0,1);
    lcd.print(thistime+"        ");
   }
  else 
   {
    lcd.setCursor(0,0);
    lcd.print("Temp "+String(temperature)+" "+(char)223+"C   ");
    lcd.setCursor(0,1);
    lcd.print("Humidity "+String(humidity)+" %");
   }
    
  if (displaycounter < 1) 
   {
    // it will rotate between measurements and date/time display using a 10 seconds repeat time
    displaycounter = 10;
    Ethernet.maintain();

    // deactivate LCD backlight automatically at nighttime and activate in the morning
    if(lightswitch == true && myRTC.getHour(h12,PM) == NIGHT )
      {
       lightswitch = false;
       displaylight = false;
       lcd.noBacklight(); 
      }
    if(lightswitch == false && myRTC.getHour(h12,PM) == MORNING )
      {
       lightswitch = true;
       displaylight = true;
       lcd.backlight(); 
      }

    // interval for sending data to the webserver
    sendcounter--;
    if (sendcounter < 1)
     {
      sendcounter = 6 * sendspeed;
      lcd.clear();
      lcd.print("Sending to:");
      lcd.setCursor(0,1);
      if (webclient.connect(server,80))
       {
        lcd.print(webclient.remoteIP());
        // Serial.println("Sending HTTP GET request to Server");
        // Send the HTTP GET request, with parameters: s=security code, d=date from RTC, c=time from RTC, t=temperature, h=humidity
        webclient.println("GET "+String(serverpath)+"?s="+security+"&d="+thisdate+"&c="+thistime+"&t="+String(temperature)+"&h="+String(humidity)+" HTTP/1.1");
        webclient.println("Host: "+String(server));
        webclient.println("Connection: close");
        webclient.println();
       } 
      else 
       {
        // if no connection to webserver possible
        lcd.print("failed");
        // Serial.println("Server connection failed");
       }
      delay(500);
     }
  }  
 }

// interval to read new measurement from sensor, sensorcounter=500 means ca. every 10 seconds
sensorcounter--;
if (sensorcounter < 1)
  {
   sensorcounter = 500 ;
   lcd.setCursor(15,0);
   lcd.print( (char)127 );
   humidity = dht.readHumidity() ;         // read humidity
   delay(200);
   temperature = dht.readTemperature() + tempcorr ;   // read temperature and add the correction value
   lcd.setCursor(15,0);
   lcd.print(" ");
  }

// the basic loop delay is 20 milliseconds
delay(20);
}

// the function "nice" generates from int-number a string with leading zero
String nice(int thisvalue)
{
  String nicevalue;
  if(thisvalue<10){
    nicevalue = "0"+String(thisvalue);
  }
  else {
  nicevalue = String(thisvalue);
  }
  return nicevalue;
}
