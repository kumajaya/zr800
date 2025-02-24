/*
 * Industruino Demo Code - Default code loaded onto Industruino
 *
 * Copyright (c) 2013 Loic De Buck <connect@industruino.com>
 *
 * Industruino is a DIN-rail mountable Arduino Leonardo compatible product
 * Please visit www.industruino.com for further information and code examples.
 * Standard peripherals connected to Industruino are:
 * UC1701 compatible LCD; rst:D19 dc:D20 dn:D21 sclk:D22 (set pin configuration in UC1701 library header)
 * 3-button membrane panel; D23, D24, D25
 */
#include <Indio.h>
#include <Wire.h>
#include <U8g2lib.h>

U8G2_UC1701_MINI12864_F_2ND_4W_HW_SPI lcd(U8G2_R2, /* cs=*/19, /* dc=*/22);

// menu defines

//- initial cursor parameters
int coll = 0;        // column counter for cursor - always kept at 0 in this demo (left side of the screen)
int channel = 0;     // Counter is controlled by the up&down buttons on the membrane panel. Has double use; 1. As row controller for the cursor (screen displays 6 rows of text, counting from 0 to 5). 2. As editor for numerical values shown on screen
int lastChannel = 0; // keeps track of previous 'channel'. Is used to detect change in state.

//- initial menu level parameters
int MenuLevel = 0;       // Defines the depth of the menu tree
int MenuID = 0;          // Defines the unique identifier of each menu that resides on the same menu level
int channelUpLimit = 5;  // Defines the upper limit of the button counter: 1. To limit cursor's downward row movement 2. To set the upper limit of value that is beeing edited.
int channelLowLimit = 0; // Defines the lower limit of the button counter: 1. To limit cursor's upward row movement 2. To set the lower limit of value that is beeing edited.

//- initial parameters for 'value editing mode'
int valueEditing = 0;      // Flag to indicate if the interface is in 'value editing mode', thus disabling cursor movement.
int row = 0;               // Temporary location to store the current cursor position whilst in 'value editing mode'.
int constrainEnc = 1;      // Enable/disable constraining the button panel's counter to a lower and upper limit.
float valueEditingInc = 0; // Increments of each button press when using 'value editing mode'.
float TargetValue = 0;     // Target value to be edited in 'value editing mode'

// Membrane panel button defines

int buttonUpState = 0;    // status of "Up" button input
int buttonEnterState = 0; // status of "Enter" button input
int buttonDownState = 0;  // status of "Down" button input

int prevBtnUp = 0;   // previous state of "Up" button
int prevBtnEnt = 0;  // previous state of "Enter" button
int prevBtnDown = 0; // previous state of "Down" button

int lastBtnUp = 0;   // time since last "Up" pressed event
int lastBtnEnt = 0;  // time since last "Enter" pressed event
int lastBtnDown = 0; // time since last "Down" pressed event

int enterPressed = 0; // status of "Enter" button after debounce filtering : 1 = pressed 0 = unpressed

int transEntInt = 250;                 // debounce treshold for "Enter" button
int transInt = 100;                    // debounce for other buttons
unsigned long lastAdminActionTime = 0; // keeps track of last button activity

// These constants won't change.  They're used to give names
// to the pins used:
const int analogInPin = A5;  // Analog input pin that the button panel is attached to
const int backlightPin = 26; // PWM output pin that the LED backlight is attached to
const int buttonEnterPin = 24;
const int buttonUpPin = 25;
const int buttonDownPin = 23;
const int D0 = 0;
const int D1 = 1;
const int D2 = 2;
const int D3 = 3;
const int D4 = 4;
const int D5 = 5;
const int D6 = 6;
const int D7 = 7;
const int D8 = 8;
const int D9 = 9;
const int D10 = 10;
const int D11 = 11;
const int D12 = 12;
const int D14 = 14;
const int D15 = 15;
const int D16 = 16;
const int D17 = 17;

float anOutCh1 = 0;
float anOutCh2 = 0;
float anOutUpLimit = 0;
float anOutDownLimit = 0;

int ButtonsAnalogValue = 0;      // value read from mebrane panel buttons.
int backlightIntensity = 5;      // LCD backlight intesity
int backlightIntensityDef = 5;   // Default LCD backlight intesity
unsigned long lastLCDredraw = 0; // keeps track of last time the screen was redrawn

// Function prototype
void MenuWelcome();
void Navigate();
void ScrollCursor();
void ReadButtons();
void SetInput();
void SetOutput();
float EditValue();
float EditFloatValue();

// ZR800 start
#include "Config.h"
#include "SerialRS485.h"
#include <floatToString.h>

