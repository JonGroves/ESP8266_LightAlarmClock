#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <DNSServer.h>

//Options:
const bool createAccessPoint = true;
const char* ssid = "TELUS1846";
const char* password = "publicspasswordia";
const byte DNS_PORT = 53;
const IPAddress apIP(192, 168, 1, 1);

//Globals:
DNSServer dnsServer;

void OTAsetup() {
  Serial.begin(115200);
  Serial.println("Booting");
  Serial.println(String(createAccessPoint));

  if (createAccessPoint == true)
  {
    WiFi.mode(WIFI_AP); //Access point

    Serial.print("Configuring access point ... ");
    Serial.println(WiFi.softAPConfig(apIP, apIP, IPAddress(255,255,255,0))  ? "succeeded" : "failed!");

    Serial.print("Creating access point ... ");
    //Serial.println(WiFi.begin(ssid, password) ? "succeeded" : "failed!");
    Serial.println(WiFi.softAP(ssid, password) ? "succeeded" : "failed!");
    Serial.println("If the ip is 0.0.0.0, likely's it's actually 192.168.1.1");

//https://github.com/esp8266/Arduino/blob/4897e0006b5b0123a2fa31f67b14a3fff65ce561/libraries/DNSServer/examples/DNSServer/DNSServer.ino

/*
    dnsServer.setTTL(300);
    dnsServer.setErrorReplyCode(DNSReplyCode::ServerFailure);
    dnsServer.start(DNS_PORT, "*", apIP);

    dnsServer.processNextRequest();
    */

    Serial.println("SSID 'TELUS1846', Password: 'publics***' created");
    Serial.println("IP of access point is " + WiFi.softAPIP());
    //Serial.println("IP of Esp8266 is " + WiFi.localIP());
  }
  else
  {

    Serial.println("Looking for SSID 'TELUS1846', Password: 'publics***'");
    WiFi.begin(ssid, password);
    //Wait for connection to complete
    while (WiFi.waitForConnectResult() != WL_CONNECTED) {
      Serial.println("Connection Failed! Rebooting...");
      delay(5000);
      ESP.restart();
    }
    Serial.println("IP of Esp8266 is " + WiFi.localIP());
  }


  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  // ArduinoOTA.setHostname("myesp8266");

  // No authentication by default
  // ArduinoOTA.setPassword((const char *)"123");

  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
      digitalWrite(2,LOW); //Turns on LED
      delay(1);
      digitalWrite(2,HIGH); //Turns off LED
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void OTAloopOnce() {
  ArduinoOTA.handle();
}
