/*
 *  The server will set a GPIO pin depending on the request
 *    http://server_ip/gpio/0 will set the GPIO5 low,
 *    http://server_ip/gpio/1 will set the GPIO5 high
 *  server_ip is the IP address of the ESP8266 module, will be
 *  printed to Serial when the module is connected.
 *  Servo is attached to pin 4 currently
 */
#include "main.h"



// ___________________________________________________________________________
void setup() {
  OTAsetup();
  OTAloopOnce();

  // Start the mDNS for the network (Doesn't work on android)
  MDNSConnect();

  // Start the websockets
  Serial.print("Starting websockets...");
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  Serial.println("Done!");
  //OTA makes sure it's connected to the network

   //Begin udp port for getting NTP time
   udp.begin(localPort);

   // Start the server
   Serial.print("Starting server...");
   server.begin();
   Serial.println("Done!");

  SPIFFS.begin();
  if (true) { //output filepaths on spiffs.  32 CHARACTER LENGTH MAX!  Check for weird characters at the end of these outputted strings to see if that's an issue
    Dir dir = SPIFFS.openDir("/");
    Serial.print("Contents of SPIFFS:");
    while (dir.next()) {
     Serial.print(dir.fileName() +", ");
   }
 }
  Serial.println("*Done!*");

  Serial.print("Setting up LEDs...");
  pinMode(BUILTIN_LED,OUTPUT);
  digitalWrite(BUILTIN_LED,HIGH);

  //On startup, blink built-in LED 5 times, 50ms on, 50ms off
  alertWithLED(5,50,50);
  Serial.println("Done!");

  Serial.print("Setting up and zeroing servo...");
  // prepare GPIO for Servo as servoPin
  pinMode(servoPin, OUTPUT);
  digitalWrite(servoPin, 0);
  // Attach servo to pin servoPin
  myservo.attach(servoPin);
  myservo.write(175);
  delay(2000);
  myservo.write(0);
  delay(2000);  myservo.detach(); //Need to delay until the servo stops moving before detaching
  Serial.println("Done!");

  Serial.println("Waiting for a client...");
      Serial.println("This websocket loop really slows down transfering the js, css, and html files.  Not sure why, but look into it!");
}

