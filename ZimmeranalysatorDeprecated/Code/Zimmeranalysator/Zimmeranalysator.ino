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
#include <DHT22.h>            // To interpret the DHT via 1-Wire
#include "MHZ19.h"            // To interpret the MHZ-19 via UART
#include <Adafruit_SSD1306.h> // To handle the OLED Display (SSD1306)
#include <Adafruit_GFX.h>     // To Display graphics on OLED
#include <Website.h>

#define pushButton_pin   33
#define SDlog_pin 34
#define RedLED   25
#define GreenLED   32
#define PinDHT 26
#define PinLDR 35
#define timeoutTime 2000
#define livingroom false      // livingroom = false -> bedroom

/* --------------------------------------------------------------------------*/

Website Website(livingroom);

// Replace with your network credentials
const char* ssid     = "YourWifiName";
const char* password = "YourWifiPassword";

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
short lux,co2;      // Actual values for lightintensity and CO2

// Actual time variables
uint16_t year;
uint8_t mon,mday,hour,minute,sec;

hw_timer_t *Timer0 = NULL;          // Setting up hardware timer
hw_timer_t *Timer1 = NULL;          // Setting up hardware timer
bool volatile TimeTrigger = false;  // Flag set true by hardware timer
char volatile TimerTimes = 0;       // Timer cant be set to more than 20 sec
bool volatile DispTrig = false;     // Flag set true if Display should be turned off

bool volatile ButtonTrigger = false;  // Flag set by button

DHT22 dht22(PinDHT);           // DHT22 object

MHZ19 myMHZ19;                 // Create MHZ19 object
HardwareSerial mySerial(2);    // On ESP32 we do not require the SoftwareSerial library, since we have 2 UARTS available (16,17)

void evalQuality(double h, short c)
{
  // Air is only good, if 40<hum<60 and CO2<1250 (1400 CO2 is really bad)
  if ((h<40) || (h>62) || (c>1250))
  {
    digitalWrite(RedLED,1);
    digitalWrite(GreenLED,0);
  }
  else
  {
    digitalWrite(RedLED,0);
    digitalWrite(GreenLED,1);
  }
}

void getSensoryData(double& t, double& h, short& l, short& c)
{
  t = dht22.getTemperature();
  h = dht22.getHumidity();
  l = analogRead(PinLDR);       // Read voltage on PinLDR
  c = myMHZ19.getCO2();         // Request CO2 (as ppm)
}

void initTime(String timezone) 
{
  struct tm timeinfo;

  configTime(0, 0, "pool.ntp.org");  // First connect to NTP server, with 0 TZ offset

  while (!getLocalTime(&timeinfo)) {
    delay(500);
    digitalWrite(RedLED,!digitalRead(RedLED));
  }
  
  // Now we can set the real timezone
  setenv("TZ", timezone.c_str(), 1);  //  Now adjust the TZ.  Clock settings are adjusted to show the new local time
  tzset();
}

// Declaration for an SSD1306 Display connected to I2C (SDA, SCL pins)
// (SCREEN_WIDTH, SCREEN_HEIGHT, ... , ...)
Adafruit_SSD1306 Display(128, 32, &Wire, -1);

void DisplayValues(double T, double H, short CO2, short Lux)
{
  Display.ssd1306_command(SSD1306_DISPLAYON);
  Display.clearDisplay();
  Display.setTextColor(WHITE);
  Display.setCursor(0, 0);
  // Display Temperature
  Display.print("Temp: ");
  Display.print(T);       
  // ° (degree)
  Display.println("C");
  // Display Humidity
  Display.print("Hum: ");
  Display.print(H);
  Display.println("%");
  // Display Lux
  Display.print("Lux: ");
  Display.print(Lux);
  Display.println(" Lux");
  // Display CO2
  Display.print("CO2: ");
  Display.print(CO2);
  Display.println(" PPM");

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
  digitalWrite(RedLED,1);
  digitalWrite(GreenLED,1);
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
  evalQuality(DataArr[1], DataArr[3]);
}

void initLogging(fs::FS &fs)
{
  if (analogRead(SDlog_pin))
  {
    dispshortmsg("Appending", "SD Card");
    return;
  }

  dispshortmsg("Overwriting", "SD Card");

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
}

void IRAM_ATTR toggleOLED()
// void ICACHE_RAM_ATTR toggleOLED()
{
  ButtonTrigger = true;
}

void IRAM_ATTR isrTimer()
{
  if (TimerTimes>8)
  {
    TimeTrigger = true;
    TimerTimes = 0;
  }
  else TimerTimes++;
}

void IRAM_ATTR DispTimer()
// void ICACHE_RAM_ATTR DispTimer()
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
            evalQuality(hum,co2);
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
  pinMode(GreenLED, OUTPUT);
  pinMode(SDlog_pin, INPUT);
  digitalWrite(RedLED,1);
  digitalWrite(GreenLED,1);
  Display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  initLogging(SD);
  delay(3000);
  dispshortmsg("Booting Up");

  mySerial.begin(9600);                               // (Uno example) device to MH-Z19 serial start
  myMHZ19.begin(mySerial);                                // *Serial(Stream) reference must be passed to library begin().

  myMHZ19.autoCalibration();                              // Turn auto calibration ON (OFF autoCalibration(false))
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
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(500);
  }
  dispshortmsg("Initialising", "Time via NTP");
  // Print local IP address and start web server
  server.begin();
  initTime("CET-1CEST,M3.5.0,M10.5.0/3");  // Set for Berlin

  dispshortmsg("Setting Timer", "and ending Prep");

  pinMode(PinLDR,INPUT_PULLUP);
  pinMode(pushButton_pin, INPUT_PULLUP);
  attachInterrupt(pushButton_pin, toggleOLED, FALLING);

  // Init of Time-Interrupt, Formula: Tout = TimeTicks*Prescaler/Clock
  // -> Tout = 120 seconds with TimeTicks=120000 and Prescaler 80000, Clock is 80 MHz
  // -> Tout = 120 seconds with TimeTicks=64000 and Prescaler 150000, Clock is 80 MHz
  // -> Tout = 12 seconds with TimeTicks=64000 and Prescaler 15000, Clock is 80 MHz
  Timer0 = timerBegin(0, 15000, true);
  timerAttachInterrupt(Timer0, isrTimer, true);
  timerAlarmWrite(Timer0, 64000, true);
  timerAlarmEnable(Timer0);

  // Init of Time-Interrupt, Formula: Tout = TimeTicks*Prescaler/Clock
  // -> Tout = ~8 seconds with TimeTicks=42000 and Prescaler 15000, Clock is 80 MHz
  Timer1 = timerBegin(1, 15000, true);
  timerAttachInterrupt(Timer1, DispTimer, true);
  timerAlarmWrite(Timer1, 42000, true);
  timerAlarmDisable(Timer1);

  getSensoryData(tem, hum, lux, co2);
  evalQuality(hum,co2);
  Display.ssd1306_command(SSD1306_DISPLAYOFF);
}
unsigned long begin = millis();
void loop()
{
  WebsiteHandler();
  if (ButtonTrigger)
  {
    getSensoryData(tem, hum, lux, co2);
    evalQuality(hum,co2);
    DisplayValues(tem, hum, co2, lux);
    timerWrite(Timer1, 0);      // reset timer value
    timerAlarmEnable(Timer1);   // enable timer
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
    timerAlarmDisable(Timer1);  // disable timer
    DispTrig = false;
  }
}