float oxygen = 0;
float temperature = 0;
float oxygen_disp = 0;
float temperature_disp = 0;
uint8_t oxygen_unit = 0; // 0 = %; 1 = ppm
unsigned long lastSerialRead = 0;
uint8_t lastStatColor = 0;
uint8_t calibration = 0;

#include <Adafruit_SleepyDog.h>
static const uint16_t wd_period PROGMEM = 8000;

float remap(float valueIn, float baseMin, float baseMax, float limitMin, float limitMax, bool min_clip = true, bool max_clip = true)
{
  float v = ((limitMax - limitMin) * (valueIn - baseMin) / (baseMax - baseMin)) + limitMin;
  if (min_clip)
  {
    if (v < limitMin)
      v = limitMin;
  }
  if (max_clip)
  {
    if (v > limitMax)
      v = limitMax;
  }
  return v;
}

void SerialRS485Loop();

void setup()
{

  Indio.analogWriteMode(1, mA);
  Indio.analogWriteMode(2, mA);
  Indio.analogWrite(1, 0, true);
  Indio.analogWrite(2, 0, true);

  SetInput(); // Sets all general pins to input
  pinMode(buttonEnterPin, INPUT);
  pinMode(buttonUpPin, INPUT);
  pinMode(buttonDownPin, INPUT);

  pinMode(backlightPin, OUTPUT);                                      // set backlight pin to output
  analogWrite(backlightPin, (map(backlightIntensity, 5, 1, 255, 0))); // convert backlight intesity from a value of 0-5 to a value of 0-255 for PWM.

  // LCD init
  lcd.begin();
  lcd.clearBuffer();
  lcd.setMaxClipWindow();
  lcd.setFont(u8g2_font_6x12_tr);
  lcd.setDrawColor(0); // set color to blank
  lcd.drawBox(0, 0, 128, 64);
  lcd.sendBuffer();    // update screen now
  lcd.setDrawColor(1); // set color to black

  // debug
#if __DEBUG__
  SerialUSB.begin(9600); // enables port for debugging messages
#endif

  // Menu init
  MenuWelcome(); // load first menu

  // Configuration
  Config.begin(); // load default values or from EEPROM

  // RS485 init
  SerialRS485.begin(BAUDRATE[Config.settings.baudrate]);

  // Restore backlight config
  analogWrite(backlightPin, (map(Config.settings.backlight, 5, 0, 255, 0)));

  lcd.setCursor(5, (8 * 8) - 2);
  if (REG_PM_RCAUSE == PM_RCAUSE_SYST)
  {
    lcd.print(F("Reboot by system"));
  }
  if (REG_PM_RCAUSE == PM_RCAUSE_WDT)
  {
    lcd.print(F("Reboot by watchdog"));
  }
  if (REG_PM_RCAUSE == PM_RCAUSE_EXT)
  {
    lcd.print(F("Reboot by ext. reset"));
  }
#if 0
  if (REG_PM_RCAUSE == PM_RCAUSE_BOD33){
    SerialMon.println("Reset brown out 3.3V");
  }
  if (REG_PM_RCAUSE == PM_RCAUSE_BOD12){
    SerialMon.println("Reset brown out 1.2v");
  }
#endif
  if (REG_PM_RCAUSE == PM_RCAUSE_POR)
  {
    lcd.print(F("Reboot by power"));
  }

  lcd.sendBuffer(); // update screen now

  // enable 8s watchdog, 16s max. Reset it in every task
  Watchdog.enable(wd_period);
  delay(1000);
}

/*
* 1. The loop function calls a function to check the buttons (this could also be driven by timer interrupt) and updates the button counter (variable called 'channel'), which increases when 'Down' button is pressed and decreases when "Up" buttons is pressed.
* 2. Next, the loop function calls the 'Navigate' function which draws the cursor in a position based on the button counter, and when the "Enter" button is pressed checks which new menu should be  loaded or what other action to perform.
* 3. Each menu's content and scope is defined in a separate function. Each menu should have a defined 'MenuLevel' (depth of the menu tree, starting from 0) and unique MenuID so that the Navigate function can discern which menu is active.

*To make your own menus you should take 2 steps:

*1. make a new menu function, edit the parameters such MenuLevel and MenuID, scope of the cursor (number of rows, constraints etc).
*2. Edit the 'Navigate' function to reflect the menu function that you just made and assigning an action to it.
*/

void loop()
{
  // Make sure to reset watchdog every loop iteration!
  Watchdog.reset();
  ReadButtons(); // check buttons
  Navigate();    // update menus and perform actions
  //  delay(50);
  SerialRS485Loop();
}

