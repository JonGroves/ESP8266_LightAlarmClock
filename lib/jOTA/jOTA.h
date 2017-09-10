/**************
Created by JG on 2016.05.11
1. Add the following to platformio.ini:
  upload_port = 192.168.2.125
2. Add the following to your main.cpp after the other #includes:
      #include <ArduinoOTA.h>
      #include <WiFiUdp.h>
      #include "jOTA.h"
3. Add to the START of setup():
  OTAsetup();
  OTAloopOnce();
4. Add to the START of loop():
  OTAloopOnce();
5. Add both the .cpp and .h files to the library folder, not the SRC folder
6.  Make sure the following are included in the main.cpp file (in the same order, and not twice):
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
*/

void OTAsetup();
void OTAloopOnce();
