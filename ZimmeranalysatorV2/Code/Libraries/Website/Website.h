#ifndef Website_h
#define Website_h

#include "Arduino.h"
#include <WiFi.h>

class Website
{
  public:
    Website(bool lr);
    void printWebsite(double tem, double hum, double lux, short co2, uint16_t year, uint8_t mon, uint8_t mday, uint8_t hour, uint8_t minute, uint8_t sec, WiFiClient client);
  private:
	String addzero(uint8_t date);
	bool livingroom;
};

#endif