void SerialRS485Loop()
{
  if ((millis() - lastSerialRead) > 1000)
  {
    oxygen = SerialRS485.request("?\r").toFloat();
    float fx = 0;
    if (!calibration)
    {
      fx = remap(oxygen, Config.scalings.oxygen_min * pow(10, -6),
                 Config.scalings.oxygen_max * pow(10, -2), Config.analogs[4].base_min,
                 Config.analogs[4].base_max);
      Indio.analogWrite(1, fx, false); // write scaled oxygen to analog output
    }
    oxygen_unit = 1; // dispaly unit in ppm by default
    oxygen = oxygen * 100.0 * 10000.0;
    int i = int(log10(oxygen) + 1); // check digit, max 5 for ppm
    if (i > 5)
    {
      oxygen_unit = 0; // display unit in %
      oxygen = oxygen / 10000.0;
    }
    delay(100);
    temperature = SerialRS485.request("TEMP?\r").toFloat();
    if (!calibration)
    {
      fx = remap(temperature, Config.scalings.temperature_min, Config.scalings.temperature_max,
                 Config.analogs[5].base_min, Config.analogs[5].base_max);
      Indio.analogWrite(2, fx, false); // write scaled temperature to analog output
    }
#if __DEBUG__
    SerialUSB.println(oxygen, 1);
    SerialUSB.println(temperature, 1);
#endif
    lastStatColor = !lastStatColor;
    lastSerialRead = millis();
  }
}

//-----------------------------------------------------------------------------------------------------------------------------------------------------------
// UI menu content - edit, add or remove these functions to make your own menu structure
// These functions only generate the content that is printed to the screen, please also edit the "Navigate" function further below to add actions to each menu.
//------------------------------------------------------------------------------------------------------------------------------------------------------------

void MenuWelcome()
{ // this function draws the first menu - splash screen
  // menu initializers
  channel = 0;         // starting row position of the cursor (top row) - controlled by the button panel counter
  channelUpLimit = 0;  // upper row limit
  channelLowLimit = 0; // lower row limit
  MenuLevel = 0;       // menu tree depth -> first level
  MenuID = 0;          // unique menu id -> has to be unique for each menu on the same menu level.
  enterPressed = 0;    // clears any possible accidental "Enter" presses that could have been carried over from the previous menu
  lcd.clearBuffer();
  // actual user content on the screen
  lcd.setDrawColor(1);
  lcd.setCursor(6, 8 * 1);                         // set the cursor to the sixth pixel from the left edge, first row.
  lcd.print(F("Welcome to"));                      // print text on screen
  lcd.setCursor(6, 8 * 2);                         // set the cursor to the sixth pixel from the left edge, second row.
  lcd.print(F(ZR800_NAME " v" ZR800_VERSION "!")); // print text on screen
  lcd.setCursor(6, 8 * 4);
  lcd.print(F("a Kiiota product"));
  lcd.setCursor(6, 8 * 6);
  lcd.print(F(ZR800_YEAR ", Automation DEV"));
  lcd.sendBuffer(); // update screen now
  delay(2000);
}

void MenuMainLive();

void MenuMain()
{ // second menu - choice of submenu's
  // menu inintialisers
  channel = 8; // starting row position of the cursor (top row) - controlled by the button panel counter
  channelLowLimit = 8;
  channelUpLimit = 8; // upper row limit
  MenuLevel = 1;      // menu tree depth -> second level
  MenuID = 1;         // unique menu id -> has to be unique for each menu on the same menu level.
  enterPressed = 0;   // clears any possible accidental "Enter" presses that could have been carried over from the previous menu
  lcd.clearBuffer();  // clear the screen
  // actual user content on the screen
  lcd.setDrawColor(1);
  lcd.drawRBox(6, 0, 122, 14, 2); // Draw a rounded box for the title background
  int stringWidth = lcd.getStrWidth(ZR800_NAME " DISPLAY");
  lcd.setCursor(9 + (122 - stringWidth) / 2, 9);
  lcd.setDrawColor(0);
  lcd.print(F(ZR800_NAME " DISPLAY"));
  lcd.setDrawColor(1);
  lcd.drawRFrame(6, 9, 122, 26, 3); // Draw a rounded frame for the main display area
  lcd.setDrawColor(0);
  lcd.drawBox(7, 11, 120, 3); // Draw a white box to hide rounded part
  lcd.setDrawColor(1);
  lcd.drawRFrame(6, 37, 122, 26, 3);    // Draw a rounded frame for the secondary display area
  stringWidth = lcd.getStrWidth("ppm"); // use ppm string as reference
  lcd.drawCircle((128 - stringWidth) - 2, 42, 1);
  lcd.setCursor((128 - stringWidth) + 1, 48);
  lcd.print(F("C"));
}

