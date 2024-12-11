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
#include <Wire.h>             // For I2C
#include "Adafruit_SHT4x.h"   // To communicate with the SHT45 (Serial Humidity Temperature)
#include "MHZ19.h"            // To interpret the MHZ-19 via UART for CO2
#include <Adafruit_SSD1306.h> // To handle the OLED Display (SSD1306)
#include <Adafruit_GFX.h>     // To Display data on OLED

#define RedLED 13
#define YellowLED 12
#define BlueLED 14
#define GreenLED 27
#define pushButton_pin   33
#define PinLDR 35

/* --------------------------------------------------------------------------*/

double tem,hum;     // Values for temperature and humidity
double lux;         // Value for lux unit
short co2;      // Values for lightintensity and CO2

hw_timer_t *timer2 = NULL;          // Setting up hardware timer 2
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
  Display.print("Lux:  ");
  Display.print(Lux);
  Display.println("   Lux");
  // Display CO2
  Display.print("CO2:  ");
  Display.print(CO2);
  Display.print(CO2<1000?" ":"");
  Display.println("   PPM");

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

void IRAM_ATTR toggleOLED()
{
  ButtonTrigger = true;
}

void ARDUINO_ISR_ATTR DispTimer()
{
  DispTrig = true;
}

void setup()
{
  pinMode(RedLED, OUTPUT);
  pinMode(BlueLED, OUTPUT);
  pinMode(GreenLED, OUTPUT);
  pinMode(YellowLED, OUTPUT);

  pinMode(PinLDR,INPUT);
  pinMode(pushButton_pin, INPUT_PULLUP);
  attachInterrupt(pushButton_pin, toggleOLED, FALLING);

  digitalWrite(YellowLED,1);
  digitalWrite(GreenLED,1);
  digitalWrite(BlueLED,1);
  digitalWrite(RedLED,1);
  Display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  dispshortmsg("Booting Up");
  delay(1000);

  Serial2.begin(9600, SERIAL_8N1, 16, 17);                // HardwareSerial with RX2 = Pin16, TX2 = Pin17
  myMHZ19.begin(Serial2);                                // *Serial(Stream) reference must be passed to library begin().

  myMHZ19.autoCalibration();                              // Turn auto calibration ON (OFF autoCalibration(false))

  sht4.begin();
  // You can have 3 different precisions, higher precision takes longer
  sht4.setPrecision(SHT4X_HIGH_PRECISION);
  sht4.setHeater(SHT4X_NO_HEATER);

  dispshortmsg("Setting Timer");

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

  if (ButtonTrigger)
  {
    getSensoryData(tem, hum, lux, co2);
    DisplayValues(tem, hum, co2, lux);

    timerWrite(timer2, 0);
    timerStart(timer2);

    ButtonTrigger = false;
  }
  
  if (DispTrig)
  {
    Display.ssd1306_command(SSD1306_DISPLAYOFF);
    timerStop(timer2);
    DispTrig = false;
  }
}

