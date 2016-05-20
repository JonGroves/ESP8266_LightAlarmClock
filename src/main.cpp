/*
 *  The server will set a GPIO pin depending on the request
 *    http://server_ip/gpio/0 will set the GPIO5 low,
 *    http://server_ip/gpio/1 will set the GPIO5 high
 *  server_ip is the IP address of the ESP8266 module, will be
 *  printed to Serial when the module is connected.
 *  Servo is attached to pin 4 currently
 */

#include <ESP8266WiFi.h>
#include <Servo.h>
#include <WiFiUdp.h>

//#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
//#include <WiFiUdp.h>
#include <ArduinoOTA.h>

#include "jOTA.h"

Servo myservo;  // create servo object to control a servo // a maximum of eight servo objects can be created
int servoPos = 0;    // variable to store the servo position
uint8_t servoPin = 5;
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

void alertWithLED(int numTimes, int onTimeMS, int offTimeMS)
{
  int BUILTIN_LED_CORRECT = 2; //GPIO 2 and pin 2
  pinMode(BUILTIN_LED_CORRECT,OUTPUT);
  for(int counter = 0; counter <= numTimes; counter += 1 )
  {
    digitalWrite(BUILTIN_LED_CORRECT,LOW); //Turns on LED
    delay(onTimeMS);
    digitalWrite(BUILTIN_LED_CORRECT,HIGH); //Turns off LED
    delay(offTimeMS);
  }
}


void startServoCode(unsigned long timerEndTime, int timerEndHours, int timerEndMinutes) {
  //***** Start of servo code ****
                      Serial.println("Servo code started...");

                      myservo.attach(servoPin);
                      myservo.write(0);
                      Serial.println("ServoValue=0");

                      Serial.println("Running test...");
                      Serial.println("Warming up the lightbulb...");

                      myservo.write(180);
                      Serial.println("ServoValue=180");
                      delay(1500);  //heat up light

                      Serial.println("Slowly turning up light (Quickly for testing)...");
                      for(servoPos = 55; servoPos < 180; servoPos += 1 )  // goes from 0 degrees to 180 degrees
                      {                                  // in steps of 1 degree
                        myservo.write(servoPos);              // tell servo to go to position in variable 'pos'
                        Serial.println("ServoValue=" + String(servoPos));
                        delay(30);                       // waits 15ms for the servo to reach the position
                      }
                      myservo.write(0);
                      delay (3000); myservo.detach(); //Waits 3 seconds before detaching so servo can finish moving.

                      Serial.println("ServoValue=0");

                      Serial.println("Waiting " + String(timerEndHours) + " Hours and " + String(timerEndMinutes) + " minutes...");
                      Serial.println(String((timerEndTime - millis()) / 1000) + " seconds left");
                      Serial.println("Hopefully, " + String(timerEndTime) + "is greater than " + String(millis()/1000));
                      while (timerEndTime > (millis()/1000)) {
                        Serial.println(String(timerEndTime - (millis() / 1000)) + " seconds left");
                        delay(1000);
                      }


                     // delay(JTimeInMilliSec);  //25,200,000 = 7 hours, 30,600,000 = 8.5 hours, 23,400,000 = 6.5 hrs


                      myservo.attach(servoPin);
                      myservo.write(180);
                      Serial.println("ServoValue=180");
                      delay(1500);  //heat up light

                      Serial.println("Warming up the lightbulb...");

                      myservo.write(0);
                      Serial.println("ServoValue=180");
                      Serial.println("Slowly turning up light...");
                      myservo.write(55);
                      delay(2000) ; myservo.detach();
                      for(servoPos = 55; servoPos < 180; servoPos += 1 )  // goes from 0 degrees to 180 degrees
                      {                                  // in steps of 1 degree
                          myservo.attach(servoPin);
                          myservo.write(servoPos);              // tell servo to go to position in variable 'pos'
                          delay(1000);  myservo.detach(); //Need to delay until the servo stops moving before detachin1

                        Serial.println("ServoValue=" + String(servoPos));
                        delay(13400);                       // waits 14.4seconds (minus the 1000ms before detaching) so that the light brightens over 30 mins (14.4*(180-55))
                        alertWithLED(1, 50, 0);
                      }
                      Serial.println("Ending servo code and starting infinite loop.");

                      while(true)
                      {
                          OTAloopOnce();
                          delay(1000);
                      }
}

