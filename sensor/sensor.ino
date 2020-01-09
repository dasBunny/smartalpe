#include <SoftwareSerial.h>
#include <FreqCount.h>
#include <EEPROM.h>
//#include <DHT.h>

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

#define overflowSensorSwitch 7
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

unsigned long ms=0;

byte getOverflow(){
  //always uses pin 5
  unsigned long measurements[3];
  digitalWrite(overflowSensorSwitch, HIGH);
  for(int i=0; i<3; i++){
    FreqCount.begin(1000);
    while(!FreqCount.available()){}
    measurements[i-1]=FreqCount.read();
    debug.println(measurements[i-1]);
    FreqCount.end();
  }
  digitalWrite(overflowSensorSwitch, LOW);
  unsigned long avg_measurement = (measurements[0]+measurements[1]+measurements[2])/3;
  double waterheight = 0.20261*pow(avg_measurement,4)-2.1898*pow(avg_measurement,3)+61.855*pow(avg_measurement,2)-1132.7*avg_measurement+102198;
  byte of = 0;
  if(waterheight > 255) return 255;
  if(waterheight = 0) return 0;
  return (byte)waterheight;
}
int getHeight(int mode){ //The algorithm is the same for both snow in winter mode and water in summer mode, only the mounting height can differ. mode=1 uses summer value, mode=2 uses winter
  digitalWrite(US_Switch, HIGH);
  unsigned long pulse = pulseIn(US_In, HIGH); //Pulse length in microseconds, 1us = 1mm
  debug.print("Raw pulse: ");
  debug.println(pulse);
  pulse = pulse / 10; //convert from mm to cm
  if(pulse>EEPROM[mode]) return 0x00;
  pulse = EEPROM[mode] - pulse; //Get water/snow height based on measurement and defined mount height
  debug.print("Calculated height: ");
  debug.println(pulse);
  if(pulse>255) return 0xFF; //Any value above 255 is reduced to 255 to prevent problems when casting to byte. This should not happen if the sensor works properly
  debug.print("Return height: ");
  debug.println(pulse);
  return (byte) pulse;
}

int handleRequest(){
  debug.print("EEPROM 0: ");
  debug.println(EEPROM[0]);
  debug.print("EEPROM 1: ");
  debug.println(EEPROM[1]);
  debug.print("EEPROM 2: ");
  debug.println(EEPROM[2]);
  debug.print("EEPROM 3: ");
  debug.println(EEPROM[3]);
  byte return_array[6]; //Creating array that will be returned to the other arduino
  return_array[0]=0xFF; //control signal, the upper arduino will only use the bytes after reading 0xFF
  
  if(!EEPROM[0]){ //If not wintermode (0=winter, every other value is summer mode)
    return_array[wintermode_adr]='S'; //Set season indicator
    return_array[us_adr]=getHeight(1); //Getting the tank fill height
    if(return_array[us_adr]>tankFillThreshold){ //If tank is filled above the threshold the overflow will be measured
      return_array[overflow_adr]=getOverflow(); //getting overflow value
    }
    else return_array[overflow_adr]=0; //overflow is 0 when threshold is not reached
    }
  else { //If winter mode
    return_array[us_adr]=getHeight(2);
    return_array[overflow_adr]=0; //during winter no overflow is measured, thus this is 0
    return_array[wintermode_adr]='W'; //set season indicator
  }
  /*
   * PLACEHOLDER
   * Temperature and humidity data go here
   */
   
  dataTransmit.flush();
  for(int i=0; i<=5; i++){ //Transmitting the 6 (1 control + 5 data) bytes
    dataTransmit.write(return_array[i]); //Send all the bytes to the RF Arduino
    debug.println(return_array[i]); //send data to debug port as well
  }
  return 0;
}

//Code for the DHT sensors. Unused until other sensors are added to the hardware.
/*byte insideTemperature(){ 
  int temp = dht_object.readTemperature();
  Serial.print(F("Inside Temperature: "));
  Serial.print(temp);
  Serial.println(F("Celsius"));
  if(temp >= 127) return 127;
  if(temp <= -127) return 127;
  if(0 <= temp < 127) return (byte) temp;
  if(-127 < temp < 0) return (byte) 128 - temp;
  return 127;
}
byte insideHumidity(){
  int humid = dht_object.readHumidity();
  Serial.print(F("Inside Humidity: "));
  Serial.print(humid);
  Serial.println(F("%"));
  if(humid >= 127) return 127;
  if(humid < 0) return 127;
  if(0 <= humid < 127) return (byte) humid;
  return 255;
}*/

void setup() {
  //Pin setup
  pinMode(overflowSensorSwitch, OUTPUT);
  digitalWrite(overflowSensorSwitch, LOW);
  pinMode(US_In, 2);
  pinMode(13, OUTPUT); //Pin 13 is not used, but has an LED connected to it. 
  digitalWrite(13, LOW); //Setting this pin to low causes to LED to be off and saves energy.

  //Initialize Serial connections.
  dataTransmit.begin(9600);
  //debug.begin(9600);
  debug.println("Printing");

 EEPROM[3]=200;
 if(EEPROM[8]!=99){ //Setting EEPROM values to default if they have not previously been set. Unset EEPROM values could cause crashes.
  EEPROM[0]=false;
  EEPROM[1]=2000;
  EEPROM[8]=99; 
  //EEPROM[8] is set to 99 after the default values have been set. When the Arduino is powered on again 
  //the system knows the values have already been set and won't write the default settings. This prevents
  //unnecessary wear on the memory
 }
}

void loop() {
  if(debug.available()){ //Handles all commands send via the debug serial connection
    
  }
  if(dataTransmit.available()){ //This processes the commands send by the other Arduino
     if(dataTransmit.read()=='R'){ //If 'R' is received, measurements will be collected and send back
       debug.println("Data requested");
       handleRequest();
     }
     if(dataTransmit.read()=='S'){ //Set season to Summer
       EEPROM[0]=false;
     }
     if(dataTransmit.read()=='W'){ //Set season to Winter
       EEPROM[0]=true;
     }
   }
   
}
