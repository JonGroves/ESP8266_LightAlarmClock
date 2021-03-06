#include "main.h"
/*
 *  The server will set a GPIO pin depending on the request
 *    http://server_ip/gpio/0 will set the GPIO5 low,
 *    http://server_ip/gpio/1 will set the GPIO5 high
 *  server_ip is the IP address of the ESP8266 module, will be
 *  printed to Serial when the module is connected.
 */

//Options:
const bool getTimefromNTP = false;
const int wakeupDuration = 30; //in minutes
int ledIntensityArray[] = {0,0,0};

// ___________________________________________________________________________
void setup() {
  OTAsetup();
  OTAloopOnce();
  setLEDIntensity(0);

  // Start the mDNS for the network (Doesn't work on android)
  //MDNSConnect();

  // Start the websockets
  Serial.print("Starting websockets...");
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  Serial.println("Done!");

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

  analogWrite(PIN_RED,1); //Sets them as output //Writing 0 doesn't set them as outputs
  analogWrite(PIN_GREEN,1);//Sets them as output
  analogWrite(PIN_BLUE,1);//Sets them as output

  Serial.print("Testing LEDs");
  for(int led_counter=0; led_counter<=1024; led_counter++)
  {
    brightnessCurve(led_counter,ledIntensityArray); //Calculate the ledIntensityArray and have these values edited
    //Serial.println(String(ledIntensityArray[0])+"|"+String(ledIntensityArray[1])+"|"+String(ledIntensityArray[2]));
    setLEDIntensity(ledIntensityArray); //Pass the ledIntensityArray to the setLEDIntensity function
    delay(1);
  }
  for(int led_counter=1024; led_counter>=0; led_counter--)
  {
    brightnessCurve(led_counter,ledIntensityArray); //Calculate the ledIntensityArray and have these values edited
    //Serial.println(String(ledIntensityArray[0])+"|"+String(ledIntensityArray[1])+"|"+String(ledIntensityArray[2]));
    setLEDIntensity(ledIntensityArray); //Pass the ledIntensityArray to the setLEDIntensity function
    delay(1);
  }
  digitalWrite(PIN_RED, 0);
  digitalWrite(PIN_BLUE, 0);
  digitalWrite(PIN_GREEN, 0);

  Serial.println("Done!");

}

