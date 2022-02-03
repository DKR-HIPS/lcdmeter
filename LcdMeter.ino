/*********************

Lcd-Meter Version: 2022-02-03 v1.2.0
Arduino for temperature and humidity measurement, display on LCD and send data to web server.
Parts: DHT22 sensor via 1-wire protocol, DS3231 realtime clock and 16x2 LC-display via I2C, 5100-type Ethernet shield via SPI 
(not using the SD card on the Ethernet module). 
Option to use the built-in watchdog functionality (8 seconds timeout). One-button date/time setup.

**********************/

#include <Wire.h>
#include <LiquidCrystal_I2C.h> 
#include <DS3231.h>
#include <DHT.h>
#include <SPI.h>
#include <Ethernet.h>
#include <avr/wdt.h>

#define LCDADDR 0x27      // I2C-address for LCD module (e.g. joy-it brand used in the example project) 
#define DHTPIN 2          // The sensor is connected to PIN 2 for 1-wire protocol    
#define DHTTYPE DHT22     // Specify the DHT22 sensor type

#define LIGHTBUTTON 7     // Pin 7 input for button to toggle LCD backlight on/off
#define SENDBUTTON 6      // Pin 6 input for button to trigger data sending manually

#define NIGHT 22          // the hour when LCD backlight should automatically switch off
#define MORNING 7         // the hour when LCD backlight is automatically switched on

#define SENDSPEED 10      // Interval (in minutes) for sending data to the Webserver
#define AUTORESET 0       // time (in minutes) after which it will force a reset regularly (0=disable)

char server[] = "mackrug.de";     // Name of webserver, where the measurement data will be sent (note: a working test server is configured here)
char serverpath[] = "/lcdmeter/sensor.php";  // path to PHP script on webserver, which is called via http. Starts with "/"
char security[] = "8a7b6c5d";     // 8 hex digits security code for this device, used in the HTTP GET request and checked by PHP script on server
char marker = char('|');          // recognition sign "|" used in the 21-characters date/time response string "|Y-m-d|H:i:s|" from the server
bool trydhcp = true;         // true = try DHCP first to obtain an IP address
bool usewatchdog = true;     // true = activate the Watchdog functionality (triggered if program hangs for more than 8 seconds)

float tempcorr = 0.0 ;       // This correction value is applied to measured temperature (default: 0.0)

// MAC address from label on the ethernet shield or simply made up (but unique inside network)
byte mac[] = { 0xA0, 0xB1, 0xC2, 0xD3, 0xE4, 0xF5 };
// fallback IP address to use if DHCP was not successful, must match your local network settings
IPAddress ip(192, 168, 0, 101);   

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
  // Serial.begin(19200);
  
  lcd.init();
  // switch the LCD backlight on
  lcd.backlight();
  lcd.clear();
  lcd.print("Lcd-Meter");

  // check if the lightbutton is being held down during power-up for more than 2 seconds
  if(digitalRead(LIGHTBUTTON) == LOW) 
  { 
  lcd.setCursor(0,1);
  lcd.print("Set date/time ? ");
  int setdcount = 0;
   while(digitalRead(LIGHTBUTTON) == LOW)
    {
     setdcount++;
     delay(20);
     if(setdcount == 100)
       {
        lcd.setCursor(14,1);
        lcd.print(">>");
       }
    }
   if(setdcount > 100)
    {
     setd();   //call the function to set date/time in the RTC after button is released
    }
  }
  
  lcd.setCursor(0,1);
  lcd.print("IP: trying DHCP ");

  dht.begin(); //DHT22 sensor start

  bool dhcpconnect;
  if ( trydhcp == true )    // try first DHCP according to setup
     {
      dhcpconnect = Ethernet.begin(mac);  
     } 
  if (trydhcp == false || dhcpconnect == false)
    {
     lcd.setCursor(0,1);
     lcd.print("IP: using static");
     delay(1000);
     Ethernet.begin(mac, ip);     // if not successful, use the specified static address
    }  
  delay(1000);
  
  // Serial.print("IP = ");
  // Serial.println(Ethernet.localIP());
  
  lcd.setCursor(0,1);
  lcd.print(Ethernet.localIP());
  lcd.print("         ");
  delay(1000); 
  
  if ( usewatchdog == true )
     {
      wdt_enable(WDTO_8S);
     }  
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
unsigned long resetcounter = 5*60L*AUTORESET;
int timecounter = 1;
int sensorcounter = 1;
int displaycounter = 10;
int sendcounter = 1;

