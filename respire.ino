#define Accelerometer true
#define PressureSensor false
#define Display false
#define useButton false
#define useAnalogSensors false

bool debugSerial = true;
#define TRIGGER_PIN 0

// LIBRARIES
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager
#include <MicroOscUdp.h>          // Tof's MicroOSCUDP: https://github.com/thomasfredericks/MicroOsc > git checkout a02fa6fc4cd31b91247d93e0ed5e9c989297a127
#include <WiFiUdp.h>
#include <ESPmDNS.h>

#include "accel.h"

bool isConnectedW = false;
bool touchoscbridgeFound = false;
IPAddress sendIp(192, 168, 0, 255); // <- default not really use, we are using Bonjour (mDNS) to find IP and PORT of touchoscbridge
unsigned int sendPort = 12101; // <- touchosc port
WiFiUDP udp;
MicroOscUdp<1024> oscUdp(&udp, sendIp, sendPort);

//define your default values here
char cc_breathe_in[40];
char cc_breathe_out[40];

//flag for saving data
bool shouldSaveConfig = false;

//callback notifying us of the need to save config
void saveConfigCallback () {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}


void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.println();

  pinMode(TRIGGER_PIN, INPUT_PULLUP);

  #if defined Accelerometer
    accelerometerSetup();
  #endif

  // CUSTOM CONFIGURATION
  WiFiManagerParameter custom_cc_breathe_in("MIDI", "Midi CC value for breathe in (ie: cutoff is MIDI CC: 74)", cc_breathe_in, 40);
  WiFiManagerParameter custom_cc_breathe_out("MIDI", "Midi CC value for breathe out (ie: Breath Controller is MIDI CC: 2)", cc_breathe_out, 40);

  //WiFiManager
  WiFiManager wifiManager;

  if(digitalRead(TRIGGER_PIN) == LOW) {
  wifiManager.resetSettings();
  }
  // debug force reset
  //wifiManager.resetSettings();

  
  //set config save notify callback
  wifiManager.setSaveConfigCallback(saveConfigCallback);
  
  //add all your parameters here
  wifiManager.addParameter(&custom_cc_breathe_in);
  wifiManager.addParameter(&custom_cc_breathe_out);

  //reset settings - for testing
  //wifiManager.resetSettings();

  //set minimu quality of signal so it ignores AP's under that quality
  //defaults to 8%
  //wifiManager.setMinimumSignalQuality();

  //sets timeout until configuration portal gets turned off
  //useful to make it all retry or go to sleep
  //in seconds
  //wifiManager.setTimeout(120);

  //fetches ssid and pass and tries to connect
  //if it does not connect it starts an access point with the specified name
  //here  "AutoConnectAP"
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
  Serial.println("connected...yeey :)");
  isConnectedW = true;

  //read updated parameters
  strcpy(cc_breathe_in, custom_cc_breathe_in.getValue());
  strcpy(cc_breathe_out, custom_cc_breathe_out.getValue());
  Serial.println("The values in the file are: ");
  Serial.println("\tcc_breathe_in : " + String(cc_breathe_in));
  Serial.println("\tcc_breathe_out : " + String(cc_breathe_out));

  //save the custom parameters to FS
  if (shouldSaveConfig) {
    Serial.println("saving config");
    Serial.println(cc_breathe_in);
    Serial.println(cc_breathe_out);
    //TODO save in eeprom....
  }

  Serial.println("local ip");
  Serial.println(WiFi.localIP());

  
}

void loop() {
  if(isConnectedW) {
    Serial.println("...");

    // trying to search for touchosc bridge (via bonjour)
    if(!touchoscbridgeFound) {
        browseService("touchoscbridge", "udp");
    } else {
      // found touchosc bridge, send sensors values via OSC midi
      Serial.println("Sending sensors values");

      // Example
      uint8_t midi[4];
      midi[0] = 0;
      midi[1] = 99;
      midi[2] = 10;  // expression
      midi[3] = 176;
      oscUdp.sendMessage("/midi",  "m",  midi);

      #if defined Accelerometer
        readAccelerometer();
      #endif
    }
  }
  delay(500); // too long refacto implementation
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
