/* SD Card | ESP32
      GND     GND
      VCC     VIN
      MISO    D19
      MOSI    D23
      SCK     D18
      CS/SS   D5
*/
/* OLED I2C Display | ESP32
      GND     GND
      VCC     VIN
      SDA    D21
      SCL    D22
*/
// Arduino standard libs or esp32 automatically downloaded libs
#include "FS.h"               // FileSystem, important for SD logging
#include "SD.h"               // To interpret SD-Card
#include "SPI.h"              // Serial Peripheral Interface
#include <Wire.h>             // For I2C
#include <WiFi.h>             // For WiFi
#include "time.h"             // Time
// Manually downloaded libs
#include "Adafruit_SHT4x.h"   // To communicate with the SHT45 (Serial Humidity Temperature)
#include "MHZ19.h"            // To interpret the MHZ-19 via UART for CO2
#include <Adafruit_SSD1306.h> // To handle the OLED Display (SSD1306)
#include <Adafruit_GFX.h>     // To Display data on OLED
// Own library
#include <Website.h>          // For Hosting a Website

#define RedLED 13
#define YellowLED 12
#define BlueLED 14
#define GreenLED 27
#define pushButton_pin   33
#define SDlog_pin 32
#define PinLDR 35

#define timeoutTime 2000
#define livingroom true

/* --------------------------------------------------------------------------*/

Website Website(livingroom);

// Strings for the credentials of the local network
String ssid;
String password;

// Set your Static IP address
uint8_t lastnum()
{
  uint8_t lastnum = 223;             // bedroom
  if (livingroom) lastnum = 222;  // livingroom                 
  return lastnum;
}

IPAddress local_IP(192,168,2,lastnum());    
// Set your Gateway IP address
IPAddress gateway(192, 168, 2, 1);

IPAddress subnet(255, 255, 255, 0);
IPAddress primaryDNS(8, 8, 8, 8);   //optional
IPAddress secondaryDNS(8, 8, 4, 4); //optional

WiFiServer server(80);  // Set web server port number to 80

double tem,hum;     // Actual values for temperature and humidity
short analogLux,co2;      // Actual values for lightintensity and CO2
double lux;

// Actual time variables
uint16_t year;
uint8_t mon,mday,hour,minute,sec;

hw_timer_t *timer1 = NULL;          // Setting up hardware timer 1
hw_timer_t *timer2 = NULL;          // Setting up hardware timer 2
bool volatile TimeTrigger = false;  // Flag set true by hardware timer
bool volatile DispTrig = false;     // Flag set true if Display should be turned off

bool volatile ButtonTrigger = false;  // Flag set by button

Adafruit_SHT4x sht4 = Adafruit_SHT4x(); // Object for SHT45
sensors_event_t humidity, temp;

MHZ19 myMHZ19;                 // Create MHZ19 object

// Declaration for an SSD1306 Display connected to I2C (SDA, SCL pins)
// (SCREEN_WIDTH, SCREEN_HEIGHT, ... , ...)
Adafruit_SSD1306 Display(128, 32, &Wire, -1);

double analogToLux(short analogValue) {
    // A function to determine the lux based on the analog value.
    // It is calculated with the existing voltage divider, the datasheet of the ldr and AI
    if (analogValue <= 373) {
        // Under this value the value of lux would be 0.08 or less
        return 0;
    } else if (analogValue <= 2048) {
        // Between the analog values 373 and 2048, lux will be between 0.08 and 1
        return 0.08 + (1.0 - 0.08) * (analogValue - 373) / (2048 - 373);
    } else if (analogValue <= 3722) {
        // Between the analog values 2048 and 3722, lux will be between 1 and 20
        return 1.0 + (20.0 - 1.0) * (analogValue - 2048) / (3722 - 2048);
    } else if (analogValue <= 4054) {
        // Between the analog values 3722 and 4054, lux will be between 20 and 700
        return 20.0 + (700.0 - 20.0) * (analogValue - 3722) / (4054 - 3722);
    } else if (analogValue <= 4090) {
        // Between the analog values 4054 and 4090, lux will be between 700 and 10000
        return 700.0 + (10000.0 - 700.0) * (analogValue - 4054) / (4090 - 4054);
    } else {
        // Between the analog values 4090 and 4095, lux will be fixed at 10000. 
        // Its the end of the ldr datasheet. It means the ldr has just a few Ohms of resistance
        return 10000.0;
    }
}

void evalQuality(double h, short c)
{// Air is only good, if 40<hum<60 and CO2<1250 (1400 CO2 is really bad)

  if ((h<40) || (h>60)) digitalWrite(BlueLED,1);
  else digitalWrite(BlueLED,0);
  if (c>1250) digitalWrite(RedLED,1);
  else digitalWrite(RedLED,0);

  digitalWrite(GreenLED,!(digitalRead(BlueLED)|digitalRead(RedLED)));
}

