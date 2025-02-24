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

#pragma once

#include <ArduinoRS485.h>

class SerialRS485Class
{
public:
    void begin(unsigned long baudrate);
    String request(const char *command);
};

extern SerialRS485Class SerialRS485;
