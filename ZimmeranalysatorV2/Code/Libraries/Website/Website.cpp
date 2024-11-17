#include "Arduino.h"
#include "Website.h"
#include <WiFi.h>

Website::Website(bool lv)
{
  livingroom = lv;
}

String Website::addzero(uint8_t date)
{
	if (date<10) return ("0"+String(date));
	else return (String(date));
}

void Website::printWebsite(double tem, double hum, short lux, short co2, uint16_t year, uint8_t mon, uint8_t mday, uint8_t hour, uint8_t minute, uint8_t sec, WiFiClient client)
{
	client.println("<html>");
	client.println("<head>");
	if (livingroom) client.println("<title>Wohnzimmeranalysator</title>");
	else client.println("<title>Schlafzimmeranalysator</title>");
	client.println("<meta charset=\"UTF-8\">");
	client.println("<meta name=\"keywords\" content=\"HTML,CSS\">");
	client.println("<link rel=\"icon\" href=\"data:,\">");
	client.println("<link rel=\"stylesheet\" src=\"//normalize-css.googlecode.com/svn/trunk/normalize.css\"/>");
	client.println("<style>html{font-family: Arial; text-align: center;}");
	client.println("h1{color: #0F3376; padding: 2vh;}p{font-size: 1.5rem;}");
	client.println(".button{display: inline-block; background-color: #53C14E; border: 1.5rem;");
	client.println("border-radius: 1.5rem; color: white; padding: 1.5rem; text-decoration: none; ");
	client.println("font-size: 1.5rem; margin: 0.5rem; cursor: pointer;}");
	client.println("body {background-color: LightSlateGray;}");
	client.println(".progress-ww div {display: flex;}");
	client.println(".progress-ww div span {flex: 1;}");
	client.println(".progress-ww div span:first-of-type {text-align: right;padding-right: 1rem;}");
	client.println(".progress-ww div span:last-of-type {text-align: left;padding-left: 1rem;}");
	client.println(".progress-ww {width: 65rem;border: 1rem outset Crimson;background-color: AliceBlue;");
	client.println("text-align: center;padding: 1rem;font-size: 1.5rem;}");
	client.println(".rechtsbuendig {text-align: right;}");
	client.println("</style>");
	client.println("</head>");
	if (livingroom) client.println("<body> <h1>Wohnzimmeranalysator</h1> ");
	else client.println("<body> <h1>Schlafzimmeranalysator</h1> ");
	client.println("<center><div class=\"progress-ww\"><br>");

	client.println("<div><span>Temperatur</span>:<span>");
	client.println(String(tem));
	client.println("</span><span>&ordmC</span></div><br>");
	client.println("<div><span>Luftfeuchtigkeit</span>:<span>");
	client.println(String(hum));
	client.println("</span><span>%</span></div><br>");
	client.println("<div><span>Lichtintensit&aumlt</span>:<span>");
	client.println(String(lux));
	client.println("</span><span>Lux</span></div><br>");
	client.println("<div><span>CO2</span>:<span>");
	client.println(String(co2));
	client.println("</span><span>PPM</span></div><br>");
	client.println("<p class=\"rechtsbuendig\"><small>");
	client.println(addzero(mday)+"."+addzero(mon)+"."+String(year)+" ");
	client.println(addzero(hour)+":"+addzero(minute)+":"+addzero(sec));
	client.println("</small></p></center> </div></center>");
	client.println("<p>");
	client.println("<a href=\"/?refr\"> <button class=\"button\">REFRESH</button></a>");
	client.println("</p>");
	if (livingroom) client.println("<p><a <button class=\"button\" onclick=\"gotobedroom()\">Schlafzimmer</button></a></p>");
	else client.println("<p><a <button class=\"button\" onclick=\"gotolivingroom()\">Wohnzimmer</button></a></p>");
	client.println("<script>");
	client.println("function gotobedroom(){");
	client.println("window.open(\"http://192.168.2.223\",\"_self\");}");
	client.println("function gotolivingroom(){");
	client.println("window.open(\"http://192.168.2.222\",\"_self\");}");
	client.println("</script>");
	client.println("</body></html>");
}
