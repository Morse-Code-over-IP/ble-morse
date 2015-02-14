/*
	DinRailDevice.h - Library for access to a DinRailBus Device developed by In-Circuit

	Compatible devices may be listed on
	www.in-circuit.de or http://wiki.in-circuit.de#DIN_Rail_Modules

	v0.1 - 2014.10.24
		Basic implementation - Command types are subject to change

	Copyright (c) 2014 In-Circuit GmbH

	Permission is hereby granted, free of charge, to any person obtaining a copy of this software
	and associated documentation files (the "Software"), to deal in the Software without restriction,
	including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
	and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT
	LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
	IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
	WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#ifndef DINRAILDEVICE_H
#define DINRAILDEVICE_H

#include <inttypes.h>

#include "DinRailBus.h"

class DinRailDevice
{
private:
	DinRailBus * m_bus;
  uint32_t m_baudrate;
	uint8_t m_address;
  dinRailBusPacket m_packetbuf;

public:
  DinRailDevice(DinRailBus * bus, uint8_t addr)
  {
		m_bus = bus;
		m_address = addr;
	}
  ~DinRailDevice() {}

  uint32_t read_sadr(uint8_t sadr, uint16_t maxdelay);
  uint32_t write_sadr(uint8_t sadr, uint32_t data, uint16_t maxdelay);

	uint8_t getLastType();
};

#endif