void getWebsite(String serverUrl, String websiteURL)
{
Serial.println("Connecting to " + String(serverUrl) + String(websiteURL));
  int status = WL_IDLE_STATUS;
  // if you don't want to use DNS (and reduce your sketch size)
// use the numeric IP instead of the name for the server:
//IPAddress server(74,125,232,128);  // numeric IP for Google (no DNS)
char buf[1];
char server[serverUrl.length()+1];
serverUrl.toCharArray(server, serverUrl.length()+1);    //DO NOT USE HTTP://
//char server[] = "www.google.com";    // name address for Google (using DNS)

  Serial.println("1");
// Initialize the Ethernet client library
// with the IP address and port of the server
// that you want to connect to (port 80 is default for HTTP):
WiFiClient client;

Serial.println("\nStarting connection to server...");
  // if you get a connection, report back via serial:
  if (client.connect(server, 80)) {
    Serial.println("connected to server");
    // Make a HTTP request:
    client.println("GET /u/2452011/LightAlarm/index.php HTTP/1.1");
    client.println("Host: " + String(serverUrl));
 //       client.println("GET /search?q=arduino HTTP/1.1");
 //   client.println("Host: www.google.com");
     client.println("User-Agent: Mozilla/5.0 (Windows NT 6.3; WOW64; rv:38.0) Gecko/20100101 Firefox/38.0");
    client.println("Connection: close");
    client.println();
    client.flush();
    Serial.println("GET " + String(websiteURL) + " HTTP/1.1");  // /pst/now?\\H:\\M:\\S
    Serial.println("Host: " + String(serverUrl));

    Serial.println("Connection: close");
    Serial.flush();
  }
  else
  {
    Serial.println("Could not connect to server");
  }

  delay(1000);


    Serial.flush();
 // if there are incoming bytes available
  // from the server, read them and print them:
  while (client.available()) {
    char c = client.read();
    Serial.write(c);
    delay(1);
  }
  Serial.flush();

  // if the server's disconnected, stop the client:
  if (!client.connected()) {
    Serial.println();
    Serial.println("disconnecting from server.");
    client.stop();
    Serial.flush();

      // do nothing forevermore:
    //while (true);
  }

}


// send an NTP request to the time server at the given address
unsigned long sendNTPpacket(IPAddress& address)
{
  Serial.println("sending NTP packet...");
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  udp.beginPacket(address, 123); //NTP requests are to port 123
  udp.write(packetBuffer, NTP_PACKET_SIZE);
  udp.endPacket();
}

void getCurrentNTPTime(unsigned long &currentEpoch, int &currentHour, int &currentMinute, int &currentSecond) {
  //get a random server from the pool
  WiFi.hostByName(ntpServerName, timeServerIP);

  sendNTPpacket(timeServerIP); // send an NTP packet to a time server
  // wait to see if a reply is available
  delay(2000);

  int cb = udp.parsePacket();
  if (!cb) {
    Serial.println("no packet yet");
  }
  else {
    Serial.print("packet received, length=");
    Serial.println(String(cb));
    // We've received a packet, read the data from it
    udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer

    //the timestamp starts at byte 40 of the received packet and is four bytes,
    // or two words, long. First, esxtract the two words:

    unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
    unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
    // combine the four bytes (two words) into a long integer
    // this is NTP time (seconds since Jan 1 1900):
    unsigned long secsSince1900 = highWord << 16 | lowWord;
    Serial.println("Seconds since Jan 1 1900 = " + String(secsSince1900));

    // now convert NTP time into everyday time:
    // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
    const unsigned long seventyYears = 2208988800UL;
    const unsigned long PDTOffset = 25200; //UTC -7:00
    // subtract seventy years and PDT Offset:
    unsigned long epoch = secsSince1900 - seventyYears - PDTOffset;
    currentEpoch = epoch; //For returning value byRef DO NOT DELETE
    // print Unix time:
    Serial.println("Unix time = " + String(epoch));


    // print the hour, minute and second:

    currentHour = (epoch  % 86400L) / 3600;  // the hour (86400 equals secs per day)
    currentMinute = (epoch  % 3600) / 60; // the minute (3600 equals secs per minute)
    currentSecond = epoch % 60; // the second
    Serial.println("The UTC time is " + String(currentHour) + ":" + String(currentMinute) + ":" + String(currentSecond));       // UTC is the time at Greenwich Meridian (GMT)
  }
}

void setup() {
  OTAsetup();
  OTAloopOnce();

  pinMode(BUILTIN_LED,OUTPUT);
  digitalWrite(BUILTIN_LED,HIGH);

  //On startup, blink built-in LED twice, 100ms on, 100ms off
  alertWithLED(5,50,50);

  Serial.begin(115200);
  delay(10);


  // prepare GPIO for Servo as servoPin
  pinMode(servoPin, OUTPUT);
  digitalWrite(servoPin, 0);
  // Attach servo to pin servoPin
  myservo.attach(servoPin);
  myservo.write(180);
  delay(2000);
  myservo.write(0);
  delay(2000);  myservo.detach(); //Need to delay until the servo stops moving before detaching

/*  //Not required any more since OTA makes sure it's connected to the network
  // Connect to WiFi network
  Serial.println();
  Serial.println();
  Serial.println("Connecting to " + String(ssid));


  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.flush();
  Serial.println("");
  Serial.println("WiFi connected");
*/

  //Get current time
  udp.begin(localPort);


  getCurrentNTPTime(startingEpoch,startingHour,startingMinute,startingSecond);
  Serial.println("Time is " + String(startingHour) + ":" + String(startingMinute) + ":" + String(startingSecond) + " PDT (Valid Mar to Nov), which is an epoch of " + String(startingEpoch) + " seconds");



  // Start the server
  server.begin();
  Serial.println("Server started");

  // Print the IP address
  Serial.println(WiFi.localIP());


}

