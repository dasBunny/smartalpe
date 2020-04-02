byte tempToByte(double tempIn) {
  byte tempOut=0;
  /*
   * Range: -50 to 50
   * -50 => 0
   * 50  => 200
   * 
   * 255: Error 
   */
//   Serial.println(tempIn);
   if(tempIn > 50 || tempIn<-50) return 255;
   if(tempIn >= 0) {
    tempIn = tempIn * 2 + 100.5;
//    Serial.println(tempIn);
    tempOut = (byte) tempIn;
   }
   else {
    tempIn = tempIn * -2 + 0.5;
    tempOut = (byte) tempIn;
   }
  return tempOut;
}
byte humidityToByte(double humidIn){
  if(humidIn>127 || humidIn<0) return 255;
  return (byte) humidIn*2 + 0.5;
}
