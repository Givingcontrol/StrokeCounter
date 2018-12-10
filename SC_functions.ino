///////////////   Functions   //////////////////
void delayBlink(int d, int p){
  digitalWrite(p, HIGH);
  delay(d); //delay so we can see normal current draw
  digitalWrite(p, LOW);
  delay(d); //delay so we can see normal current draw
  digitalWrite(p, HIGH);
  delay(d); //delay so we can see normal current draw
  digitalWrite(p, LOW);         
}

void serialPlot(){
  Serial.print(totalStroke);
  Serial.print(" ");
  Serial.println(strokeSinceLub);
}

void updateBattery(){   // Battery voltage
    measuredvbat = analogRead(VBATPIN);
    measuredvbat *= 2;    // we divided by 2, so multiply back
    measuredvbat *= 3.3;  // Multiply by 3.3V, our reference voltage
    measuredvbat /= 1024; // convert to voltage
    //Serial.print("VBat: " ); Serial.println(measuredvbat);  
} 

void outputScreen(char message[]){
  u8g2.firstPage();
  do 
  {
    u8g2.setFontPosTop();                   
    u8g2.drawStr(4, 10, message);
    // u8g2.setCursor(70, 10);
    // u8g2.print(totalStroke);  
  }  
  while (u8g2.nextPage());
}

void updateScreen (){
  // screenRefresh++;  // Dummy variable to see screen update during development
  u8g2.firstPage();
  do 
  {
    u8g2.setFont(u8g2_font_7x14_tr);
    u8g2.setFontPosTop();
    //--------------------------                         
    u8g2.drawStr(4, 10, "Tot");
    u8g2.setCursor(70, 10);
    u8g2.print(totalStroke);  
    //-------------------------- 
    u8g2.drawStr(4, 30, "Lub");
    u8g2.setCursor(70, 30);
    u8g2.print(strokeSinceLub);                   
    // ----------------------------
    //u8g2.setCursor(4, 50);
    //u8g2.print(datename);       
    if(measuredvbat < 3.3){
      u8g2.setFont(u8g2_font_6x10_tr);
      u8g2.setFontPosTop();
      u8g2.setCursor(100, 50);
      u8g2.print(measuredvbat); // round(measuredvbat * 10) / 10.0);          
    }
  }
  while (u8g2.nextPage());        
}  

void SerialOutput() {                  // Debbugging output of time/date and stroke values
  Serial.print(rtc.getDay());
  Serial.print("/");
  Serial.print(rtc.getMonth());
  Serial.print("/");
  Serial.print(rtc.getYear()+2000);
  Serial.print(" ");
  Serial.print(rtc.getHours());
  Serial.print(":");
  if(rtc.getMinutes() < 10)
    Serial.print('0');                 // Trick to add leading zero for formatting
  Serial.print(rtc.getMinutes());
  Serial.print(":");
  if(rtc.getSeconds() < 10)
    Serial.print('0');                 // Trick to add leading zero for formatting
  Serial.print(rtc.getSeconds());
  Serial.print(";");
  Serial.print(measuredvbat);         // Print battery voltage  
  Serial.print(";");
  Serial.print(totalStroke);          // Print battery voltage  
  Serial.print(";");
  Serial.println(strokeSinceLub);     // Print battery voltage  
}

void importValues(char myMsg[]){
  oldlogfile = SD.open(myMsg);
  char lastResults[32];
  int k = 0;
  if (oldlogfile) {
    while (oldlogfile.available()) {
      lastResults[k] = oldlogfile.read();
      // Serial.println(lastResults[k]);
      k = (k + 1) % 32;   //  operator rolls over variable
    }
    oldlogfile.close();
  }
  // if the file isn't open, pop up an error:
  else {
    Serial.println("error opening Old file");
  }
  String myString = String(lastResults);
  String value0 = getValue(myString, ';', 0);
  String value1 = getValue(myString, ';', 1);
  String value2 = getValue(myString, ';', 2);
  String value3 = getValue(myString, ';', 3);
  totalStroke = value2.toInt();
  strokeSinceLub = value3.toInt();
  //Serial.println(totalStroke);  
}

void SdOutput() {   // Print data and time followed by battery voltage to SD card
  /* Force data to SD and update the directory entry to avoid data loss.
  if (!file.sync() || file.getWriteError()) {
    error("write error");
    error(3);     // Three red flashes means write failed.
  }
  */

  // Formatting for file out put dd/mm/yyyy hh:mm:ss; [sensor output]
  logfile.print(rtc.getDay());
  logfile.print("/");
  logfile.print(rtc.getMonth());
  logfile.print("/");
  logfile.print(rtc.getYear()+2000);
  logfile.print(" ");
  logfile.print(rtc.getHours());
  logfile.print(":");
  if(rtc.getMinutes() < 10)
    logfile.print('0');                 // Trick to add leading zero for formatting
  logfile.print(rtc.getMinutes());
  logfile.print(":");
  if(rtc.getSeconds() < 10)
    logfile.print('0');                // Trick to add leading zero for formatting
  logfile.print(rtc.getSeconds());
  logfile.print(";");
  logfile.print(measuredvbat);         // Print battery voltage
  logfile.print(";");
  if(totalStroke < 10)
    logfile.print('0');               // Trick to add leading zero for formatting  
  logfile.print(totalStroke);         // Print totalStroke voltage
  logfile.print(";");
  if(strokeSinceLub < 10)
    logfile.print('0');              // Trick to add leading zero for formatting  
  logfile.println(strokeSinceLub);   // Print strokeSinceLub voltage    
  logfile.flush();
}

void error(uint8_t errno) { // blink out an error code
  while(1) {
    uint8_t i;
    for (i=0; i<errno; i++) {
      digitalWrite(13, HIGH);
      delay(100);
      digitalWrite(13, LOW);
      delay(100);
    }
    for (i=errno; i<10; i++) {
      delay(200);
    }
  }
}

String getValue(String data, char separator, int index){ //sorts out part of string by a deliminator
  int found = 0;
  int strIndex[] = {0, -1};
  int maxIndex = data.length()-1;

  for(int i=0; i<=maxIndex && found<=index; i++){
    if(data.charAt(i)==separator || i==maxIndex){
        found++;
        strIndex[0] = strIndex[1]+1;
        strIndex[1] = (i == maxIndex) ? i+1 : i;
    }
  }

  return found>index ? data.substring(strIndex[0], strIndex[1]) : "";
}

void ISR(){ // Do something when interrupt called

}
void ISR_UR(){ // Do something when interrupt called
   upperReed = 1;
}