void getSensoryData(double& t, double& h, double& l, short& c)
{
  sht4.getEvent(&humidity, &temp);// populate humidity and temp objects with fresh data
  // sht4.getEvent(&h, &t);// populate humidity and temp objects with fresh data
  t = double(temp.temperature);
  h = double(humidity.relative_humidity);
  l = analogToLux(analogRead(PinLDR));       // Read voltage on PinLDR
  c = myMHZ19.getCO2();         // Request CO2 (as ppm)

  evalQuality(h, c);
}

void initTime(String timezone) 
{
  struct tm timeinfo;

  configTime(0, 0, "pool.ntp.org");  // First connect to NTP server, with 0 TZ offset

  while (!getLocalTime(&timeinfo)) {
    delay(500);
    digitalWrite(YellowLED,!digitalRead(YellowLED));
  }
  
  // Now we can set the real timezone
  setenv("TZ", timezone.c_str(), 1);  //  Now adjust the TZ.  Clock settings are adjusted to show the new local time
  tzset();
  digitalWrite(YellowLED,0);
}

void DisplayValues(double T, double H, short CO2, double Lux)
{
  Display.ssd1306_command(SSD1306_DISPLAYON);
  Display.clearDisplay();
  Display.setTextColor(WHITE);
  Display.setCursor(0, 0);
  // Display Temperature
  Display.print("Temp: ");
  Display.print(T);     
  Display.print("  ");  
  Display.drawRect(79, 0, 3, 3, WHITE); // degree
  Display.println(" C");
  // Display Humidity
  Display.print("Hum:  ");
  Display.print(H);
  Display.println("  %");
  // Display Lux
  Display.print("Lux: ");
  Display.print(Lux);
  Display.println("    Lux");
  // Display CO2
  Display.print("CO2: ");
  Display.print(CO2);
  Display.print(CO2<1000?" ":"");
  Display.println("    PPM");

  Display.display(); 
}

void dispshortmsg(String msg0, String msg1="")
{
  Display.clearDisplay();
  Display.setTextColor(WHITE);
  Display.setCursor(0, 0);
  Display.println(msg0);
  Display.print(msg1);
  Display.display();
}

void logData(fs::FS &fs){
  digitalWrite(YellowLED,1);
  getSensoryData(tem, hum, lux, co2);

  const char* path;
  if (livingroom) path = "/Wohn.csv"; // File on SD-Card for logging
  else path = "/Schlaf.csv";          // File on SD-Card for logging

  double DataArr[4] = {tem,hum,lux,co2};

  File file = fs.open(path, FILE_APPEND);
  for (char i=0;i<4;i++)
  {
    if (!file.print(String(DataArr[i])+","))
    {
      return;
    }
      
  }

  LoadTime(year, mon, mday, hour, minute, sec);

  uint16_t TimeArr[6] = {year,mon,mday,hour,minute,sec};

  for (char i=0;i<5;i++)
  {
    if (!file.print(String(TimeArr[i])+",")) return;
  } 

  if (!file.print(String(TimeArr[5])+"\n")) return;

  file.close();
  digitalWrite(YellowLED,0);
}

void initLogging(fs::FS &fs)
{
  digitalWrite(YellowLED,1);
  if (analogRead(SDlog_pin))
  {
    dispshortmsg("Appending", "SD Card");
    delay(3500);
    return;
  }

  dispshortmsg("Overwriting", "SD Card");
  delay(3500);

  const char* path;
  if (livingroom) path = "/Wohn.csv"; // File on SD-Card for logging
  else path = "/Schlaf.csv";          // File on SD-Card for logging

  const char* LogHeader = "temperature,humidity,lux,co2,year,month,day,hour,minute,second\n";
  File file = fs.open(path, FILE_WRITE);
  if(!file){
    return;
  }
  if(!file.print(LogHeader)){
    file.close();
    return;
  } 
  file.close();
  digitalWrite(YellowLED,0);
}

void IRAM_ATTR toggleOLED()
{
  ButtonTrigger = true;
}

void ARDUINO_ISR_ATTR isrTimer()
{
  TimeTrigger = true;
}

void ARDUINO_ISR_ATTR DispTimer()
{
  DispTrig = true;
}



void LoadTime(uint16_t& year, uint8_t& mon, uint8_t& mday, uint8_t& hour, uint8_t& minute, uint8_t& sec)
{
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) 
  {
    return;
  }
  // timeinfo.tm_year     // years since 1900
  // timeinfo.tm_mon      // months since January (0-11)
  // timeinfo.tm_mday
  // timeinfo.tm_hour
  // timeinfo.tm_min
  // timeinfo.tm_sec

  // timeinfo.tm_wday     // days since last Sunday (0-6)

  year = timeinfo.tm_year+1900;
  mon = timeinfo.tm_mon+1;
  mday = timeinfo.tm_mday;
  hour = timeinfo.tm_hour;
  minute = timeinfo.tm_min;
  sec = timeinfo.tm_sec;

  if (mday==1)
  {
    if ((hour==4)) 
    {
      if (minute<3) initTime("CET-1CEST,M3.5.0,M10.5.0/3");  // Set for Berlin
    }
  }
}