void loop() {
  OTAloopOnce();
  // Check if a client has connected
  WiFiClient client = server.available();
  if (!client) {
    return;
  }

  // Wait until the client sends some data
  Serial.println("new client");
  while(!client.available()){
    delay(1);
  }

  // Read the first line of the request
  String req = client.readStringUntil('\r');
  Serial.println(req);
  client.flush();

  // Match the request
  int val;
  if (req.indexOf("JTime=") == -1) {
     getCurrentNTPTime(startingEpoch,startingHour,startingMinute,startingSecond);
     Serial.println("Time is " + String(startingHour) + ":" + String(startingMinute) + ":" + String(startingSecond) + " PDT (Valid Mar to Nov), which is an epoch of " + String(startingEpoch) + " seconds");

     Serial.println("Sending webpage...");
     //Serial.flush();
//     client.println("<html>");
//   client.println("<style>");
//    client.println("@media screen and (max-width: 800px){");
//    client.println("body {");
//    client.println("background-color: lightblue;");
//    client.println("font-size:5em;");
//    client.println("}");
//    client.println("input {");
//    client.println("width: 50em;  height: 10em;");
//    client.println("}");
//    client.println("</style>");
//     client.println("<form>");
//     client.println("Time is " + String(startingHour) + ":" + String(startingMinute) + ":" + String(startingSecond) + " PDT (Valid Mar to Nov), which is an epoch of " + String(startingEpoch) + " seconds");
//     client.println("Please enter the wakeup time 30 mins before final alarm time).");
//     client.println("<p><input type=time name=JTime onchange=""this.form.submit()""></input>");
//     client.println("<input type=submit> </input>");
//     client.println("</form>");
//     client.println("</html>");
     String note = "";
     String htmlCode = "<html><meta name=""viewport"" content=""width=device-width, initial-scale=1.0""><font size=5em font-family=sans-serif><form>" + note + "Welcome to the light alarm clock (19May2016 Version).<br><br>Time is " + String(startingHour) + ":" + String(startingMinute) + ":" + String(startingSecond) + " PDT (Valid Mar to Nov), which is an epoch of " + String(startingEpoch) + " seconds.<br><br> Please enter the wakeup time 30 mins before final alarm time.<p><input type=time name=JTime onchange=""this.form.submit()"" style=""height:50px; width:400px""></form></font></html>"; //</input><input type=submit> </input>
     Serial.println(htmlCode);
     client.println(htmlCode);
     Serial.println("Webpage sent.");
     client.stop();
     return;
  }
  else {
      Serial.println("Reading JTime string...");
      int StartCharIndex = req.indexOf("JTime=");
      int JTimeHr = req.substring(StartCharIndex + 6,StartCharIndex + 8).toInt();
      int JTimeMin = req.substring(StartCharIndex + 11,StartCharIndex + 13).toInt();
      int timerEndHours = long((24 + double(JTimeHr) + double(JTimeMin)/60)-(double(startingHour) + double(startingMinute)/60)) % 24;
      int timerEndMinutes = long(60 + JTimeMin - startingMinute) % 60;
      unsigned long timerDuration =  timerEndHours * 3600 + timerEndMinutes * 60;
      unsigned long timerEndTime = long(millis()/1000) + timerDuration;  //https://www.arduino.cc/en/reference/millis
      //long JTimeInMilliSec = JTimeHr * 3600000 + JTimeMin * 60000; //25,200,000 = 7 hours, 30,600,000 = 8.5 hours, 23,400,000 = 6.5 hrs
      //unsigned long endEpoch = startingEpoch + JTimeInMilliSec/1000;
      Serial.println("Request is " + String(req));
      Serial.println("JTimeHr: " + String(JTimeHr));
      Serial.println("JTimeMin: " + String(JTimeMin));
      Serial.println("Total timer duration: " + String(timerDuration));
      Serial.println("Starting servo code...");
      startServoCode(timerEndTime,timerEndHours,timerEndMinutes);
  }

  // Set GPIO5 according to the request
//  digitalWrite(5, val);

  // Prepare the response
  String s = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<!DOCTYPE HTML>\r\n<html>\r\nGPIO is now ";
  s += (val)?"high":"low";
  s += "</html>\n";

  // Send the response to the client
  client.println(s);
  delay(1);
  Serial.println("Client disconnected");

  // The client will actually be disconnected
  // when the function returns and 'client' object is detroyed
}
