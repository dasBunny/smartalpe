#include <SoftwareSerial.h>
#include <FreqCount.h>
#include <EEPROM.h>
#include <avr/wdt.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
//#include <OneWire.h>
//#include <DS18B20.h>

/*  EEPROM ADRESSES
 * 0 Wintermode TRUE = winter, FALSE = summer
 * 1 Ultrasonic sensor height above bottom of tank in cm
 * 2 Ultrasonic sensor height above ground in winter mode
 * 3 Minimal tank water height for the overflow sensor to be activated
 * 4 
 * 5
 * 6
 * 7
 * 8 This is set to 99 if EEPROM has been set previously
 */

#define overflowSensorSwitch 4
#define US_Switch 3
#define US_In 8
#define overflowSensorIn A0
#define dataTransmit Serial
#define debug debugSS
#define tankFillThreshold EEPROM[3]

//DEFINE ARRAY POSITIONS
#define us_adr 1
#define overflow_adr 2
#define humidity_adr 3
#define temperature_adr 4
#define wintermode_adr 5

SoftwareSerial debugSS(6,7);
Adafruit_BME280 bme;

unsigned long ms=0;

void setup() {
  //Pin setup
  pinMode(overflowSensorSwitch, OUTPUT);
  digitalWrite(overflowSensorSwitch, LOW);
  pinMode(US_In, INPUT);
  pinMode(13, OUTPUT); //Pin 13 is not used, but has an LED connected to it. 
  digitalWrite(13, LOW); //Setting this pin to low causes to LED to be off and saves energy.

  //Initialize Serial connections.
  dataTransmit.begin(9600);
  debug.begin(9600);
  debug.println("Printing");
  bme.begin(0x76);
  if(EEPROM[8]!=99){ //Setting EEPROM values to default if they have not previously been set. Unset EEPROM values could cause crashes.
     EEPROM[3]=200;
     EEPROM[0]=false;
     EEPROM[1]=51;
     EEPROM[2]=51;
     EEPROM[8]=99; 
  //EEPROM[8] is set to 99 after the default values have been set. When the Arduino is powered on again 
  //the system knows the values have already been set and won't write the default settings. This prevents
  //unnecessary wear on the memory
 }
}

void loop() {
  wdt_enable(WDTO_4S);
    // put your main code here, to run repeatedly:
  wdt_reset();
//  delay(3000);
//  wdt_reset();
//  Serial.println(bme.readTemperature());
//  Serial.println(tempToByte(bme.readTemperature()));
//  Serial.println(bme.readHumidity());
//  Serial.println(humidityToByte(bme.readHumidity()));
  if(debug.available()){ //Handles all commands send via the debug serial connection
  }
  if(dataTransmit.available()){ //This processes the commands send by the other Arduino
//     if(dataTransmit.read()=='R'){ //If 'R' is received, measurements will be collected and send back
//       debug.println("Data requested");
//       handleRequest();
//     }
     if(dataTransmit.read()=='S'){ //Set season to Summer
       debug.println("Data requested: Summer");
       handleRequest(false);
     }
     if(dataTransmit.read()=='W'){ //Set season to Winter
       debug.println("Data requested: Winter");
       handleRequest(true);
     }
   }
   wdt_reset();
}