void MenuMainLive()
{
  int stringWidth = 0;
  char S[10];

  // update serial activity status
  lcd.setDrawColor(lastStatColor);
  lcd.drawDisc(12, 5, 2);

  lcd.setFont(u8g2_font_ncenB18_tn); // 18 pixel height
  if (oxygen_disp != oxygen)
  {
    lcd.setDrawColor(0);
    lcd.drawBox(8, 11, 100, 22);
    lcd.setDrawColor(1);
    floatToString(oxygen, S, sizeof(S), 1);
    stringWidth = lcd.getStrWidth(S);
    lcd.setCursor(100 - stringWidth, 32);
    lcd.print(S);
    lcd.setFont(u8g2_font_6x12_tr);
    stringWidth = lcd.getStrWidth("ppm");
    lcd.setDrawColor(0);
    lcd.drawBox((128 - stringWidth) - 3, 22 - 8, stringWidth, 10);
    lcd.setDrawColor(1);
    lcd.setCursor((128 - stringWidth) - 3, 22);
    if (oxygen_unit == 0)
      lcd.print(F("%"));
    else
      lcd.print(F("ppm"));
    oxygen_disp = oxygen;
  }

  lcd.setFont(u8g2_font_ncenB18_tn); // 18 pixel height
  if (temperature_disp != temperature)
  {
    lcd.setDrawColor(0);
    lcd.drawBox(8, 39, 100, 22);
    lcd.setDrawColor(1);
    floatToString(temperature, S, sizeof(S), 1);
    stringWidth = lcd.getStrWidth(S);
    lcd.setCursor(100 - stringWidth, 59);
    lcd.print(S);
    temperature_disp = temperature;
  }

  lcd.setFont(u8g2_font_6x12_tr); // restore default font
  lcd.sendBuffer();               // update screen now
}

void MenuSetup()
{ // submenu of Main menu - setup screen for Industruino
  channel = 6;
  channelUpLimit = 7;
  channelLowLimit = 0;
  MenuID = 1;
  MenuLevel = 2;
  enterPressed = 0;
  lcd.clearBuffer();
  lcd.setDrawColor(1);
  int i = 0;
  lcd.setCursor(6, 8 * (i + 1));
  lcd.print(F("Backlight"));
  lcd.setCursor(79, 8 * (i + 1));
  lcd.print(Config.settings.backlight);
  i++;
  lcd.setCursor(6, 8 * (i + 1));
  lcd.print(F("Baudrate"));
  lcd.setCursor(79, 8 * (i + 1));
  lcd.print(BAUDRATE[Config.settings.baudrate]);
  i++;
  lcd.setCursor(6, 8 * (i + 1));
  lcd.print(F("Oxygen zero"));
  int stringWidth;
  char S[10];
  floatToString(Config.scalings.oxygen_min, S, sizeof(S), 1);
  stringWidth = lcd.getStrWidth(S);
  lcd.setCursor(79, 8 * (i + 1));
  lcd.print(S);
  lcd.setCursor(79 + stringWidth + 1, 8 * (i + 1));
  lcd.print(F("E-06"));
  i++;
  lcd.setCursor(6, 8 * (i + 1));
  lcd.print(F("Oxygen span"));
  floatToString(Config.scalings.oxygen_max, S, sizeof(S), 1);
  stringWidth = lcd.getStrWidth(S);
  lcd.setCursor(79, 8 * (i + 1));
  lcd.print(S);
  lcd.setCursor(79 + stringWidth + 1, 8 * (i + 1));
  lcd.print(F("E-02"));
  i++;
  lcd.setCursor(6, 8 * (i + 1));
  lcd.print(F("Temp. zero"));
  lcd.setCursor(79, 8 * (i + 1));
  lcd.print(Config.scalings.temperature_min, 0);
  i++;
  lcd.setCursor(6, 8 * (i + 1));
  lcd.print(F("Temp. span"));
  lcd.setCursor(79, 8 * (i + 1));
  lcd.print(Config.scalings.temperature_max, 0);
  i++;
  lcd.setCursor(6, 8 * (i + 1));
  i++;
  lcd.print(F("Back"));
  lcd.setCursor(6, 8 * (i + 1));
  lcd.print(F("Next"));
  lcd.sendBuffer(); // update screen now
  ScrollCursor();
  // reset measurement for force update later
  oxygen_disp = 0;
  temperature_disp = 0;
}

