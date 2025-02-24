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

#pragma once

#include <Wire.h>
#include <I2C_eeprom.h>

// ZR800 version
#define ZR800_NAME "ZR800"
#define ZR800_VERSION "1.0"
#define ZR800_YEAR "2025"

#define __DEBUG__ 0

#define EEPROM_SIZE 255

template <class T>
void printAnything(const T &value)
{
    const byte *p = (const byte *)(const void *)&value;
    unsigned int i;
    for (i = 0; i < sizeof(value); i++)
    {
        SerialUSB.print(*p++);
        SerialUSB.print(F(" "));
    }

    SerialUSB.println();
}

static const int BAUDRATE[4] PROGMEM = {2400, 4800, 9600, 19200};

// EEPROM address
#define START_MARK 95
#define SETTING_START 161
static const int ANALOGS[6] PROGMEM = {0, 13, 26, 39, 52, 65};
static const int SCALINGS[5] PROGMEM = {132, 133, 137, 141, 145};

class ConfigClass
{
public:
    void begin();
    void SaveAnalog(uint8_t);
    void SaveScaling();
    void SaveScaling(uint8_t, float);
    void SaveSetting();
    void ResetAll();

    // -------------- 1st eeprom -------------- //
    struct analogs_t
    {
        uint8_t mark;   // 1 byte, addr: 0, 13, 26, 39, 52, 65
        float base_min; // 4 byte, addr: 1..4
        float base_max; // 4 byte, addr: 5..8
    } analogs[6];       // 4 analog input, 2 analog output

    struct analogs_t_t
    {
        float current;
    } analogs_t[6];

    struct scalings_t
    {
        uint8_t mark;          // 1 byte, addr: 132
        float oxygen_min;      // 4 byte, addr: 133..136
        float oxygen_max;      // 4 byte, addr: 137..140
        float temperature_min; // 4 byte, addr: 141..144
        float temperature_max; // 4 byte, addr: 145..148
    } scalings;

    struct settings_t
    {
        uint8_t mark;      // 1 byte, addr: 161
        uint8_t backlight; // 1 byte, addr: 162 0..5
        uint8_t baudrate;  // 1 byte, addr: 163 0..3
    } settings;
};

extern ConfigClass Config;
