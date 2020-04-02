int handleRequest(bool season){
  wdt_reset();
  debug.print("EEPROM 0: ");
  debug.println(EEPROM[0]);
//  debug.print("EEPROM 1: ");
//  debug.println(EEPROM[1]);
  debug.print("EEPROM 2: ");
  debug.println(EEPROM[2]);
  debug.print("EEPROM 3: ");
  debug.println(EEPROM[3]);
  byte return_array[7]; //Creating array that will be returned to the other arduino
  return_array[0]=0xFF; //control signal, the upper arduino will only use the bytes after reading 0xFF
  if(!season){ //If not wintermode (0=winter, every other value is summer mode)
    return_array[wintermode_adr]='S'; //Set season indicator
    return_array[us_adr]=getHeight(1); //Getting the tank fill height
//    if(return_array[us_adr]>tankFillThreshold){ //If tank is filled above the threshold the overflow will be measured
      if(true){
      return_array[overflow_adr]=getOverflow(); //getting overflow value
    }
    else return_array[overflow_adr]=0; //overflow is 0 when threshold is not reached
    }
  else { //If winter mode
    return_array[us_adr]=getHeight(2);
    return_array[overflow_adr]=0; //during winter no overflow is measured, thus this is 0
    return_array[wintermode_adr]='W'; //set season indicator
  }
  return_array[temperature_adr]=tempToByte(bme.readTemperature());
  return_array[humidity_adr]=humidityToByte(bme.readHumidity());
  return_array[6]=120;
  wdt_reset();
  dataTransmit.flush();
  wdt_reset();
  for(int i=0; i<=6; i++){ //Transmitting the 6 (1 control + 5 data) bytes
    dataTransmit.write(return_array[i]); //Send all the bytes to the RF Arduino
    debug.println(return_array[i]); //send data to debug port as well
  }
  return 0;
  wdt_reset();
}