// ___________________________________________________________________________
void loop() {
  OTAloopOnce();
  // Check if a client has connected
  WiFiClient client = server.available();
  if (!client) {
    webSocket.loop();
    return;
  }

  // Wait until the client sends some data
  //Serial.println("Client connected");
  while(!client.available()){
    delay(1);
  }

//Fix slow sending of files from SPIFFS (https://github.com/esp8266/Arduino/issues/1853)
  client.setNoDelay(1); //about 3 times faster

  // Read the first line of the request
  String req = client.readStringUntil('\r');
  //Serial.println(req);
  client.flush();

  // Match the request
  int val;
  if (req.indexOf("JTime=") == -1) {
    int startIndex = req.indexOf(" ") + 1;
    int endIndex = req.indexOf(" ",req.indexOf(" ")+1);
    String pagePath = req.substring(startIndex, endIndex);
    if (pagePath == "/")
    {
      pagePath = "/colors.html";
    }
     Serial.print("Sending " + pagePath + "...");
     String note = "";
     Dir dir = SPIFFS.openDir("/");
     File file;
     //String filepath = "/colors.html";
     if(SPIFFS.exists(pagePath))
     {
       file = SPIFFS.open(pagePath, "r");
     }
     if (!file) {
       Serial.print("\nERROR: " + pagePath + " not found or failed to open.");
     }

     if (pagePath.indexOf(".html") == -1) //ie not an html file
     {
       while (file.available())
       {
         size_t bufferLength = 2048;  //Bigger is MUCH faster (2048 is about 5x faster than 1024), but bigger than this usually causes the esp8266 to crash (our of memory).
         char buffer[bufferLength];
         size_t lengthOfBufferUsed = file.readBytes(buffer, bufferLength);
         yield();
         client.write(buffer,lengthOfBufferUsed);
       }
         client.flush();
     }
     else
     {
       String htmlCode = "";
       //file.setTimeout(5000);
       while (file.peek() != -1)
       {
        htmlCode += file.readStringUntil('\r');
         yield();
       }
       if (htmlCode.indexOf("String(startingHour)") != -1)
       {
              getCurrentNTPTime(startingEpoch,startingHour,startingMinute,startingSecond);
       }
       htmlCode.replace("String(note)",note);
       htmlCode.replace("String(startingHour)",String(startingHour));
       htmlCode.replace("String(startingMinute)",String(startingMinute));
       htmlCode.replace("String(startingSecond)",String(startingSecond));
       htmlCode.replace("String(startingEpoch)",String(startingEpoch));
      // int htmlCodeLength = htmlCode.length() + 1; //+1 for null terminator
      // char htmlCodeCharArray[htmlCodeLength];
    //   htmlCode.toCharArray(htmlCodeCharArray, htmlCodeLength);
    //   const char * htmlCodeCharArray = htmlCode.c_str();

      //Sending data  in chucks overrides the small buffer and timeout of sending data as one string
      for(int startOfChunk = 0; startOfChunk < htmlCode.length(); startOfChunk += 2048)
      {
        int endOfChunk = startOfChunk + 2048;
        //Serial.println("Sending chars " + String(startOfChunk) + " to " + String(endOfChunk) + " of " + String(htmlCode.length()));
        if (endOfChunk > htmlCode.length())
        {
          endOfChunk = htmlCode.length();
        }

        client.print(htmlCode.substring(startOfChunk,endOfChunk));
        client.flush();
        yield();
      }
    }


     //Serial.println(htmlCode);
     Serial.println("Done!");
     client.flush();
     //client.stop();

     return;
  }
  else {
      Serial.println("Reading JTime string...");
      webSocket.broadcastTXT("Reading JTime string...");
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
      webSocket.broadcastTXT("Request is " + String(req) + ". JTimeHr: " + String(JTimeHr) + ". JTimeMin: " + String(JTimeMin) + ". Total timer duration: " + String(timerDuration) + ". Starting servo code...");
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



// ___________________________________________________________________________
// WebSocket Events
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  digitalWrite(2,LOW); //Turns on LED
  delay(1);
  digitalWrite(2,HIGH); //Turns off LED

  switch (type) {
    case WStype_DISCONNECTED:
      Serial.print("Websockets disconnected.");
      Serial.flush();
      break;
    case WStype_CONNECTED:
      {
        IPAddress ip = webSocket.remoteIP(num);
        Serial.print("Websockets connected at");
        Serial.println(ip);
        Serial.flush();
      }
      break;

    case WStype_TEXT:
      {
        lastTimeRefresh = millis();

        String text = String((char *) &payload[0]);
        Serial.println(text);
        Serial.flush();
        if (text == "LED") {
          digitalWrite(13, HIGH);
          delay(500);
          digitalWrite(13, LOW);
          Serial.println("led just lit");
          //webSocket.sendTXT(num, "led just lit", length);
          webSocket.broadcastTXT("led just lit");
        }

        if (text.startsWith("getNTP")) {
          unsigned long tempEpoch = 0;
          int tempHour = 0;
          int tempMinute = 0;
          int tempSecond = 0;
          getCurrentNTPTime(tempEpoch,tempHour,tempMinute,tempSecond);
          String timeMessage = "Time Message: " + String(tempHour) + ":" + String(tempMinute) + ":" + String(tempSecond); //Will be filtered and not overwrite the normal broadcast message due to "Time Message:"
          webSocket.broadcastTXT("Time Message: " + timeMessage);
        }

        if (text.startsWith("x")) {
          String xVal = (text.substring(text.indexOf("x") + 1, text.length()));
          int xInt = xVal.toInt();
          if (!myservo.attached())
          {
            myservo.attach(servoPin);
          }
          myservo.write(xInt);
          //analogWrite(REDPIN, xInt);
          //DEBUGGING(xVal);
          String message = "Manual to "+ String(xInt);
          //webSocket.sendTXT(num, message);
          webSocket.broadcastTXT(message);
        }

        /*
        if (text.startsWith("y")) {
          String yVal = (text.substring(text.indexOf("y") + 1, text.length()));
          int yInt = yVal.toInt();
          //analogWrite(GREENPIN, yInt);
          //DEBUGGING(yVal);
        }

        if (text.startsWith("z")) {
          String zVal = (text.substring(text.indexOf("z") + 1, text.length()));
          int zInt = zVal.toInt();
          //analogWrite(BLUEPIN, zInt);
          //DEBUGGING(zVal);
        }
        if (text.startsWith("t")) {
          String tVal = (text.substring(text.indexOf("t") + 1, text.length()));
          rainbowDelay = tVal.toInt();
          lastTimeRefreshRainbow = 0;
          lastTimeRefresh = 0;
          //DEBUGGING(tVal);
        }
        if (text == "RESET") {
          rainbowFlag = 0;
          //analogWrite(BLUEPIN, LOW);
          //analogWrite(REDPIN, LOW);
          //analogWrite(GREENPIN, LOW);
          //DEBUGGING("reset");

        }
        if (text == "RAINBOW") {
          rainbowFlag = 1;
          lastTimeRefreshRainbow = 0;
          lastTimeRefresh = 0;
          //for (int iii = 0; iii < 256; iii++) {
            //writeWheel(iii, RGB);
            //delay(10);
          //}
          //DEBUGGING("rainbow");
        }
        */
      }
      break;

    case WStype_BIN:
      hexdump(payload, length);
      // echo data back to browser
      webSocket.sendBIN(num, payload, length);
      Serial.print("Websockets binary data was sent.");
      Serial.flush();
      break;
  }
}

// ___________________________________________________________________________
void MDNSConnect() {
  //NOTE: the MDNS works, but android can't handle mdns.  Try from a computer
  if (!MDNS.begin("JLightAlarm")) {
   Serial.println("Error setting up MDNS responder!");
    while (1) {
      delay(1000);
    }
  }
  Serial.println("mDNS responder started");
  MDNS.addService("ws", "tcp", 81);
  MDNS.addService("http", "tcp", 80);
}

// ___________________________________________________________________________
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

// ___________________________________________________________________________
void startServoCode(unsigned long timerEndTime, int timerEndHours, int timerEndMinutes) {
  //***** Start of servo code ****
                      Serial.println("Servo code started...");

                      myservo.attach(servoPin);
                      myservo.write(0);
                      Serial.println("ServoValue=0");

                      Serial.println("Running test...");
                      Serial.println("Warming up the lightbulb...");

                      myservo.write(175);
                      Serial.println("ServoValue=175");
                      delay(1500);  //heat up light

                      Serial.println("Slowly turning up light (Quickly for testing)...");
                      for(servoPos = 55; servoPos < 175; servoPos += 1 )  // goes from 0 degrees to 175 degrees
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
                        unsigned long hoursLeft = (timerEndTime - millis()) / 1000 / 60 / 60;
                        unsigned long minutesLeft = ((timerEndTime - millis()) / 1000 / 60) - (hoursLeft * 60);
                        webSocket.broadcastTXT(String(hoursLeft) +" hours, " + String(minutesLeft) + " minutes left");
                        digitalWrite(2,LOW); //Turns on LED
                        delay(1);
                        digitalWrite(2,HIGH); //Turns off LED
                        delay(1000);
                      }


                     // delay(JTimeInMilliSec);  //25,200,000 = 7 hours, 30,600,000 = 8.5 hours, 23,400,000 = 6.5 hrs


                      myservo.attach(servoPin);
                      myservo.write(175);
                      Serial.println("ServoValue=175");
                      delay(1500);  //heat up light

                      Serial.println("Warming up the lightbulb...");

                      myservo.write(0);
                      Serial.println("ServoValue=175");
                      Serial.println("Slowly turning up light...");
                      myservo.write(55);
                      delay(2000) ; myservo.detach();
                      for(servoPos = 55; servoPos < 175; servoPos += 1 )  // goes from 0 degrees to 180 degrees
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

// ___________________________________________________________________________
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

// ___________________________________________________________________________
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

// ___________________________________________________________________________
void getCurrentNTPTime(unsigned long &currentEpoch, int &currentHour, int &currentMinute, int &currentSecond) {
  //get a random server from the pool
  WiFi.hostByName(ntpServerName, timeServerIP);

  sendNTPpacket(timeServerIP); // send an NTP packet to a time server
  // wait to see if a reply is available
  delay(3000); //was 2000 and seemed to work

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
