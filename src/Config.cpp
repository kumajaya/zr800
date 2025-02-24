/*
  Copyright (c) 2025 Ketut P. Kumajaya.  All right reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "Config.h"

I2C_eeprom eeprom50(0x50, EEPROM_SIZE);
I2C_eeprom eeprom51(0x51, EEPROM_SIZE);
I2C_eeprom eeprom52(0x52, EEPROM_SIZE);
I2C_eeprom eeprom53(0x53, EEPROM_SIZE);

I2C_eeprom eeprom[4] = {eeprom50, eeprom51, eeprom52, eeprom53};

union f_4bytes
{ // define a union structure to convert float into 4 bytes and back
  float f;
  byte b[4];
};

union d_8bytes
{ // define a union structure to convert double into 8 bytes and back
  double d;
  byte b[8];
};

void ConfigClass::begin()
{
  // EEPROM
  Wire.begin();
  eeprom50.begin();
  eeprom51.begin();
  eeprom52.begin();
  eeprom53.begin();
  delay(2000);

  byte val = 0;
  for (int i = 0; i <= 5; i++)
  {
    val = eeprom50.readByte(ANALOGS[i]);
    delay(100);
    if (val == START_MARK)
      eeprom50.readBlock(ANALOGS[i], (uint8_t *)&analogs[i], sizeof(analogs[i]));
    else
      analogs[i] = {0, 4, 20}; // default
    delay(100);
  }

  scalings = {0, 3.0, 5.5, 0.0, 700.0}; // default
  val = eeprom50.readByte(SCALINGS[0]);
  delay(100);
  if (val == START_MARK)
  {
    float scalings_t[4];
    for (int i = 0; i <= 3; i++)
    {
      union f_4bytes union_float_read;
      for (int j = 0; j <= 3; j++)
      {
        union_float_read.b[j] = eeprom50.readByte(SCALINGS[i + 1] + j);
        delay(10);
      }
      scalings_t[i] = union_float_read.f;
    }
    scalings = {START_MARK, scalings_t[0], scalings_t[1], scalings_t[2], scalings_t[3]}; // new value
  }

  delay(100);
  settings = {0, 3, 2}; // default
  val = eeprom50.readByte(SETTING_START);
  delay(100);
  if (val == START_MARK)
    eeprom50.readBlock(SETTING_START, (uint8_t *)&settings, sizeof(settings));
}

// cppcheck-suppress unusedFunction
void ConfigClass::SaveAnalog(uint8_t i)
{
  analogs[i].mark = START_MARK;
  eeprom50.writeBlock(ANALOGS[i], (uint8_t *)&analogs[i], sizeof(analogs[i]));
}

// cppcheck-suppress unusedFunction
void ConfigClass::SaveScaling()
{
  eeprom50.writeByte(SCALINGS[0], START_MARK);
  delay(10);

  float scalings_t[4] = {scalings.oxygen_min, scalings.oxygen_max, scalings.temperature_min, scalings.temperature_max};
  union f_4bytes union_float_write;
  for (int i = 0; i <= 3; i++)
  {
    union_float_write.f = scalings_t[i];
    for (int j = 0; j <= 3; j++)
    {
      eeprom50.writeByte(SCALINGS[i + 1] + j, union_float_write.b[j]);
      delay(10);
    }
  }
}

// cppcheck-suppress unusedFunction
void ConfigClass::SaveScaling(uint8_t i, float value)
{
  eeprom50.writeByte(SCALINGS[0], START_MARK);
  delay(10);

  union f_4bytes union_float_write;
  union_float_write.f = value;
  for (int j = 0; j <= 3; j++)
  {
    eeprom50.writeByte(SCALINGS[i] + j, union_float_write.b[j]);
    delay(10);
  }
}

// cppcheck-suppress unusedFunction
void ConfigClass::SaveSetting()
{
  settings.mark = START_MARK;
  eeprom50.writeBlock(SETTING_START, (uint8_t *)&settings, sizeof(settings));
}

// cppcheck-suppress unusedFunction
void ConfigClass::ResetAll()
{ // remove START_MARK
  eeprom50.writeByte(SCALINGS[0], 0);
  delay(10);
  eeprom50.writeByte(SETTING_START, 0);
  delay(10);
  for (int i = 0; i <= 5; i++)
  {
    eeprom50.writeByte(ANALOGS[i], 0);
    delay(10);
  }
}

ConfigClass Config;