void MenuCalibrateOutput()
{
  channel = 4;
  channelUpLimit = 5;
  channelLowLimit = 0;
  MenuLevel = 3;
  MenuID = 1;
  enterPressed = 0;
  lcd.clearBuffer();
  lcd.setDrawColor(1);
  ScrollCursor();
  int i = 0;
  lcd.setCursor(6, 8 * (i + 1));
  lcd.print(F("Out. 1 zero"));
  lcd.setCursor(98, 8 * (i + 1));
  lcd.print(Config.analogs[4].base_min, 3);
  i++;
  lcd.setCursor(6, 8 * (i + 1));
  lcd.print(F("Out. 1 span"));
  lcd.setCursor(92, 8 * (i + 1));
  lcd.print(Config.analogs[4].base_max, 3);
  i++;
  lcd.setCursor(6, 8 * (i + 1));
  lcd.print(F("Out. 2 zero"));
  lcd.setCursor(98, 8 * (i + 1));
  lcd.print(Config.analogs[5].base_min, 3);
  i++;
  lcd.setCursor(6, 8 * (i + 1));
  lcd.print(F("Out. 2 span"));
  lcd.setCursor(92, 8 * (i + 1));
  lcd.print(Config.analogs[5].base_max, 3);
  i++;
  lcd.setCursor(6, 8 * (i + 1));
  lcd.print(F("Back"));
  lcd.sendBuffer(); // update screen now
  i++;
  lcd.setCursor(6, 8 * (i + 1));
  lcd.print(F("Reset"));
  lcd.sendBuffer(); // update screen now

  calibration = 1;
  Indio.analogWrite(1, Config.analogs[4].base_min, false);
  Indio.analogWrite(2, Config.analogs[5].base_min, false);
}

//---------------------------------------------------------------------------------------------------------------------------------------------------
// UI control logic, please edit this function to reflect the specific menus that your created above and your desired actions for each cursor position
//---------------------------------------------------------------------------------------------------------------------------------------------------

void Navigate()
{

  if (valueEditing != 1)
  {

    if (MenuLevel == 0) // check if current activated menu is the 'splash screen' (first level)
    {
      {
        if (enterPressed == 1)
          MenuMain(); // if enter is pressed load the 'Main menu'
      }
    }

    if (MenuLevel == 1)
    { // check if current activated menu is the 'Main menu' (first level)
      if ((millis() - lastLCDredraw) > 300)
      {
        MenuMainLive();
        lastLCDredraw = millis();
      }
      if (channel == 8 && enterPressed == 1)
      {
        MenuSetup();
      }
      else
      {
        MenuMainLive();
      }
    }

    if (MenuLevel == 2)
    {
      if (MenuID == 1)
      {
        uint8_t backlight = 6;
        if (channel == 0 && enterPressed == 1) // using 'value editing mode' to edit a variable using the UI
        {
          backlight = Config.settings.backlight; // save current value of the variable to be edited
          TargetValue = backlight;               // copy variable to be edited to 'Target value'
          Config.settings.backlight = (uint8_t)EditValue();
          analogWrite(backlightPin, (map(Config.settings.backlight, 5, 0, 255, 0)));
        }
        uint8_t baud = 4;
        if (channel == 1 && enterPressed == 1) // using 'value editing mode' to edit a variable using the UI
        {
          baud = Config.settings.baudrate; // save current value of the variable to be edited
          TargetValue = baud;              // copy variable to be edited to 'Target value'
          Config.settings.baudrate = (byte)EditValue();
        }
        float oxygen_min = -1;
        if (channel == 2 && enterPressed == 1) // using 'value editing mode' to edit a variable using the UI
        {
          oxygen_min = Config.scalings.oxygen_min; // save current value of the variable to be edited
          TargetValue = oxygen_min;                // copy variable to be edited to 'Target value'
          Config.scalings.oxygen_min = EditFloatValue();
        }
        float oxygen_max = -1;
        if (channel == 3 && enterPressed == 1) // using 'value editing mode' to edit a variable using the UI
        {
          oxygen_max = Config.scalings.oxygen_max; // save current value of the variable to be edited
          TargetValue = oxygen_max;                // copy variable to be edited to 'Target value'
          Config.scalings.oxygen_max = EditFloatValue();
        }
        float temperature_min = -1;
        if (channel == 4 && enterPressed == 1) // using 'value editing mode' to edit a variable using the UI
        {
          temperature_min = Config.scalings.temperature_min; // save current value of the variable to be edited
          TargetValue = temperature_min;                     // copy variable to be edited to 'Target value'
          Config.scalings.temperature_min = (float)EditValue();
        }
        float temperature_max = -1;
        if (channel == 5 && enterPressed == 1) // using 'value editing mode' to edit a variable using the UI
        {
          temperature_max = Config.scalings.temperature_max; // save current value of the variable to be edited
          TargetValue = temperature_max;                     // copy variable to be edited to 'Target value'
          Config.scalings.temperature_max = (float)EditValue();
        }
        if (channel == 7 && enterPressed == 1)
          MenuCalibrateOutput();
        if (channel == 6 && enterPressed == 1)
        {
          if (backlight != Config.settings.backlight)
            Config.SaveSetting();
          if (baud != Config.settings.baudrate)
          {
            Config.SaveSetting();
            SerialRS485.begin(BAUDRATE[Config.settings.baudrate]);
          }
          if (oxygen_min != Config.scalings.oxygen_min)
            Config.SaveScaling(1, Config.scalings.oxygen_min);
          if (oxygen_max != Config.scalings.oxygen_max)
            Config.SaveScaling(2, Config.scalings.oxygen_max);
          if (temperature_min != Config.scalings.temperature_min)
            Config.SaveScaling(3, Config.scalings.temperature_min);
          if (temperature_max != Config.scalings.temperature_max)
            Config.SaveScaling(4, Config.scalings.temperature_max);
          MenuMain();
        }
      }
    }

    if (MenuLevel == 3)
    {
      if (MenuID == 1)
      {
        int i = 0;
        float fx;
        if (channel == i && enterPressed == 1)
        {
          TargetValue = Config.analogs[4].base_min;
          fx = remap(4.0, 4.0, 20.0, TargetValue, Config.analogs[4].base_max);
          Indio.analogWrite(1, fx, false);
          Config.analogs[4].base_min = EditFloatValue();
        }
        i++;
        if (channel == i && enterPressed == 1)
        {
          TargetValue = Config.analogs[4].base_max;
          fx = remap(20.0, 4.0, 20.0, Config.analogs[4].base_min, TargetValue);
          Indio.analogWrite(1, fx, false);
          Config.analogs[4].base_max = EditFloatValue();
        }
        i++;
        if (channel == i && enterPressed == 1)
        {
          TargetValue = Config.analogs[5].base_min;
          fx = remap(4.0, 4.0, 20.0, TargetValue, Config.analogs[4].base_max);
          Indio.analogWrite(2, fx, false);
          Config.analogs[5].base_min = EditFloatValue();
        }
        i++;
        if (channel == i && enterPressed == 1)
        {
          TargetValue = Config.analogs[5].base_max;
          fx = remap(20.0, 4.0, 20.0, Config.analogs[4].base_min, TargetValue);
          Indio.analogWrite(2, fx, false);
          Config.analogs[5].base_max = EditFloatValue();
        }
        i++;
        if (channel == i && enterPressed == 1)
        {
          Config.SaveAnalog(4);
          delay(100);
          Config.SaveAnalog(5);
          delay(100);
          calibration = 0;
          MenuSetup();
        }
        i++;
        if (channel == i && enterPressed == 1)
        {
          Config.ResetAll();
          delay(1000);
          NVIC_SystemReset(); // processor software reset
        }
      }
    }

    // dont remove this part
    if (channel != lastChannel && valueEditing != 1 && MenuID != 0)
    { // updates the cursor position if button counter changed and 'value editing mode' is not running
      ScrollCursor();
    }
  }
}

