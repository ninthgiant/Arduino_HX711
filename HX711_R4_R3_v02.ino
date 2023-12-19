/*
    FILE: HX711_R4_R3_v01
    AUTHOR: RAM
    PURPOSE: Handle load cell data collection using the HX711 amplifier
    STATUS: runs on both R4 and R3 versions. Tested 12/13/2023

    this version has working components hi-speed looping while writing to open file
    - to change: clean so doesn't use as much space
    - make saving data faster by only accessing RTC once per loop, put that time stamp on every one in that loop - needed?
    - set the rtc to the computer time - uses RTCLib (not RTC)

    on R4 compile: 
    Sketch uses 74148 bytes (28%) of program storage space. Maximum is 262144 bytes.
    Global variables use 6672 bytes (20%) of dynamic memory, leaving 26096 bytes for local variables. Maximum is 32768 bytes. 
    
    on R3 compile:
    Sketch uses 20266 bytes (62%) of program storage space. Maximum is 32256 bytes.
    Global variables use 1645 bytes (80%) of dynamic memory, leaving 403 bytes for local variables. Maximum is 2048 bytes.

*/

// include the library code: from YWRobot Hello World - same as used in MOM apps
#include <Wire.h>                // last build in 2023 and is R4 compatible - enables I2C which may be why next library works
#include <LiquidCrystal_I2C.h>.  // not explcitly R4 compatible, older library, but seems to work. However, the HD44780 compatible by Tacuna is compatable with LiquidCrystal library which is good with R4

LiquidCrystal_I2C lcd(0x27, 20, 4);  // set the LCD address to 0x27 for a 16 chars and 2 line display

/// SD card libraries
// include headers for t//he SD card communications
// both are official Arduino libraries so are R4 compatible
#include <SPI.h>
#include <SD.h>
String myFilename;

// Include the RTC library - works with R4 - from Arduino examples certified to work on all platforms
#include "Time.h"
#include "RTClib.h"
RTC_DS1307 RTC;  // works on both boards we have

// Setup time variables that will be used in multiple places
DateTime currenttime;
long int myUnixTime;

// HX711 library - updated Nov 2023 so is R4 compatible by RobTillaart on Github
#include "HX711.h"
HX711 scale;
long int strain;

int tCounter = 0;  // Count # loops we to thru - must be global because don't want to initialize each time

// set constants
// initialize variables for SD -- use chipSelect = 4 without RTC board
const int chipSelect = 10;  // for the Wigoneer board and Adafruit board
// set the communications speed
const int comLevel = 115200;
// set the delay amount
const int dataDelay = 0;
// set flag for amount of feedback - false means to give us too much info
const bool verbose = true;
// set flag for printing to screen - could automate this when check for serial
const bool printScreen = true;
// set flag for saving to disk
const bool saveData = true;
// set flag for printing to LCD
const bool printLCD = true;
// set flag to use the RTC
const bool useRTC = true;
// set flag to use the RTC
const bool debug = true;
// set flag to use the RTC
const bool addStamp = true;



////////////////////
//  Get_Data - encapsulate data access to test different ideas - for now, very simple
////
long int Get_Data() {
  return (scale.read());
}

////////////////////
//  Get_TimeStamp - encapsulate data in case we change libraries
////
long int Get_TimeStamp() {
  /* GET CURRENT TIME FROM RTC */
  currenttime = RTC.now();

  // convert to raw unix value for return - could give options
  return (currenttime.unixtime());
}

///////////////////
// File name function - based on RTC time - so that we have a new filename for every day "DL_MM_DD.txt"
/////////
String rtnFilename() {

  DateTime currenttime;

  currenttime = RTC.now();

  int MM = currenttime.month();
  int DD = currenttime.day();

  String formattedDateTime = "DL_";
  formattedDateTime += (MM < 10 ? "0" : "") + String(MM) + "_";
  formattedDateTime += (DD < 10 ? "0" : "") + String(DD);
  formattedDateTime += ".txt";

  return formattedDateTime;
}


