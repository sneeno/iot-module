/**************************************************************************/
/*
  Samantha Neeno
  Heat-Monitor IOT System: 6-1-2023

  Thermocouple output above a threshold will deliver an alert, and raw data is stored locally to a microSD card for future visualization 

  This code refers to:
   --- a Adafruit Feather M0 with a ATSAMD21 + ATWINC1500 
   --- a PCF8523 RTC connected via I2C and Wire lib with the INT/SQW pin wired to an interrupt-capable input.
   --- a MCP9700 Therocouple amplifier board
   --- a OLED FeatherWing 128x64
*/
/**************************************************************************/
// Import libraries:
#include "Wire.h"
#include "Adafruit_I2CDevice.h"
#include "Adafruit_I2CRegister.h"
#include "Adafruit_MCP9600.h"
#include "RTClib.h"
#include "SPI.h"
#include "SD.h"
#include "math.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
//#include <MemoryFree.h>
/************************** Configuration ***********************************/
// Edit the config.h tab and enter your Adafruit IO credentials
// and any additional configuration needed for WiFi connection
#include "config.h"
 
/************************ Define the IO Feed *******************************/
// Grab the TCProbe feed
AdafruitIO_Feed *TCprobe = io.feed("TCProbe");

Adafruit_SH1107 display = Adafruit_SH1107(64, 128, &Wire);

// OLED FeatherWing buttons map to different pins depending on board:
#if defined(ESP8266)
  #define BUTTON_A  0
  #define BUTTON_B 16
  #define BUTTON_C  2
#elif defined(ESP32) && !defined(ARDUINO_ADAFRUIT_FEATHER_ESP32S2)
  #define BUTTON_A 15
  #define BUTTON_B 32
  #define BUTTON_C 14
#elif defined(ARDUINO_STM32_FEATHER)
  #define BUTTON_A PA15
  #define BUTTON_B PC7
  #define BUTTON_C PC5
#elif defined(TEENSYDUINO)
  #define BUTTON_A  4
  #define BUTTON_B  3
  #define BUTTON_C  8
#elif defined(ARDUINO_NRF52832_FEATHER)
  #define BUTTON_A 31
  #define BUTTON_B 30
  #define BUTTON_C 27
#else // 32u4, M0, M4, nrf52840, esp32-s2 and 328p
  #define BUTTON_A  9
  #define BUTTON_B  6
  #define BUTTON_C  5
#endif

// Define Addresses and Pins:
#define I2C_ADDRESS (0x67)
const int chipSelect = 10;// // for the data logging shield, we use digital pin 10 for the SD cs line

// Define RTC and TC board:
RTC_PCF8523 rtc;
Adafruit_MCP9600 mcp;

// Define Booleans:
bool alert = false; 
bool screenbool = true; 

// Define Variables:
float MAXT = 30.0; // Maximum temperature target  
String filename; 
float TEMP; 

// Define Time Variables:
DateTime dtcurrent;

// the logging file(s)
File logfile;

#define SYNC_INTERVAL 1000 // mills between calls to flush() - to write data to the card
uint32_t syncTime = 0; // time of last sync()

#define  BUFFER_SIZE  10   // more readings = smoother data




// Declare functions before I use them: 
String print_time(DateTime ts) {
  char message[120];

  int Year = ts.year();
  int Month = ts.month();
  int Day = ts.day();
  int Hour = ts.hour();
  int Minute = ts.minute();
  int Second = ts.second();

  sprintf(message, "%d-%d-%d %02d:%02d:%02d", Month, Day, Year, Hour, Minute, Second);
  return message;
}

String createNewFile() {
  // function that saves per day to a new file
  char filename[] = "L000.CSV";
  int k;
  bool done = false;
  // Try to open files until you find a filename that does not exist
  for (int c = 0; c < 10; c++) { // Look for numbers between 0-999
    for (int i = 0; i < 100; i++) { 
      k = i + 100 * c;
      filename[1] = k / 100 + '0';
      filename[2] = i / 10 + '0';
      filename[3] = k % 10 + '0';

      if (! SD.exists(filename)) {
        Serial.println(filename);
        // only open a new file if it doesn't exist
        done = true;
        break;  // leave the loop!
      }
    }
    if (done) {
      break;
    }
  }
  return String(filename);//values;
}

void connect_AIO() {
  // wait for a connection
  while (io.status() < AIO_CONNECTED) {
    Serial.println(io.statusText());
    delay(500);
  }

  // we are connected
  Serial.println();
  Serial.println(io.statusText());
}

void tc_out(float temp){
  connect_AIO();

  Serial.println("Sending TCProbe value to feed..");
  TCprobe->save(temp);
  io.run();
}