//---------------------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------------------

float EditValue() // a function to edit a variable using the UI - function is called by the main 'Navigate' UI control function and is loaded with a variable to be edited
{
  row = channel;       // save the current cursor position so that after using the buttons for 'value editing mode' the cursor position can be reinstated.
  constrainEnc = 0;    // disable constrainment of button counter's range
  valueEditingInc = 1; // increment for each button press
  valueEditing = 1;    // flag to indicate that we are going into 'value editing mode'.
  enterPressed = 0;    // clears any possible accidental "Enter" presses that could have been caried over
  int downLimit = 0;
  int upLimit = 4;
  while (enterPressed != 1)
  {                   // stays in 'value editing mode' until enter is pressed
    Watchdog.reset(); // reset watchdog
    ReadButtons();    // check the buttons for any change
    lcd.setCursor(0, 8 * (row + 1));
    lcd.print("*");
    if (channel != lastChannel)
    { // when up or down button is pressed
      if (MenuLevel == 2 && MenuID == 1)
      {
        if (row == 1)
        {
          downLimit = 0;
          upLimit = 2;
        } // baudrate
        if (row == 4 || row == 5)
        {
          downLimit = 0;
          upLimit = 999;
        } // temperature range
      }
      if (channel < lastChannel && TargetValue <= upLimit)
      {                                 // if 'Up' button is pressed, and is within constraint range.
        TargetValue += valueEditingInc; // increment target variable with pre-defined increment value
      }
      if (channel > lastChannel && TargetValue > downLimit)
      {                                 // if 'Down' button is pressed, and is within constraint range.
        TargetValue -= valueEditingInc; // decrement target variable with pre-defined increment value
      }
      // clear a section of a row to make space for updated value
      lcd.setDrawColor(0);
      lcd.drawBox(79, 8 * row, 128 - 79, 8);
      // print updated value
      lcd.setDrawColor(1);
      lcd.setCursor(79, 8 * (row + 1));
#if __DEBUG__
      SerialUSB.println(TargetValue);
#endif
      if (MenuLevel == 2 && MenuID == 1 && row == 1)
        lcd.print(BAUDRATE[(byte)TargetValue]);
      else
        lcd.print(TargetValue, 0);
      lastChannel = channel;
    }
    // delay(50);
    lcd.sendBuffer(); // update screen now
  }
  channel = row;      // load back the previous row position to the button counter so that the cursor stays in the same position as it was left before switching to 'value editing mode'
  constrainEnc = 1;   // enable constrainment of button counter's range so to stay within the menu's range
  valueEditing = 0;   // flag to indicate that we are leaving 'value editing mode'
  enterPressed = 0;   // clears any possible accidental "Enter" presses that could have been caried over
  return TargetValue; // return the edited value to the main 'Navigate' UI control function for further processing
}

