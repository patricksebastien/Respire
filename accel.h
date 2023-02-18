#include <ADXL345.h>
ADXL345 adxl; //variable adxl is an instance of the ADXL345 library
double ax,ay,az; // actual acceleration on all axis
int aIntx, aInty, aIntz; // Lose precision but hey it's MIDI!
int amIntx, amInty, amIntz; // Lose precision but hey it's MIDI!
int x,y,z; // pan tilt roll


/// Accelerometer ///
String storedAccelxChan;
String storedAccelxCC;
int storedAccelxChanInt;
int storedAccelxCCInt;

String storedAccelyChan;
String storedAccelyCC;
int storedAccelyChanInt;
int storedAccelyCCInt;

String storedAccelzChan;
String storedAccelzCC;
int storedAccelzChanInt;
int storedAccelzCCInt;

String storedPanChan;
String storedPanCC;
int storedPanChanInt;
int storedPanCCInt;

String storedTiltChan;
String storedTiltCC;
int storedTiltChanInt;
int storedTiltCCInt;

String storedRollChan;
String storedRollCC;
int storedRollChanInt;
int storedRollCCInt;

bool checkAccel = false;
bool checkPanTiltRoll = false;

bool readAccelerometer();

void accelerometerSetup(){
  
  adxl.powerOn();

  //set activity/ inactivity thresholds (0-255)
  adxl.setActivityThreshold(75); //62.5mg per increment
  adxl.setInactivityThreshold(75); //62.5mg per increment
  adxl.setTimeInactivity(10); // how many seconds of no activity is inactive?
 
  //look of activity movement on this axes - 1 == on; 0 == off 
  adxl.setActivityX(1);
  adxl.setActivityY(1);
  adxl.setActivityZ(1);
 
  //look of inactivity movement on this axes - 1 == on; 0 == off
  adxl.setInactivityX(1);
  adxl.setInactivityY(1);
  adxl.setInactivityZ(1);
 
  //look of tap movement on this axes - 1 == on; 0 == off
  adxl.setTapDetectionOnX(0);
  adxl.setTapDetectionOnY(0);
  adxl.setTapDetectionOnZ(1); // 1 
 
  //set values for what is a tap, and what is a double tap (0-255)
  adxl.setTapThreshold(50); //62.5mg per increment
  adxl.setTapDuration(15); //625us per increment
  adxl.setDoubleTapLatency(80); //1.25ms per increment
  adxl.setDoubleTapWindow(200); //1.25ms per increment
 
  //set values for what is considered freefall (0-255)
  adxl.setFreeFallThreshold(7); //(5 - 9) recommended - 62.5mg per increment
  adxl.setFreeFallDuration(45); //(20 - 70) recommended - 5ms per increment
 
  //setting all interrupts to take place on int pin 1
  //I had issues with int pin 2, was unable to reset it
  adxl.setInterruptMapping( ADXL345_INT_SINGLE_TAP_BIT,   ADXL345_INT1_PIN );
  adxl.setInterruptMapping( ADXL345_INT_DOUBLE_TAP_BIT,   ADXL345_INT1_PIN );
  adxl.setInterruptMapping( ADXL345_INT_FREE_FALL_BIT,    ADXL345_INT1_PIN );
  adxl.setInterruptMapping( ADXL345_INT_ACTIVITY_BIT,     ADXL345_INT1_PIN );
  adxl.setInterruptMapping( ADXL345_INT_INACTIVITY_BIT,   ADXL345_INT1_PIN );
 
  //register interrupt actions - 1 == on; 0 == off  
  adxl.setInterrupt( ADXL345_INT_SINGLE_TAP_BIT, 1);
  adxl.setInterrupt( ADXL345_INT_DOUBLE_TAP_BIT, 1);
  adxl.setInterrupt( ADXL345_INT_FREE_FALL_BIT,  1);
  adxl.setInterrupt( ADXL345_INT_ACTIVITY_BIT,   1);
  adxl.setInterrupt( ADXL345_INT_INACTIVITY_BIT, 1);
 
}

bool readAccelerometer() {

  /*
  adxl.readXYZ(&x, &y, &z); //read the accelerometer values and store them in variables  x,y,z
  x = map(x,-320,320,0,127); // Should be 355 degrees but in practice I don't see values above 320...
  y = map(y,-320,320,0,127);
  z = map(z,-320,320,0,127);
  */
  
  double xyz[3];
  //double ax,ay,az;
  adxl.getAcceleration(xyz);
  aIntx = int(xyz[0] * 100);
  aInty = int(xyz[1] * 100);
  aIntz = int(xyz[2] * 100);
  amIntx = map(aIntx,-110,57,0,127);
  amInty = map(aInty,-200,200,0,127);
  amIntz = map(aIntz,-200,200,0,127);

  uint8_t midi[4];
  if (sendSensorData) {
    // midi[1] = aInty; not useful in my case
    switch (sendAccel) {
        case 0:
          // default we don't send accel
        break;
        case 1:
          // x
          midi[0] = 0;
          midi[1] = aIntx;
          midi[2] = 3;  // expression
          midi[3] = 176;
          oscUdp.sendMessage("/midi",  "m",  midi);
        break;
        case 2:
          // z
          midi[0] = 0;
          midi[1] = aIntz;
          midi[2] = 4; // breath controller
          midi[3] = 176;
          oscUdp.sendMessage("/midi",  "m",  midi);
        break;
        case 3:
          // send x and z (not y)
          midi[0] = 0;
          midi[1] = aIntx;
          midi[2] = 3;  // expression
          midi[3] = 176;
          oscUdp.sendMessage("/midi",  "m",  midi);
  
          midi[0] = 0;
          midi[1] = aIntz;
          midi[2] = 4; // breath controller
          midi[3] = 176;
          oscUdp.sendMessage("/midi",  "m",  midi);
         break;
      }
  }
  
  if (debugSerial && sendSensorData) {
    // Output x,y,z values
    switch (sendAccel) {
      case 0:
        
      break;
      case 1:
        Serial.print("values of accel X: ");
        Serial.println(constrain(amIntx, 0, 127));
      break;
      case 2:
        Serial.print("values of z: ");
        Serial.println(aIntz);
      break;
      case 3:
        Serial.print("values of X , Z: ");
        Serial.print(aIntx);
        Serial.print(" , ");
        Serial.println(aIntz);
      break;
    }
  }


    // check interruptsource
    byte interrupts = adxl.getInterruptSource();
  
   //double tap
   //use as a modulo for selecting accelerometer output
   //0 - x, 1 - z, 2 - xz, 3 none
   if(adxl.triggered(interrupts, ADXL345_DOUBLE_TAP)){
     if (debugSerial) {
        Serial.println("double tap");
     }
     sendAccel = (sendAccel + 1) % 4;
     
   }
 
   //single tap
   //use to stop sending sensor values (useful to freeze position)
   if(adxl.triggered(interrupts, ADXL345_SINGLE_TAP)){
    // send only x, y, z or all of them (pressure sensor is always sent)
    if (debugSerial) {
      Serial.println("single tap");
    }
    sendSensorData = !sendSensorData;
   } 
  
  return 0;
} // End readAccelerometer
