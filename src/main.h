/*
 *  This sketch demonstrates how to set up a simple HTTP-like server.
 *  The server will set a GPIO pin depending on the request
 *    http://server_ip/gpio/0 will set the GPIO5 low,
 *    http://server_ip/gpio/1 will set the GPIO5 high
 *  server_ip is the IP address of the ESP8266 module, will be
 *  printed to Serial when the module is connected.
 *  Servo is attached to pin 4 currently
 */
 //Updated on 19May2016 to have LED indicator, clean up servo pin numbers, verified it works with esp8266, detach servos when not in use, clean up html code
  //Updated on 8Apr2017 to replace servo with LED MOSFET code

#include <ESP8266WiFi.h>
#include <Servo.h>
#include <WiFiUdp.h>
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>
#include <FS.h>
#include <Stream.h>
#include "jOTA.h"
#include "WebSocketsServer.h"
#include <Hash.h> //Required for websockets
#include <DNSServer.h>
#include <time.h>

int PIN_RED = 12; //IO12, Pin10
int PIN_GREEN = 14; //IO14, Pin9
int PIN_BLUE = 13; //IO13, Pin12

//Websocket objects
WebSocketsServer webSocket = WebSocketsServer(81);
unsigned long lastTimeRefresh = 0;
unsigned long lastTimeRefreshRainbow = 0;
unsigned long lastTimeHost = 0;
boolean rainbowFlag = 1;
int RGB[3];
int cnt = 0;
int rainbowDelay = 500;

#define SPIFFS_OBJ_NAME_LEN (64)
//For NTP Time

unsigned int localPort = 2390;      // local port to listen for UDP packets
/* Don't hardwire the IP address or we won't get the benefits of the pool.
 *  Lookup the IP address for the host name instead */
//IPAddress timeServer(129, 6, 15, 28); // time.nist.gov NTP server
IPAddress timeServerIP; // time.nist.gov NTP server address
const char* ntpServerName = "time.nist.gov";
const int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message
byte packetBuffer[ NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets
// A UDP instance to let us send and receive packets over UDP
WiFiUDP udp;

  unsigned long startingEpoch;
  int startingHour;
  int startingMinute;
  int startingSecond;

//const char* ssid = "********";
//const char* password = "********";

// Create an instance of the server
// specify the port to listen on as an argument
WiFiServer server(80);


void setup();
void loop();
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length);
void MDNSConnect();
void alertWithLED(int numTimes, int onTimeMS, int offTimeMS);
void startLEDCodeOld(unsigned long timerEndTime, int timerEndHours, int timerEndMinutes);
//void startLEDCode(unsigned long timerEndTime, int timerEndHours, int timerEndMinutes);

void getWebsite(String serverUrl, String websiteURL);
// send an NTP request to the time server at the given address
unsigned long sendNTPpacket(IPAddress& address);
bool getCurrentNTPTime(unsigned long &currentEpoch, int &currentHour, int &currentMinute, int &currentSecond);
void setLEDIntensity(int intensity);