float EditFloatValue() // a function to edit a variable using the UI - function is called by the main 'Navigate' UI control function and is loaded with a variable to be edited
{
  row = channel;         // save the current cursor position so that after using the buttons for 'value editing mode' the cursor position can be reinstated.
  constrainEnc = 0;      // disable constrainment of button counter's range
  valueEditingInc = 0.5; // increment for each button press
  valueEditing = 1;      // flag to indicate that we are going into 'value editing mode'.
  enterPressed = 0;      // clears any possible accidental "Enter" presses that could have been caried over
  while (enterPressed != 1)
  {                   // stays in 'value editing mode' until enter is pressed
    Watchdog.reset(); // reset watchdog
    ReadButtons();    // check the buttons for any change
    lcd.setCursor(0, 8 * (row + 1));
    lcd.print("*");
    if (channel != lastChannel)
    { // when up or down button is pressed
      if (MenuLevel == 2 && MenuID == 1)
      { // oxygen range
        valueEditingInc = 0.5;
        if (row == 2)
        {
          anOutDownLimit = 0.5;
          anOutUpLimit = 9.5;
        }
        if (row == 3)
        {
          anOutDownLimit = 0.5;
          anOutUpLimit = 19.5;
        }
      }
      if (MenuLevel == 3 && MenuID == 1)
      { // output calibration
        int i = 0;
        valueEditingInc = 0.005;
        if (row == i || row == (i + 2))
        {
          anOutDownLimit = 3;
          anOutUpLimit = 5;
        }
        i++;
        if (row == i || row == (i + 2))
        {
          anOutDownLimit = 19;
          anOutUpLimit = 21;
        }
      }
      if (channel < lastChannel && TargetValue <= anOutUpLimit)
      {                                 // if 'Up' button is pressed, and is within constraint range.
        TargetValue += valueEditingInc; // increment target variable with pre-defined increment value
      }
      if (channel > lastChannel && TargetValue >= anOutDownLimit)
      {                                 // if 'Down' button is pressed, and is within constraint range.
        TargetValue -= valueEditingInc; // decrement target variable with pre-defined increment value
      }
      // clear a section of a row to make space for updated value
      int stringWidth;
      char S[10];
      lcd.setDrawColor(0);
      if (MenuLevel == 2 && MenuID == 1)
      { // oxygen range
        floatToString(TargetValue, S, sizeof(S), 1);
        stringWidth = lcd.getStrWidth(S);
        lcd.drawBox(79, 8 * row, lcd.getStrWidth("00.0 E-00"), 8);
        lcd.setDrawColor(1);
        lcd.setCursor(79, 8 * (row + 1));
        lcd.print(S);
        lcd.setCursor(79 + stringWidth + 1, 8 * (row + 1));
        if (row == 2)
        {
          lcd.print("E-06");
        }
        else if (row == 3)
        {
          lcd.print("E-02");
        }
      }
      else if (MenuLevel == 3 && MenuID == 1)
      { // output calibration
        float fx = 0;
        lcd.drawBox(92, 8 * row, 128 - 92, 8);
        if (row == 0)
        {
          fx = remap(4.0, 4.0, 20.0, TargetValue, Config.analogs[4].base_max);
          Indio.analogWrite(1, fx, false);
          lcd.setCursor(98, 8 * (row + 1));
        }
        if (row == 1)
        {
          fx = remap(20.0, 4.0, 20.0, Config.analogs[4].base_min, TargetValue);
          Indio.analogWrite(1, fx, false);
          lcd.setCursor(92, 8 * (row + 1));
        }
        if (row == 2)
        {
          fx = remap(4.0, 4.0, 20.0, TargetValue, Config.analogs[5].base_max);
          Indio.analogWrite(2, fx, false);
          lcd.setCursor(98, 8 * (row + 1));
        }
        if (row == 3)
        {
          fx = remap(20.0, 4.0, 20.0, Config.analogs[5].base_min, TargetValue);
          Indio.analogWrite(2, fx, false);
          lcd.setCursor(92, 8 * (row + 1));
        }
        lcd.setDrawColor(1);
        lcd.print(TargetValue, 3);
      }
      else
      {
        lcd.drawBox(79, 8 * row, 128 - 79, 8);
        lcd.setCursor(79, 8 * (row + 1));
        lcd.setDrawColor(1);
        lcd.print(TargetValue, 1);
      }
      // print updated value
#if __DEBUG__
      SerialUSB.println(TargetValue);
#endif
      lastChannel = channel;
    }
    // delay(50);
    lcd.sendBuffer(); // update screen now
  }
  channel = row;      // load back the previous row position to the button counter so that the cursor stays in the same position as it was left before switching to 'value editing mode'
  constrainEnc = 1;   // enable constrainment of button counter's range so to stay within the menu's range
  valueEditing = 0;   // flag to indicate that we are leaving 'value editing mode'
  enterPressed = 0;   // clears any possible accidental "Enter" presses that could have been caried over
  return TargetValue; // return the edited value to the main 'Navigate' UI control function for further processing
}

