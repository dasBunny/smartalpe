byte getOverflow(){
  //always uses pin 5
  unsigned long measurements[3];
  digitalWrite(overflowSensorSwitch, HIGH);
  for(int i=0; i<3; i++){
    FreqCount.begin(1000);
    wdt_reset();
    while(!FreqCount.available()){}
    wdt_reset();
    measurements[i]=FreqCount.read();
    debug.println(measurements[i]);
    FreqCount.end();
    wdt_reset();
  }
  digitalWrite(overflowSensorSwitch, LOW);
  unsigned long avg_measurement = (measurements[0]+measurements[1]+measurements[2])/3;
  //Serial.println(avg_measurement);
 
  double waterheight = ((-0.0013*avg_measurement+122.4)/100)*(1.5/100);
//  Serial.println(waterheight);
  waterheight = waterheight+0.57735027*pow(((-0.0013*avg_measurement+122.4)/100),2.);
  waterheight = waterheight * 600;
//  Serial.println(waterheight);
 // double waterheight = 0.20261*pow(avg_measurement,4)-2.1898*pow(avg_measurement,3)+61.855*pow(avg_measurement,2)-1132.7*avg_measurement+102198;
  wdt_reset();
 if(waterheight > 255){
  //  Serial.println("Returning 1");
    return 1;
  }
  if(waterheight == 0){
  //  Serial.println("returning 0");
    return 0;
  }
  //Serial.print("last wh");
  //Serial.println(waterheight);
  int sd = (int) waterheight;
  //Serial.print("SD: ");
  //Serial.println(sd);
  return (byte)sd;
}
int getHeight(int mode){ //The algorithm is the same for both snow in winter mode and water in summer mode, only the mounting height can differ. mode=1 uses summer value, mode=2 uses winter
  digitalWrite(US_Switch, HIGH);
  wdt_reset();
  unsigned long pulse = pulseIn(US_In, HIGH); //Pulse length in microseconds, 1us = 1mm
  wdt_reset();
  debug.print("Raw pulse: ");
  debug.println(pulse);
  pulse = pulse / 10; //convert from mm to cm
  wdt_reset();
/*  if(pulse>51) return 0x00;
  pulse = 51 - pulse; //Get water/snow height based on measurement and defined mount height
  debug.print("Calculated height: ");
  debug.println(pulse);
 */ if(pulse>255) return 0xFF; //Any value above 255 is reduced to 255 to prevent problems when casting to byte. This should not happen if the sensor works properly
  debug.print("Return height: ");
  debug.println(pulse);
  return (byte) pulse;
}