void setup () {
  Serial.begin(9600);

// Initialize IO connection
  Serial.println("Connecting to Adafruit IO...");
  io.connect();

  // Set up RTC, TC Boards:
  Wire.begin();
  delay(3000);
  if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    while (1);
  }
  if (rtc.lostPower() || !rtc.initialized()) {
    Serial.println("RTC lost power, lets set the time!");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  /* Initialise the driver with I2C_ADDRESS and the default I2C bus. */
  if (! mcp.begin(I2C_ADDRESS)) {
    Serial.println("Sensor not found. Check wiring!");
    while (1);
  }
  Serial.println("Found MCP9600!");

  mcp.setADCresolution(MCP9600_ADCRESOLUTION_18);
  Serial.print("ADC resolution set to ");
  switch (mcp.getADCresolution()) {
    case MCP9600_ADCRESOLUTION_18:   Serial.print("18"); break;
    case MCP9600_ADCRESOLUTION_16:   Serial.print("16"); break;
    case MCP9600_ADCRESOLUTION_14:   Serial.print("14"); break;
    case MCP9600_ADCRESOLUTION_12:   Serial.print("12"); break;
  }
  Serial.println(" bits");

  mcp.setThermocoupleType(MCP9600_TYPE_K);
  Serial.print("Thermocouple type set to ");
  switch (mcp.getThermocoupleType()) {
    case MCP9600_TYPE_K:  Serial.print("K"); break;
    case MCP9600_TYPE_J:  Serial.print("J"); break;
    case MCP9600_TYPE_T:  Serial.print("T"); break;
    case MCP9600_TYPE_N:  Serial.print("N"); break;
    case MCP9600_TYPE_S:  Serial.print("S"); break;
    case MCP9600_TYPE_E:  Serial.print("E"); break;
    case MCP9600_TYPE_B:  Serial.print("B"); break;
    case MCP9600_TYPE_R:  Serial.print("R"); break;
  }
  Serial.println(" type");

  mcp.setFilterCoefficient(3);
  Serial.print("Filter coefficient value set to: ");
  Serial.println(mcp.getFilterCoefficient());

  mcp.enable(true);

  Serial.println(F("------------------------------"));

  // Define Pin Outputs:
  pinMode(LED_BUILTIN, OUTPUT);

  Serial.println(F("\nStarting Heat-Monitoring IOT System."));
  Serial.print(F("To start again from the top, hit the 'reset' button on the board: "));
  Serial.println(F("This will save data to the SD card as well!\n\n"));

  //////////////////////////////////////////////////////////////
  Serial.println("128x64 OLED FeatherWing test");
  delay(250); // wait for the OLED to power up
  display.begin(0x3C, true); // Address 0x3C default
  Serial.println("OLED begun");

  // Show image buffer on the display hardware.
  display.display();
  delay(1000);

  // Clear the buffer.
  display.clearDisplay();
  display.display();

  display.setRotation(1);
  Serial.println("Buttons are ready");

  pinMode(BUTTON_A, INPUT_PULLUP);
  pinMode(BUTTON_B, INPUT_PULLUP);
  pinMode(BUTTON_C, INPUT_PULLUP);

  // Text display tests
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(0,0);
  display.print("Heat-Monitoring System: Setting UP!':");
  display.print("---");
  display.print("---");
  display.display(); // actually display all of the above

  //////////////////////////////////////////////////////////////

  // Initialize the SD card
  Serial.print("Initializing SD card...");
  // Make sure that the default chip select pin is set to
  // output, even if you don't use it:
  pinMode(chipSelect, OUTPUT);

  // See if the card is present and can be initialized:
  int cnt = 0; 
  while(cnt < 6){
    delay(1000); 
    cnt++;   
    if (!SD.begin(chipSelect)) {
      Serial.print("Card failed, ");      Serial.print(cnt,DEC);     Serial.println("trying again");
    }
  }
  Serial.println("card initialized.");

  //////////////////////////////////////////////////////////////

  // Create local-storage file for temperature and alerts
  String filename = createNewFile();
  logfile = SD.open( filename , FILE_WRITE);

  if (! logfile) {
    Serial.println("couldnt create file");
  }
  if (! logfile) {
    screenbool = "false";  
  } else {
    screenbool = "true"; 
  }

  Serial.println("Logging to: ");
  Serial.println(filename);

  logfile.print("DateTime");                logfile.print(',');
  logfile.print("Temp");                    logfile.print(',');
  logfile.println("ALERT01");
}

void loop () { // In this loop, ever 100ms the Thermocouple temperature is read and logged. If temp is out of spec, an alert is displayed sent 
  // Pull current time
  dtcurrent = rtc.now();

  // Check there is a file which is being written to: this boolean is displayed on screen for troubleshooting 
  if (! logfile) {
    screenbool = "false";  
  } else {
    screenbool = "true"; 
  }

  // Read thermocouple 
  TEMP = mcp.readThermocouple();

  // Assign alert Boolean: if outside of threshold, it is TRUE
  if (TEMP >= MAXT ){
    alert = true;    
  } else {
    alert = false; 
  }

  // Send thermocouple data to AdafruitIO
  tc_out(TEMP);

  // Serial print timstamp, data, and boolean
  Serial.print(print_time(dtcurrent));        Serial.print('/');
  Serial.print("Temperature: ");              Serial.print('/');
  Serial.print(TEMP, DEC);                    Serial.print('/');
  Serial.print("Alert: ");                    Serial.print('/');    
  Serial.println(alert);           

  // Save timstamp, data, and boolean
  logfile.print(print_time(dtcurrent));       logfile.print(',');
  logfile.print(TEMP, DEC);                   logfile.print(',');
  logfile.println(alert);
  logfile.flush();
  
  if(!digitalRead(BUTTON_A)){
    // Unused functionality: Enable an alert
  }
  if(!digitalRead(BUTTON_B)) {
    // Unused functionality: Disable an alert
  }
  if(!digitalRead(BUTTON_C)) display.print("C");

  // Wait 6 seconds
  delay(6000);

  // Write Data and alerts to OLED display 
  display.clearDisplay();
  display.display();
  
  display.setCursor(0, 0);
  display.print(print_time(dtcurrent)); 
  display.print("    Temp:  "); 
  display.print(TEMP);
  display.print("         Alert: ");
  display.print(alert);
  display.print("               Logging Status: ");
  display.print(screenbool);
  display.print("          ");
  
  yield();
  display.display();

}
