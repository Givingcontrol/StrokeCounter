/* REED testing sketch. 
 https://www.arduino.cc/en/Tutorial/Debounce
 https://github.com/cavemoa/Feather-M0-Adalogger
 https://learn.adafruit.com/adafruit-feather-m0-adalogger/adapting-sketches-to-m0
 https://stackoverflow.com/questions/29671455/how-to-split-a-string-using-a-specific-delimiter-in-arduino
 https://github.com/GabrielNotman/RTCZero

 181023: Battery function + screencountr
 181027: Writing to SDcard + reading out last results
 181030: Small fixes
 181101: Adding sleep mode on time
 181115: Splitting to several files 
 181117: Adding sleep mode on pin
 181118: Clean-up and LIVE CODE
 Renaming to StrokeCounter and addin it to github
 created LoRa branch? (nad learning github
*/
#include <Arduino.h>
#include <U8g2lib.h>
#include <SPI.h>
#include <SD.h>
#include <RTCZero.h> // Uses GabrielNotman's version of RTCZero which correctly sets 24Hr mode compared to main branch
#include <Wire.h>
#include <U8g2lib.h>
#include "logo.h"

// #define ECHO_TO_SERIAL // Allows serial output if uncommented
// #define PLOT_TO_SERIAL
// const int datename = 181118;

// initialize OLED with the numbers of the interface pins
U8G2_SH1106_128X64_NONAME_1_SW_I2C u8g2(U8G2_R0, 21, 20);
bool isOLEDOn = true;

#define VBATPIN A7    // The Feather battery pin
#define cardSelect 4  // Set the pins used fro SDcard

/////////////// Global Objects ////////////////////
RTCZero rtc;    // Create RTC object
File logfile;   // Create file object
File oldlogfile;   // Create file object

//////////////// Key Settings ///////////////////
/* Change these values to set the current initial time */
const byte hours = 00;
const byte minutes = 39;
const byte seconds = 0;
/* Change these values to set the current initial date */
const byte day = 20;
const byte month = 11;
const byte year = 18;

#define SampleIntSec 30 // RTC - Sample interval in seconds
int NextAlarmSec; // Variable to hold next alarm time in seconds

int totalStroke = 0;
int strokeSinceLub = 0;
const int resetPin = 12;     // the number of the pushbutton pin for resetting all to zero
const int lubPin = 11;       // the number of the pushbutton pin for resetting after lubrication
const int screenPin = 10;    // the number of the pushbutton pin to turn off the screen

int resetState = 0;         // variable for reading the pushbutton status
int lubState = 0;           // variable for reading the pushbutton status
int screenState = 0;        // variable for reading the pushbutton status

float measuredvbat = 0;                // battery variables  
unsigned long previousBattery = 0;     // the last time the battery readings was done
const long intervalBattery = 300000;    // 5 min interval time between battery readings

unsigned long previousSleep = 0;     // the last time the  readings was done
const long intervalSleep = 600000;    // 10 min  before going to sleep 

const int upperReedPin = 6;     // the number of the upper REED switch pin
const int lowerReedPin = 5;    // the number of the lower REED switch pin

int upperReedState = 0;
int lowerReedState = 0;
int upperReed = 0;

// Debounce of screen button
unsigned long lastDebounceTime = 0;  // the last time the output pin was toggled
unsigned long debounceDelay = 50;    // the debounce time; increase if the output flickers
int lastScreenState = HIGH;          // the previous reading from the input pin

int screenRefresh = 0;   // Dummy variable to see screen update during development