void loop()
{

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
  resetcounter--;
  thistime = nice(myRTC.getHour(h12,PM))+":"+nice(myRTC.getMinute())+":"+nice(myRTC.getSecond());
}

if (thistime != lasttime)
{
  wdt_reset();
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
      sendcounter = 6 * SENDSPEED;
      wdt_reset();
      
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

      wdt_reset();
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
   temperature = dht.readTemperature() + tempcorr ;   // read temperature and add the correction value
  }
else if(sensorcounter == 475)
  {
   lcd.setCursor(15,0);
   lcd.print(" ");
  }

// the basic loop delay is 20 milliseconds
delay(20);
if (resetcounter < 1 && AUTORESET > 0 && usewatchdog == true)
   {
    lcd.clear();
    lcd.print("Auto-reset!");
    lcd.setCursor(0,1);
    while(true)
    {
     lcd.print(".");
     delay(1000);
     //endless loop to force the 8 sec watchdog (happens every AUTORESET minutes)
     }
   }
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

// the function "setd" provides a simple one-button date/time setup routine
bool setd()
{
  int setval[] = { 0,0,21,1,1 };
  int maxval[] = { 23,59,50,12,31 };
  int minval[] = { 0,0,21,1,1 };
  int monval[] = { 0,31,29,31,30,31,30,31,31,30,31,30,31 };
  int rowval[] = { 0,0,1,1,1,0 };
  int posval[] = { 7,10,9,12,15,15 };
  int maxvalue;
  lcd.clear();
  lcd.print("Time: 00:00 h");
  lcd.setCursor(0,1);
  lcd.print("Date: 2021-01-01");
  lcd.blink();
  for(int setsel = 0; setsel < 5; setsel++ )
  {  
  bool wsetd = false;
  lcd.setCursor( posval[setsel],rowval[setsel] );
  while(wsetd == false)
     {
      if(digitalRead(LIGHTBUTTON) == LOW) 
        { 
         setval[setsel]++;
         if(setsel == 4)
           {
            maxvalue = monval[setval[3]];  // max. days setting depends on the month
            }
         else
           {
            maxvalue = maxval[setsel];
            }
         
         if(setval[setsel] > maxvalue) 
           { 
            setval[setsel] = minval[setsel]; 
           }
         lcd.setCursor( posval[setsel]-1,rowval[setsel] );
         lcd.print(nice( setval[setsel] ));
         lcd.setCursor( posval[setsel], rowval[setsel] );
         int nsetd = 0;
         while(digitalRead(LIGHTBUTTON) == LOW)
           {
           nsetd++;
           delay(10);
           if(nsetd == 150)    // if button hold down for more than 1.5 sec
             {
              wsetd = true; 
              lcd.setCursor( posval[setsel+1], rowval[setsel+1] );
             } 
           }
         }
       if(digitalRead(SENDBUTTON) == LOW)   // this button cancels date/time setup
         {
          lcd.noBlink();
          lcd.clear();
          lcd.print("Date/time cancel");
          delay(500);
          return false;
          }
       delay(20);
     }
  }
  lcd.noBlink();
  lcd.clear();
  lcd.print("Date/time set !");
  
  myRTC.setClockMode(false);
  myRTC.setYear(setval[2]);
  myRTC.setMonth(setval[3]);
  myRTC.setDate(setval[4]);
  myRTC.setDoW(1);
  myRTC.setHour(setval[0]);
  myRTC.setMinute(setval[1]);
  myRTC.setSecond(0);

  delay(500);
  return true;
}