void setup() {

  Serial.begin(115200);


  ///////////////////////////
  // setup LCD
  ///////////////////////////
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0,0);
  lcd.print("Checking...");  // remove this when actually checking
  delay(1000);
  lcd.setCursor(0,0);
  lcd.print("                ");  // remove this when actually checking

  ///////////////////////////
  // RTC - Setup - turn on and off with flag
  ///////////////////////////
  if (useRTC) {  // must do this first so that we can use the GetFileName when we want it

    Serial.println("RTC setup");
    if (!RTC.begin()) {  // initialize RTC
      Serial.println("RTC failed");
    } else {
      if (!RTC.isrunning()) {
        Serial.println("RTC is NOT running!");
        // following line sets the RTC to the date & time this sketch was compiled
        RTC.adjust(DateTime(F(__DATE__), F(__TIME__)));
      } else {
        RTC.adjust(DateTime(F(__DATE__), F(__TIME__)));
        Serial.println("RTC is already initialized.");
      }
    }
    // need to check to see if the clock is correct, or always update to internal clock
  }

  ///////////////////////////
  // setup SD Card - turn on and off with flag
  ///////////////////////////
  if (saveData) {

    if (debug) {
      myFilename = "Test_013.txt";  // use this if you want a custom name
      // myFilename = rtnFilename();  // use this if debugging and want to have the autonamed file - NEED RTC running to make it work
    } else {
      myFilename = rtnFilename();
    }

    // setup lcd for user feedback
    lcd.setCursor(0, 0);
    lcd.print("Initialize card ");

    // initialize the SD card process
    Serial.print("Initializing SD card...");

    delay(1000);

    // see if the card is present and can be initialized:
    if (!SD.begin(chipSelect)) {
      Serial.println("Card failed, or not present");  // don't do anything more:
      lcd.print("                ");
      lcd.setCursor(1, 0);
      lcd.print("Card failed!");
      lcd.setCursor(1, 1);
      lcd.print("Disconnect!");
      delay(10000);
      // turn off the lcd
      lcd.noBacklight();
      lcd.noDisplay();


      while (1)
        ;
      Serial.println("card initialized.");  // confirm that it is good to go
    }                                       // end of checking for card and initializing

  }  // end of if SaveDATA

  ///////////////////////////
  // Instantiate Load Cell/amplifier
  ///////////////////////////
  // Make the rest of these local variables? Could make these local. for now, leave it
  uint8_t dataPin = 3;
  uint8_t clockPin = 2;
  scale.begin(dataPin, clockPin);

  if (verbose) {  // give full feedback on status

    float wt_Check01;  // track current weight
    float wt_Check02;  // track the raw values from load cell
    float wt_Check03;  // track the raw values from load cell

    Serial.println("Before setting up the scale:");
    Serial.print("read: \t\t");
    Serial.println(scale.read());  // print a raw reading from the ADC

    Serial.print("read average: \t\t");
    Serial.println(scale.read_average(20));  // print the average of 20 readings from the ADC

    wt_Check01 = scale.read();
    Serial.print("Check 01: \t\t");
    Serial.println(wt_Check01);
    delay(200);
    wt_Check02 = scale.read();
    Serial.print("Check 02: \t\t");
    Serial.println(wt_Check02);
    delay(200);
    wt_Check03 = scale.read();
    Serial.print("Check 03: \t\t");
    Serial.println(wt_Check03);
    delay(200);

    if ((wt_Check01 == wt_Check02) & (wt_Check02 == wt_Check03)) {
      Serial.print("We have a problem; load cell always reads: ");
      Serial.println(wt_Check01);
      lcd.setCursor(1, 0);
      lcd.print("Load cell Problem!");
      lcd.setCursor(1, 1);
      lcd.print("Disconnnect!    ");
      delay(10000);
    } else {
      lcd.setCursor(0, 0);
      lcd.print("Load cell works");
      Serial.println("Load cell working properly.");
      delay(1000);
    }

    int N = 10;
    for (int i = 1; i < N; i++) {
      lcd.setCursor(0, 1);
      lcd.print("Start in: ");
      lcd.print(N - i);
      lcd.print(" secs");
      delay(1000);
    }
  }

  // turn off the lcd?
  if (!printLCD) {
    lcd.noBacklight();
    lcd.noDisplay();
  } else {
    lcd.setCursor(0, 0);
    lcd.print("Data:           ");
    lcd.setCursor(0, 1);
    lcd.print("                ");
  }
}


void loop() {
  tCounter = tCounter + 1;
  unsigned long strain;

    // get a single time stamp to put on all 300 data points - not sure worth the speed to lose the resolution
  myUnixTime = Get_TimeStamp();
    // open the file to write 300 data points
  File dataFile = SD.open(myFilename, FILE_WRITE);

  if (dataFile) {

    for (int i = 0; i < 300; i++) {

      strain = Get_Data();

      if (addStamp) {  // only adds it to the saved data
        dataFile.print(strain);
        dataFile.print(", ");  // do we want to add the time stamp
        dataFile.println(myUnixTime);

        // datafile.println(currenttime.getUnixTime());  // for now, do it every time. let's see how it goes
      } else {
        dataFile.print(strain);
        dataFile.println(", ");  // do we want to add the time stamp
      }


      if (debug) {
        Serial.print(strain);
        if (i < 1) {
          Serial.print(",");
          Serial.println(tCounter);
        } else {
          Serial.print(",");
          Serial.println(Get_TimeStamp());
        }

      } else {
        Serial.println(",");
      }  // end of if debug



    }  // end of for loop; data file being open

    // show user we are collecting data
    lcd.setCursor(6, 0);
    lcd.print(strain);  // do this while we are messing with closing the datafile

    dataFile.close();  // at the end, close before we start another loop


  } else {
    Serial.println("Error opening file ");  //  + myFile);  // if the file isn't open, pop up an error:
    lcd.setCursor(0, 0);
    lcd.print("Can't open file!!");  // do this while we are messing with closing the datafile
    delay(2000);
    lcd.setCursor(0, 1);
    lcd.print("Figure it out!");  // do this while we are messing with closing the datafile
    delay(10000);
  }
}


// -- END OF FILE --