void setup(void) {
  pinMode(13, OUTPUT); //Onboard Red LED used for Error codes
  pinMode(8, OUTPUT);  //Onboard Green LED used for activity notification
  digitalWrite(13, LOW);
  digitalWrite(8, LOW);  
  delayBlink(1000,8);
  #ifdef ECHO_TO_SERIAL
    Serial.begin(115200);
    while (! Serial); // Wait until Serial is ready
    Serial.println("\r\nFeather M0 Analog logger");
  #endif
  #ifdef ECHO_TO_SERIAL
    Serial.println("Assigning OLED Screen");
  #endif    
  u8g2.begin();
  u8g2.setFont(u8g2_font_6x10_tr);      // set the font for the terminal window u8g2_font_6x10_tr
  //-- Loggig
  rtc.begin();    // Start the RTC in 24hr mode
  rtc.setTime(hours, minutes, seconds);   // Set the time
  rtc.setDate(day, month, year);    // Set the date
  // initialize the pushbutton pin as an input:
  pinMode(resetPin, INPUT_PULLUP);
  pinMode(screenPin, INPUT_PULLUP);  
  pinMode(lubPin, INPUT_PULLUP);
  pinMode(upperReedPin, INPUT_PULLUP);
  pinMode(lowerReedPin, INPUT_PULLUP);  
  //-- End Logging

  // Setting up the file for logging
  outputScreen("Setting up SDcard");
  
  #ifdef ECHO_TO_SERIAL
    Serial.println("Setting up SDcard");
  #endif
  // see if the card is present and can be initialized:
  if (!SD.begin(cardSelect)) {
    Serial.println("Card init. failed! or Card not present");
    error(2);     // Two red flashes means no card or card init failed.
  }
  char filename[15];
  char oldfilename[15];
  strcpy(filename, "WHCLOG00.CSV");
  for (uint8_t i = 0; i < 100; i++) {
    filename[6] = '0' + i/10;
    filename[7] = '0' + i%10;
    // create if does not exist, do not open existing, write, sync after write
    if (! SD.exists(filename)) {
      break;
    }
    else{
        strcpy(oldfilename, filename);      
    }
  }
  if (SD.exists(oldfilename)) {
    importValues(oldfilename);
  }
  logfile = SD.open(filename, FILE_WRITE);
  if( ! logfile ) {
    Serial.print("Couldnt create "); 
    Serial.println(filename);
    error(3);
  }
  SdOutput();
  #ifdef ECHO_TO_SERIAL
    Serial.print("Writing to "); 
    Serial.println(filename);
    Serial.println("Logging ....");  
    Serial.println("--------------------");
    Serial.println( "Compiled: " __DATE__ ", " __TIME__ ", " __VERSION__);
    Serial.println(__FILE__);
    Serial.println("--------------------");
  #endif  
  // picture loop
  u8g2.firstPage();
  do
  {
    u8g2.drawBitmap(0, 0, 16, 64, logo);
  }
  while (u8g2.nextPage()); 
  delay(5000);
  updateBattery();
  updateScreen();
}
 
