/*
  CameronTech.io Inclinometer

  Thanks to http://bildr.org/2011/04/sensing-orientation-with-the-adxl335-arduino
  for finding the correct formula for converting the ADXL335 voltage to angles.

  ------------------------------------------------------------------------------

  Serial Enabled LCD Kit
  by: Jim Lindblom - SparkFun Electronics
  Characters received in a serial stream are displayed on a 16x2,
  20x4, etc. LCD. Special commands are also available to clear, scroll
  display cursors and other functions.

  License: Beerware - As long as you retain this notice you
  can do whatever you want with this stuff. If we meet some day,
  and you think this stuff is worth it, you can buy
  me a beer in return.

  Revisions:
  9/29/10 - Added support for 20x4, 16x4 and 20x2 LCDs.
  See special commands 0x03 through 0x06. LCD size is stored
  in EEPROM and retained upon loss of power.

  The circuit:
  * LCD RS pin to digital pin 2
  * LCD R/W pin to digital pin 3
  * LCD Enable pin to digital pin 4
  * LCD D4 pin to digital pin 5
  * LCD D5 pin to digital pin 6
  * LCD D6 pin to digital pin 7
  * LCD D7 pin to digital pin 8
  * 10K resistor:
  * ends to +5V and ground
  * wiper to LCD VO pin (pin 3)

  Uses the LiquidCrystal Arduino library to control the LCD
  ---------------------------------------------------------
  http://www.arduino.cc/en/Tutorial/LiquidCrystal
  Much thanks to: Limor Fried, Tom Igoe, David A. Mellis
  for their fantastic work on this library!
*/

#include <LiquidCrystal.h>
#include <EEPROM.h>

const bool DEBUG = true;

// analog input pins
const int X_PIN = 0;
const int Y_PIN = 1;
const int Z_PIN = 2;

// max/min analog values
const int MIN = 405;
const int MAX = 615;

// ASCII character for degree symbol
const int DEGREE = 223;

// --- EEPROM ADDRESS DEFINITIONS
#define LCD_BACKLIGHT_ADDRESS 1  // EEPROM address for backlight setting
#define BAUD_ADDRESS 2  // EEPROM address for Baud rate setting
#define SPLASH_SCREEN_ADDRESS 3 // EEPROM address for splash screen on/off
#define ROWS_ADDRESS 4  // EEPROM address for number of rows
#define COLUMNS_ADDRESS 5  // EEPROM address for number of columns

// --- SPECIAL COMMAND DEFINITIONS
#define BACKLIGHT_COMMAND 128  // 0x80
#define SPECIAL_COMMAND 254 // 0xFE
#define BAUD_COMMAND 129  // 0x81

// --- ARDUINO PIN DEFINITIONS
uint8_t RSPin = 2;
uint8_t RWPin = 3;
uint8_t ENPin = 4;
uint8_t D4Pin = 5;
uint8_t D5Pin = 6;
uint8_t D6Pin = 7;
uint8_t D7Pin = 8;
uint8_t BLPin = 9;

char inKey;  // Character received from serial input
uint8_t Cursor = 0;  // Position of cursor, 0 is top left, (rows*columns)-1 is bottom right
uint8_t LCDOnOff = 1;  // 0 if LCD is off
uint8_t blinky = 0;  // Is 1 if blinky cursor is on
uint8_t underline = 0; // Is 1 if underline cursor is on
uint8_t splashScreenEnable = 1;  // 1 means splash screen is enabled
uint8_t rows = 2;  // Number rows, will be either 2 or 4
uint8_t columns = 16; // Number of columns, will be 16 or 20
uint8_t characters; // rows * columns

// initialize the LCD at pins defined above
LiquidCrystal lcd(RSPin, RWPin, ENPin, D4Pin, D5Pin, D6Pin, D7Pin);

/* ----------------------------------------------------------
  In the setup() function, we'll read the previous baud,
  screen size, backlight brightness, and splash screen state
  from EEPROM. Serial will be started at the proper baud, the
  LCD will be initialized, backlight turned on, and splash
  screen displayed (or not) according to the EEPROM states.
  ----------------------------------------------------------*/
void setup(){
  //////////////////////
  // LCD setup
  //////////////////////

  // initialize the serial communications:
  if (DEBUG) {
    Serial.begin(9600);
  }

  // set up the LCD's number of rows and columns:
  lcd.begin(16, 2);

  // Set up the backlight
  pinMode(BLPin, OUTPUT);
  analogWrite(BLPin, 255);

  // Splashscreen
  lcd.print(" CameronTech.io ");
  lcd.setCursor(0, 1);
  lcd.print("  Inclinometer  ");
  delay(2000);
  lcd.clear();

  //////////////////////
  // Inclinometer setup
  //////////////////////

  // set analogRead max to 3.3v instead of default 5v
  // analogReference(EXTERNAL);

  // write header
  lcd.print("  PITCH   ROLL  ");
}

void loop() {

  // sample the voltages
  int x = analogRead(X_PIN);
  int y = analogRead(Y_PIN);
  int z = analogRead(Z_PIN);

  // convert to range of -90 to +90 degrees
  int xAng = map(x, MIN, MAX, -90, 90);
  int yAng = map(y, MIN, MAX, -90, 90);
  int zAng = map(z, MIN, MAX, -90, 90);

  // convert radians to degrees
  int pitch = RAD_TO_DEG * (atan2(-xAng, -zAng) + PI);
  int roll = RAD_TO_DEG * (atan2(-yAng, -zAng) + PI);

  // convert left roll and forward pitch to negative degrees
  if (pitch > 180) {
    pitch = pitch - 360;
  }
  if (roll > 180) {
    roll = roll - 360;
  }

  // write the pitch and roll to the second line
  updateDisplay(pitch, roll);

  // Wait a quarter second so the numbers aren't flashing so fast
  delay(250);

}


void updateDisplay(int pitch, int roll) {
  // Move to the start of the second line
  lcd.setCursor(0,1);

  // convert int values to strings for output
  String pitchString = String(pitch);
  String rollString = String(roll);

  // pad spaces before pitch value
  String output = "  ";
  if (pitchString.length() < 4) {
    output += " ";
  }
  if (pitchString.length() == 1) {
    output += " ";
  }
  output += pitchString;

  // write pitch value
  lcd.print(output);
  lcd.write(DEGREE);

  int outputLength = output.length() + 1;

  // pad spaces before pitch value
  output = "";
  for (int i=outputLength; i<10; i++) {
    output += " ";
  }
  if (pitchString.length() < 3) {
    output += " ";
  }
  if (rollString.length() == 1) {
    output += " ";
  }
  output += rollString;

  // pad spaces before roll value
  lcd.print(output);
  lcd.write(DEGREE);

  outputLength += output.length() + 1;

  // fill the rest of the line with blanks
  for (int i=outputLength; i<16; i++) {
    lcd.print(" ");
  }

  if (DEBUG){
    Serial.print("pitch: ");
    Serial.print(pitchString);
    Serial.print(" roll: ");
    Serial.print(rollString);
    Serial.println("");
  }
}
