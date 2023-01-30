#define Accelerometer true
#define PressureSensor false
#define Display false
#define useButton false
#define useAnalogSensors false

bool debugSerial = true;
bool forcePortal = false;
#define TRIGGER_PIN 0

// LIBRARIES
#include <Arduino.h>
#include <Wire.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager
#include <MicroOscUdp.h>          // Tof's MicroOSCUDP: https://github.com/thomasfredericks/MicroOsc > git checkout a02fa6fc4cd31b91247d93e0ed5e9c989297a127
#include <WiFiUdp.h>
#include <ESPmDNS.h>
#include <Preferences.h>

bool isConnectedW = false;
bool touchoscbridgeFound = false;
bool shouldSaveConfig = false;
IPAddress sendIp(192, 168, 0, 255); // <- default not really use, we are using Bonjour (mDNS) to find IP and PORT of touchoscbridge
unsigned int sendPort = 12101; // <- touchosc port
const long period = 100; //time between samples in milliseconds - 5
long startMillis = 0;
WiFiUDP udp;
MicroOscUdp<1024> oscUdp(&udp, sendIp, sendPort);
Preferences preferences;

//define your default values here
char cc_breathe_in[40];
char cc_breathe_out[40];
unsigned short pref_cc_breathe_out;
unsigned short pref_cc_breathe_in;

#include "accel.h"

//callback notifying us of the need to save config
void saveConfigCallback () {
  shouldSaveConfig = true;
}


void setup() {
  btStop();
  Serial.begin(115200);
  while (!Serial)
  {
  }
  preferences.begin("respire", false);

  pinMode(TRIGGER_PIN, INPUT_PULLUP);

  #if defined Accelerometer
    accelerometerSetup();
  #endif

  // CUSTOM CONFIGURATION
  WiFiManagerParameter custom_cc_breathe_in("MIDICCBI", "<h1>MIDI CC #</h1><div>Midi CC value for breathe in<br /><i>(ie: cutoff is MIDI CC: 74)", cc_breathe_in, 40);
  WiFiManagerParameter custom_cc_breathe_out("MIDICCBO", "Midi CC value for breathe out<br /><i>(ie: Breath Controller is MIDI CC: 2)</i></div>", cc_breathe_out, 40);

  // WiFiManager
  WiFiManager wifiManager;

  if(digitalRead(TRIGGER_PIN) == LOW || forcePortal) {
    wifiManager.resetSettings();
  }

  // Bonjour
  if (!MDNS.begin("Respire")) {
      Serial.println("Error setting up MDNS responder!");
      while(1){
          delay(1000);
      }
  }

  //set config save notify callback
  wifiManager.setSaveConfigCallback(saveConfigCallback);
  
  //add all your parameters here
  wifiManager.addParameter(&custom_cc_breathe_in);
  wifiManager.addParameter(&custom_cc_breathe_out);

  //fetches ssid and pass and tries to connect
  //if it does not connect it starts an access point with the specified name
  //and goes into a blocking loop awaiting configuration
  wifiManager.setConnectTimeout(20);
  if (!wifiManager.autoConnect("Respire")) {
    Serial.println("failed to connect and hit timeout");
    delay(3000);
    //reset and try again, or maybe put it to deep sleep
    ESP.restart();
    delay(5000);
  }

  //if you get here you have connected to the WiFi
  if (debugSerial) {
    Serial.println("Connected...yeey :)");
  }
  isConnectedW = true;

  //save the custom parameters to FS
  if (shouldSaveConfig) {
    if (debugSerial) {
      Serial.println("Saving config");
    }
    strcpy(cc_breathe_in, custom_cc_breathe_in.getValue());
    strcpy(cc_breathe_out, custom_cc_breathe_out.getValue());
    preferences.putUShort("cc_breathe_in", atoi(cc_breathe_in));
    preferences.putUShort("cc_breathe_out", atoi(cc_breathe_out));
  }
  
  // READ EEPROM
  pref_cc_breathe_in = preferences.getUShort("cc_breathe_in", 74);
  pref_cc_breathe_out = preferences.getUShort("cc_breathe_out", 2);
  
  if (debugSerial) {
    Serial.println();
    Serial.println("Reading EEPROM");
    Serial.print("cc_breathe_in: ");
    Serial.println(pref_cc_breathe_in);
    Serial.print("cc_breathe_out: ");
    Serial.println(pref_cc_breathe_out);
  }

  if (debugSerial) {
    Serial.println("local ip");
    Serial.println(WiFi.localIP());
  }
}

void loop() {
  startMillis = millis();
  if(isConnectedW) {
    // trying to search for touchosc bridge (via bonjour)
    if(!touchoscbridgeFound) {
        browseService("touchoscbridge", "udp");
    } else {
      // found touchosc bridge, send sensors values via OSC midi
      #if defined Accelerometer
        readAccelerometer();
      #endif

      /*
      // Example
      uint8_t midi[4];
      midi[0] = 0;
      midi[1] = 99;
      midi[2] = 10;  // expression
      midi[3] = 176;
      oscUdp.sendMessage("/midi",  "m",  midi);
      */
    }
  }
  while ((millis() - startMillis) < period);
}


// Bonjour
void browseService(const char * service, const char * proto){
    if (debugSerial) {
      Serial.printf("Browsing for service _%s._%s.local. ... ", service, proto);
    }
    int n = MDNS.queryService(service, proto);
    if (n == 0) {
      if (debugSerial) {
        Serial.println("no services found");
      }
    } else {
      
        if (debugSerial) {
          Serial.print(n);
          Serial.println(" service(s) found");
        }
        
        for (int i = 0; i < n; ++i) {
            oscUdp.setDestination(MDNS.IP(i), MDNS.port(i));
            if (debugSerial) {
              // Print details for each service found
              Serial.print("  ");
              Serial.print(i + 1);
              Serial.print(": ");
              Serial.print(MDNS.hostname(i));
              Serial.print(" (");
              Serial.print(MDNS.IP(i));
              Serial.print(":");
              Serial.print(MDNS.port(i));
              Serial.println(")");
            }
         }
         touchoscbridgeFound = true;
         
      }
}