void loop(void) {
    unsigned long currentMillis = millis();
    if (currentMillis - previousBattery >= intervalBattery) {
      updateBattery();
      updateScreen();
      previousBattery = currentMillis;       // save the last time battery statu was updated
    }  
    //-----------------------------------      
    int reading = digitalRead(screenPin);     // read the state of the screen pushbutton
    if (reading != lastScreenState) {         // If the switch changed, due to noise or pressing
      lastDebounceTime = millis();            // reset the debouncing timer
    }
    if ((millis() - lastDebounceTime) > debounceDelay) {   // whatever the reading is at, it's been there for longer than the debounce delay, so take it as the actual current state:
      if (reading != screenState) {                        // if the button state has changed:
        screenState = reading;
        if (screenState == LOW) {                          // only toggle the LED if the new button state is LOW
           if(isOLEDOn){
             u8g2.setPowerSave(1);                         // Enables PowerSave mode 
             isOLEDOn = !isOLEDOn;
            }
            else{
              u8g2.setPowerSave(0);                       // Disables PowerSave mode
              isOLEDOn = !isOLEDOn;
            }
            #ifdef ECHO_TO_SERIAL
              Serial.println("Screen button pushed");
            #endif  
        }
      }
    }
    lastScreenState = reading;        // save the reading. Next time through the loop, it'll be the lastresetState:
    //-----------------------------------
    resetState = digitalRead(resetPin);    // read the state of the pushbutton value:
    if (resetState == LOW) {               // check if the pushbutton is pressed. If it is, the resetState is LOW:
      totalStroke = 0;                     //resetting totalStroke value
      strokeSinceLub = 0;                  //resetting strokeSinceLub value
      updateBattery();
      updateScreen();
      SdOutput();                         // Output to uSD card
      #ifdef ECHO_TO_SERIAL
        SerialOutput();                   // Only logs to serial if ECHO_TO_SERIAL is uncommented at start of code
      #endif
      #ifdef PLOT_TO_SERIAL
        serialPlot();
      #endif 
      delay(50);  
    } 
    //-----------------------------------    
    lubState = digitalRead(lubPin);    
    if (lubState == LOW) {            // check if the pushbutton is pressed. If it is, the resetState is LOW:
      strokeSinceLub = 0;             //resetting only strokeSinceLub varialbe to zero
      updateBattery();
      updateScreen();
      SdOutput();                    // Output to uSD card
      #ifdef ECHO_TO_SERIAL
        SerialOutput();              // Only logs to serial if ECHO_TO_SERIAL is uncommented at start of code
      #endif      
      #ifdef PLOT_TO_SERIAL
        serialPlot();
      #endif
      delay(50);
    } 
    //-----------------------------------
    upperReedState = digitalRead(upperReedPin);     // read the state of the reed value:
    if (upperReedState == LOW && lowerReedState == HIGH){                    // check if the reed upper is active. If it is, the resetState is Low:
      upperReed = 1;
      #ifdef ECHO_TO_SERIAL
        Serial.print("Upper reed activated: ");                             // Only logs to serial if ECHO_TO_SERIAL is uncommented at start of code
        Serial.println(upperReed);
      #endif   
      delay(50);
    }
    //-----------------------------------
    lowerReedState = digitalRead(lowerReedPin);
    if (lowerReedState == LOW && upperReed == 1 && upperReedState == HIGH){   // check if the lower reed lower is active and  the upper reed has been activated before
      totalStroke++;
      strokeSinceLub++;
      upperReed = 0;
      #ifdef PLOT_TO_SERIAL
        serialPlot();
      #endif
      updateBattery ();
      updateScreen ();
      SdOutput();                                   // Output to uSD card
      #ifdef ECHO_TO_SERIAL
        Serial.print("Lower reed activated: "); 
        Serial.println(upperReed);       
        SerialOutput();                             // Only logs to serial if ECHO_TO_SERIAL is uncommented at start of code
      #endif   
      delay(50);
    }
    // Sleep Code
    unsigned long sleepMillis = millis();
    if (sleepMillis - previousSleep >= intervalSleep) {
      #ifdef ECHO_TO_SERIAL
        Serial.println("-------------------------------");    
        Serial.println("Enter Sleep mode");
        SerialOutput();
      #endif  
      outputScreen("Enter Sleep mode");
      delay(2000);      
      float intervaltimetest = currentMillis - previousSleep;
      #ifdef ECHO_TO_SERIAL
        Serial.print("intervaltimetest: ");
        Serial.println(intervaltimetest);
        Serial.print("intervalSleep: ");
        Serial.println(intervalSleep);
      #endif  
      u8g2.setPowerSave(1);                         // Enables screen PowerSave mode 
      isOLEDOn = !isOLEDOn;
      // Sleep Code
      NextAlarmSec = (rtc.getSeconds() + SampleIntSec) % 60;   // i.e. 65 becomes 5
      rtc.setAlarmSeconds(NextAlarmSec); // RTC time to wake, currently seconds only

      //following two lines enable alarm, comment both out if you want to do external interrupt      
      //rtc.enableAlarm(rtc.MATCH_SS); // Match seconds only
      //rtc.attachInterrupt(ISR); // Attaches function to be called, currently blank
      //comment out the below line if you are using RTC alarm for interrupt
      attachInterrupt(upperReedPin, ISR_UR, LOW);
      attachInterrupt(screenPin, ISR, LOW);
      #ifdef ECHO_TO_SERIAL
        Serial.end();
        USBDevice.detach(); // Safely detach the USB prior to sleeping  
      #endif    
      delay(500); // Brief delay prior to sleeping not really sure its required
      rtc.standbyMode();    // Sleep until next alarm match
      // Code re-starts here after sleep !
      delay(250);
      #ifdef ECHO_TO_SERIAL      
        USBDevice.attach();   // Re-attach the USB, audible sound on windows machines
      #endif
      detachInterrupt(upperReedPin);
      detachInterrupt(screenPin);
      #ifdef ECHO_TO_SERIAL
        Serial.begin(115200);
        while (! Serial); // Wait until Serial is ready
        Serial.println("Waking up");
      #endif
      u8g2.setPowerSave(0);                       // Disables PowerSave mode
      isOLEDOn = !isOLEDOn;
      //outputScreen("Wakeing up");
      //delayBlink(50,13);
      updateBattery();
      updateScreen();
      previousSleep = millis();       // save the last time  statu was updated
    }    
}

