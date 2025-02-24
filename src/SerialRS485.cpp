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
  Foundation, Inc., 51 Franklin Street, Suite 500, Boston, MA 02110-1335, USA
*/

#include "SerialRS485.h"

void SerialRS485Class::begin(unsigned long baudrate)
{
    RS485.setPins(1, 9, 10); // Set different pins for DE and RE
    RS485.begin(baudrate);
    RS485.receive(); // enable reception
}

String SerialRS485Class::request(const char *command)
{
    String response = "";

    RS485.beginTransmission(); // enable transmission
    RS485.print(command);
    RS485.endTransmission(); // disable transmission
    unsigned long startTime = millis();
    while (millis() - startTime < 150) // 150 ms timeout
    {
        if (RS485.available())
        {
            char c = RS485.read();
            response += c;
            startTime = millis(); // reset timeout if data is received
        }
    }

    return response;
}

SerialRS485Class SerialRS485;