void getCredentials(fs::FS &fs, String& ssid, String& pw){
  File file = fs.open("/WIFI.txt");
  while (!file) {
    digitalWrite(YellowLED,!digitalRead(YellowLED));
    delay(500);
  }

  bool toggle = true;
  int i;
  while (file.available()) {
    
    i = file.read();

    if (i == 10 || i== 13){
      toggle = false;
    }
    else if (toggle == true){
      ssid += char(i);
    }
    else pw += char(i);
  }
  ssid = ssid.c_str();
  pw = pw.c_str();
  digitalWrite(YellowLED,0);
}

void WebsiteHandler()
{
  delay(1);   // Problem of Webserver, important for esp
  WiFiClient client = server.available();   // Listen for incoming clients

  if (client) {     
    // Time variables to timeout internet connection to client
    unsigned long currentTime = millis();
    unsigned long previousTime = currentTime;                        
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected() && currentTime - previousTime <= timeoutTime) {            // loop while the client's connected
      currentTime = millis();
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        if (c == '\n') {                    // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();

            // get data to Display
            getSensoryData(tem, hum, lux, co2);
            LoadTime(year, mon, mday, hour, minute, sec);

            // Display the HTML web page
            Website.printWebsite(tem, hum, lux, co2, year, mon, mday, hour, minute, sec, client);
            
            // The HTTP response ends with another blank line
            client.println();
            // Break out of the while loop
            break;
          } else { // if you got a newline, then clear currentLine
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
      } 
    }
    // Close the connection
    client.stop();
  }
}

void setup()
{
  pinMode(RedLED, OUTPUT);
  pinMode(BlueLED, OUTPUT);
  pinMode(GreenLED, OUTPUT);
  pinMode(YellowLED, OUTPUT);
  pinMode(SDlog_pin, INPUT);

  pinMode(PinLDR,INPUT_PULLUP);
  pinMode(pushButton_pin, INPUT_PULLUP);
  attachInterrupt(pushButton_pin, toggleOLED, FALLING);

  digitalWrite(YellowLED,1);
  digitalWrite(GreenLED,1);
  digitalWrite(BlueLED,1);
  digitalWrite(RedLED,1);
  Display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  initLogging(SD);
  dispshortmsg("Booting Up");
  delay(1000);

  Serial2.begin(9600, SERIAL_8N1, 16, 17);                // HardwareSerial with RX2 = Pin16, TX2 = Pin17
  myMHZ19.begin(Serial2);                                // *Serial(Stream) reference must be passed to library begin().

  myMHZ19.autoCalibration();                              // Turn auto calibration ON (OFF autoCalibration(false))

  sht4.begin();
  // You can have 3 different precisions, higher precision takes longer
  sht4.setPrecision(SHT4X_HIGH_PRECISION);
  sht4.setHeater(SHT4X_NO_HEATER);

  if(!SD.begin())
  {
    return;
  }
  uint8_t cardType = SD.cardType();

  if(cardType == CARD_NONE)
  {
    return;
  }

  dispshortmsg("Connecting", "to Wifi");

  // Configures static IP address
  if (!WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS)) 
  {
    return;
  }
  
  // Connect to Wi-Fi network with SSID and password

  getCredentials(SD, ssid, password);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(500);
  }
  dispshortmsg("Initialising", "Time via NTP");
  // Print local IP address and start web server
  server.begin();
  initTime("CET-1CEST,M3.5.0,M10.5.0/3");  // Set for Berlin

  dispshortmsg("Setting Timer");

  timer1 = timerBegin(1000000); // 1 MHz
  timerAttachInterrupt(timer1, &isrTimer); //attach callback
  timerAlarm(timer1, 1000000*120, true, 0); //set time in us


  timer2 = timerBegin(1000000); // 1 MHz

  timerAttachInterrupt(timer2, &DispTimer); //attach callback
  timerAlarm(timer2, 1000000*8, true, 0); //set time in us
  timerStop(timer2);
  timerWrite(timer2, 0);

  dispshortmsg("Calculating climate");

  getSensoryData(tem, hum, lux, co2);

  Display.ssd1306_command(SSD1306_DISPLAYOFF);

  digitalWrite(YellowLED,0);
}

void loop()
{
  WebsiteHandler();

  if (ButtonTrigger)
  {
    getSensoryData(tem, hum, lux, co2);
    DisplayValues(tem, hum, co2, lux);

    timerWrite(timer2, 0);
    timerStart(timer2);

    ButtonTrigger = false;
  }

  if (TimeTrigger)
  {
    logData(SD);
    TimeTrigger = false;
  }
  
  if (DispTrig)
  {
    Display.ssd1306_command(SSD1306_DISPLAYOFF);
    timerStop(timer2);
    DispTrig = false;
  }
}