//---------------------------------------------------------------------------------------------------------------------------------------------
// Peripheral functions
//---------------------------------------------------------------------------------------------------------------------------------------------
void ReadButtons()
{

  buttonEnterState = digitalRead(buttonEnterPin);
  buttonUpState = digitalRead(buttonUpPin);
  buttonDownState = digitalRead(buttonDownPin);

  if (buttonEnterState == HIGH && prevBtnEnt == LOW)
  {
    if ((millis() - lastBtnEnt) > (unsigned int)transEntInt)
    {
      enterPressed = 1;
    }
    lastBtnEnt = millis();
    lastAdminActionTime = millis();
#if __DEBUG__
    SerialUSB.println(enterPressed ? "EnterPressed" : "");
#endif
  }
  prevBtnEnt = buttonEnterState;

  if (buttonUpState == HIGH && prevBtnUp == LOW)
  {
    if ((millis() - lastBtnUp) > (unsigned int)transInt)
    {
      channel--;
    }
    lastBtnUp = millis();
    lastAdminActionTime = millis();
#if __DEBUG__
    SerialUSB.println("UpPressed");
#endif
  }
  prevBtnUp = buttonUpState;

  if (buttonDownState == HIGH && prevBtnDown == LOW)
  {
    if ((millis() - lastBtnDown) > (unsigned int)transInt)
    {
      channel++;
    }
    lastBtnDown = millis();
    lastAdminActionTime = millis();
#if __DEBUG__
    SerialUSB.println("DownPressed");
#endif
  }
  prevBtnDown = buttonDownState;

  if (constrainEnc == 1)
  {
    channel = constrain(channel, channelLowLimit, channelUpLimit);
  }
}

void SetOutput()
{ // a simple function called to set a group of pins as outputs
  pinMode(D0, OUTPUT);
  pinMode(D1, OUTPUT);
  pinMode(D2, OUTPUT);
  pinMode(D3, OUTPUT);
  pinMode(D4, OUTPUT);
  pinMode(D5, OUTPUT);
  pinMode(D6, OUTPUT);
  pinMode(D7, OUTPUT);
  pinMode(D8, OUTPUT);
  pinMode(D9, OUTPUT);
  pinMode(D10, OUTPUT);
  pinMode(D11, OUTPUT);
  pinMode(D12, OUTPUT);
  pinMode(D14, OUTPUT);
  pinMode(D15, OUTPUT);
  pinMode(D16, OUTPUT);
  pinMode(D17, OUTPUT);
}

void SetInput()
{ // a simple function called to set a group of pins as inputs
  pinMode(D0, INPUT);
  pinMode(D1, INPUT);
  pinMode(D2, INPUT);
  pinMode(D3, INPUT);
  pinMode(D4, INPUT);
  pinMode(D5, INPUT);
  pinMode(D6, INPUT);
  pinMode(D7, INPUT);
  pinMode(D8, INPUT);
  pinMode(D9, INPUT);
  pinMode(D10, INPUT);
  pinMode(D11, INPUT);
  pinMode(D12, INPUT);
  pinMode(D14, INPUT);
  pinMode(D15, INPUT);
  pinMode(D16, INPUT);
  pinMode(D17, INPUT);
}

//---------------------------------------------------------------------------------------------------------------------------------------------
// UI core functions
//---------------------------------------------------------------------------------------------------------------------------------------------

void ScrollCursor() // makes the cursor move
{
  lastChannel = channel; // keep track button counter changes
  lcd.setDrawColor(0);
  lcd.drawBox(coll, 0, 6, 64); // clear the whole column when redrawing a new cursor
  lcd.setDrawColor(1);
  lcd.setCursor(coll, 8 * (channel + 1));
  lcd.print(">"); // draw cursor
  lcd.sendBuffer();
}