// ___________________________________________________________________________
void loop() {

  OTAloopOnce();
  // Check if a client has connected

  WiFiClient client = server.available();
  if (!client) {
    webSocket.loop();
    delay(1);
    return; //Terminate a function and return a value from a function to the calling function, if desired.
  }

  while(!client.available()){
    delay(1);
  }
Serial.println("client found");
//Fixes slow sending of files from SPIFFS (https://github.com/esp8266/Arduino/issues/1853)
  client.setNoDelay(1); //about 3 times faster

  // Read the first line of the request
  String req = client.readStringUntil('\r');
  Serial.println(req);
  client.flush();

  // Match the request
  int val;
  if (req.indexOf("JTime=") != -1)  //JTime was specified
  {
      Serial.println("Reading JTime string...");
      webSocket.broadcastTXT("Reading JTime string...");
      int StartCharIndex = req.indexOf("JTime=");
      int JTimeHr = req.substring(StartCharIndex + 6,StartCharIndex + 8).toInt();
      int JTimeMin = req.substring(StartCharIndex + 11,StartCharIndex + 13).toInt();

      //Calculate time until alarm time by setting the alarm time as the time, finding the epoch, changing the time back, and calculating the differece in seconds
      unsigned long currentTime = now();
      adjustTime(12*60*60); //Add 12 hours so the day will be right when we set the time (ie. for tomorrow)
      int timerDay = day();
      int timerMonth = month();
      int timerYear = year();

      setTime(JTimeHr,JTimeMin,0,timerDay,timerMonth,timerYear); //Temporarility sets the time to the wakeup time to get the epoch
      adjustTime(-1*wakeupDuration*60); //Subtract 30 minutes so you set the alarm time, not the when the light alarm starts
      unsigned long alarmTime = now();//get the epoch of the wake up time
      setTime(currentTime);//Set the time back

      long timerDuration = alarmTime - currentTime;
      if (timerDuration<0 || timerDuration>11*60*60)  //Make sure timer is within 11 hours, else start now to show it messed up
      {
        timerDuration=0;
      }
      Serial.println("currenttime:" + String(currentTime) +", alarmTime:" + String(alarmTime));

      Serial.println("Request is " + String(req));
      Serial.println("JTimeHr: " + String(JTimeHr));
      Serial.println("JTimeMin: " + String(JTimeMin));
      Serial.println("Total timer duration: " + String(timerDuration) + " seconds.");
      Serial.println("Starting LED code...");
      Serial.flush();
      webSocket.broadcastTXT("Request is " + String(req) + ". JTimeHr: " + String(JTimeHr) + ". JTimeMin: " + String(JTimeMin) + ". Total timer duration: " + String(timerDuration) + " seconds. Starting LED code...");

      startLEDCode(timerDuration);
  }
  else if (req.indexOf("JCurrentTime=") != -1)  //JCurrentTime was specified (ie. setting the current time by hand)
  {
    String tempString = "JCurrentTime=";
    int StartCharIndex = req.indexOf(tempString);
    startingHour = req.substring(StartCharIndex + tempString.length(),StartCharIndex + tempString.length() + 2).toInt();
    startingMinute = req.substring(StartCharIndex + tempString.length() + 5,StartCharIndex + tempString.length() + 5 + 2).toInt();
    startingSecond=0; //not required
    startingEpoch=0;  //not required
  }
  else if (req.indexOf("restartESP8266=") != -1) //resetart was specified
  {
    ESP.restart();
  }
  else
  {
    Serial.println("Client found");
    int startIndex = req.indexOf(" ") + 1;
    int endIndex = req.indexOf(" ",req.indexOf(" ")+1);
    String pagePath = req.substring(startIndex, endIndex);
    if (pagePath == "/")
    {
      Serial.print("Sending html...");
      pagePath = "/index.html";
      Serial.println("Done!");
    }
    Serial.print("Sending " + pagePath + "...");
    String note = "Set time as 12:01AM for 1 minute in the future.  8:00 AM for 8 hours into the future etc.  Be sure to connect first to websockets for control.";
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
      while (file.peek() != -1)
      {
       htmlCode += file.readStringUntil('\r');
        yield();
      }
      if (htmlCode.indexOf("String(startingHour)") != -1)
      {
        if (getTimefromNTP)
        {
          Serial.print("Getting current time from NTP Servers ... ");
          bool NTPSucceeded = getCurrentNTPTime(startingEpoch,startingHour,startingMinute,startingSecond);
          Serial.println( NTPSucceeded ? "succeeded" : "failed, but that's ok.");
          if (!NTPSucceeded)
          {
            startingHour=0;
            startingMinute=0;
            startingSecond=0;
            startingEpoch=0;
          }
        }
        else
        {
          Serial.print("Skipping getting current time from NTP Servers.");
        }

      }
      htmlCode.replace("String(note)",note);
      htmlCode.replace("String(startingHour)",String(startingHour));
      htmlCode.replace("String(startingMinute)",String(startingMinute));
      htmlCode.replace("String(startingSecond)",String(startingSecond));
      htmlCode.replace("String(startingEpoch)",String(startingEpoch));

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

void setLEDIntensity(int intensity)
{
  analogWrite(PIN_RED, intensity);
  analogWrite(PIN_GREEN, intensity);
  analogWrite(PIN_BLUE, intensity);
}

void setLEDIntensity(int RedInt, int GreenInt, int BlueInt)
{
  analogWrite(PIN_RED, RedInt);
  analogWrite(PIN_GREEN, GreenInt);
  analogWrite(PIN_BLUE, BlueInt);
}

void setLEDIntensity(int intensityArray[])
{
  analogWrite(PIN_RED, intensityArray[0]);
  analogWrite(PIN_GREEN, intensityArray[1]);
  analogWrite(PIN_BLUE, intensityArray[2]);
}
// ___________________________________________________________________________
// WebSocket Events
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  Serial.println("something received");
  Serial.flush();
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
          webSocket.broadcastTXT("led just lit");
        }
        if (text.startsWith("getNTP")) {
          unsigned long tempEpoch = 0;
          int tempHour = 0;
          int tempMinute = 0;
          int tempSecond = 0;
          if(getTimefromNTP)
          {
            Serial.print("Getting current time from NTP Servers ... ");
            getCurrentNTPTime(tempEpoch,tempHour,tempMinute,tempSecond);
          }
          else
          {
            Serial.print("Skipping getting current time from NTP Servers.");
          }
          String timeMessage = "Time Message: " + String(tempHour) + ":" + String(tempMinute) + ":" + String(tempSecond); //Will be filtered and not overwrite the normal broadcast message due to "Time Message:"
          webSocket.broadcastTXT("Time Message: " + timeMessage);
        }
        if (text.startsWith("CurrentTime_SET|")) {
          //Get Time (in seconds)
          int startIndex1 = text.indexOf("|",1)+1;
          int endIndex1 = text.indexOf("|",startIndex1);
          int endIndexAdjusted1 = endIndex1-3;//cut off milliseconds with the "-3"
          //Get Timezone offset (in minutes)
          int startIndex2 = text.indexOf("|",endIndex1)+1;
          int endIndex2 = text.length();
          time_t tempEpoch = (unsigned long)((text.substring(startIndex1,endIndexAdjusted1).toInt())) - (unsigned long)(text.substring(startIndex2,endIndex2).toInt())*60; //8hrs for PST (Nov-Mar)
          setTime(tempEpoch);
          Serial.println("tempepoch:"+ String(tempEpoch));
          String timeMessage = "Time Message: " + String(hour()) + ":" + String(minute()) + ":" + String(second()); //Will be filtered and not overwrite the normal broadcast message due to "Time Message:"
          webSocket.broadcastTXT("Time Message: " + timeMessage);
          Serial.println("Time Message: " + timeMessage);
        }
        if (text.startsWith("RGB_SET|")) {
          //Red
          int startIndex = text.indexOf("|",1)+1;
          int endIndex = text.indexOf("|",startIndex);
          int RedInt = (text.substring(startIndex, endIndex)).toInt();
          //Green
          startIndex = text.indexOf("|",endIndex)+1;
          endIndex = text.indexOf("|",startIndex);
          int GreenInt = (text.substring(startIndex, endIndex)).toInt();
          //Blue
          startIndex = text.indexOf("|",endIndex)+1;
          endIndex = text.length();
          int BlueInt = (text.substring(startIndex, endIndex)).toInt();
          //Set
          setLEDIntensity(RedInt,GreenInt,BlueInt);
          //DEBUGGING(xVal);
          String message = text;
        }
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
  //Serial.print("Setting up MDNS (Only accessible from computer) ... ");
  //Serial.println(MDNS.begin("JLightAlarm") ? "succeeded" : "failed!");

  //Serial.print("Adding MDNS websockets and http ports ... ");
  //MDNS.addService("ws", "tcp", 81);
  //MDNS.addService("http", "tcp", 80);
  //Serial.println("Done");
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
void startLEDCode(unsigned long timerDuration) {
  //***** Start of led code ****
                      Serial.println("LED code started...");
                      Serial.println("Running test...");
                      Serial.println("Slowly turning up light (Quickly for testing)...");
                      for(int ledSetpoint = 0; ledSetpoint <= 1024; ledSetpoint++)
                      {
                        brightnessCurve(ledSetpoint,ledIntensityArray); //Calculate the ledIntensityArray and have these values edited
                        setLEDIntensity(ledIntensityArray); //Pass the ledIntensityArray to the setLEDIntensity function
                        delay(5);                       // waits 15ms
                      }
                      for(int ledSetpoint = 1024; ledSetpoint >= 0; ledSetpoint--)
                      {
                        brightnessCurve(ledSetpoint,ledIntensityArray); //Calculate the ledIntensityArray and have these values edited
                        setLEDIntensity(ledIntensityArray); //Pass the ledIntensityArray to the setLEDIntensity function
                        delay(5);                       // waits 15ms
                      }


                      int hoursLeft = (int)(timerDuration / 60 / 60);
                      int minutesLeft = (int)((timerDuration / 60 ) - (hoursLeft * 60));
                      int secondsLeft;
                      Serial.println("Waiting " + String(hoursLeft) + " Hours and " + String(minutesLeft) + " minutes...");
                      String lastStringSent="";
                      String currentString;
                      while (timerDuration > (millis()/1000)) {
                        String currentString = String(timerDuration - (millis() / 1000));
                        if (lastStringSent != currentString)
                        {
                          //Serial.println(String(timerDuration - (millis() / 1000)) + " seconds left");
                          webSocket.broadcastTXT(String(hoursLeft) +" hours, " + String(minutesLeft) + " minutes, " + String(secondsLeft) + " seconds left");
                          lastStringSent = currentString;
                        }

                        hoursLeft    = (int)((timerDuration - (millis() / 1000)) / 60 / 60);
                        minutesLeft = (int)(((timerDuration - (millis() / 1000)) / 60 ) - (hoursLeft * 60));
                        secondsLeft  = (int)((timerDuration - (millis() / 1000)) - (hoursLeft * 60 * 60) - (minutesLeft * 60));

                        //Blink the LED every second
                        delay(200);
                        if (hoursLeft>24)
                        {
                          //Blink the LED 10 times per second forever
                          digitalWrite(2,LOW); //Turns on LED
                          delay(1);
                          digitalWrite(2,HIGH); //Turns off LED
                          delay(100);
                          Serial.println("Time was wrong.  I'm in an endless loop forever now");
                        }
                      }


                     // delay(JTimeInMilliSec);  //25,200,000 = 7 hours, 30,600,000 = 8.5 hours, 23,400,000 = 6.5 hrs


                      Serial.println("Slowly turning up LEDs...");
                      float incrementalTime = wakeupDuration*60*1000/1024;
                      for(int ledSetpoint = 0; ledSetpoint <= 1024; ledSetpoint++)
                      {
                        brightnessCurve(ledSetpoint,ledIntensityArray); //Calculate the ledIntensityArray and have these values edited
                        setLEDIntensity(ledIntensityArray); //Pass the ledIntensityArray to the setLEDIntensity function
                        delay(incrementalTime);   // waits 1758ms (1.758 seconds) so that the light brightens over 30 mins (30*60*1000/1024)
                      }

                      Serial.println("Ending LED code and starting infinite loop.");

                      while(true)
                      {
                          OTAloopOnce();
                          delay(10000);
                      }
}
void brightnessCurve(int timeIndex, int ledSetpointArray[])
{
  ledSetpointArray[0] = min(max((int)(pow((double)timeIndex,(double)1.2)),0),1024);
  ledSetpointArray[1] = min(max((int)(pow(max((double)timeIndex-250,(double) 0),1.2)),0),1024);
  ledSetpointArray[2] = min(max((int)(pow(max((double)timeIndex-480,(double) 0),1.1)),0),1024);
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
bool getCurrentNTPTime(unsigned long &currentEpoch, int &currentHour, int &currentMinute, int &currentSecond) {
  //get a random server from the pool
  WiFi.hostByName(ntpServerName, timeServerIP);

  sendNTPpacket(timeServerIP); // send an NTP packet to a time server
  // wait to see if a reply is available
  delay(3000); //was 2000 and seemed to work

  int cb = udp.parsePacket();
  if (!cb) {
    Serial.println("no packet received.  Returning false.");
    return false;
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

    return true;
  }
}
