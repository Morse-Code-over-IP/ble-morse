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

#include "DinRailDevice.h"

/*
///////////////////////////////////////////////////////////////////////////////////////////
//Bits: 
// 31      noCRNL
// 30      noOUT
// 29..24  reserved
// 23..0   Baudrate in Baud
uint32_t DinRail::SetComMode(uint8_t madr, uint32_t mode)
{
uint16_t del=0;
char str[20] = {0};
uint8_t ptr=0;
uint32_t retval;  
  digitalWrite(txenpin, HIGH);
  rs485->print("RP");
  rs485->print( madr>>4, HEX);
  rs485->print(madr & 0x0F, HEX);
  rs485->print("W");//command W
  rs485->print("FE");//register FE, Bits: noCRNL=31,noOUT=30,unused 29-24,boudrade=23..0

  rs485->print(mode>>28 &0x0F, HEX);
  rs485->print(modek>>24 &0x0F, HEX);
  rs485->print(mode>>20 &0x0F, HEX);
  rs485->print(mode>>16 &0x0F, HEX);
  rs485->print(mode>>12 &0x0F, HEX);
  rs485->print(mode>>8 &0x0F, HEX);
  rs485->print(mode>>4 &0x0F, HEX);
  rs485->print(mode & 0x0F, HEX);
  rs485->print("00");//keine checksumme!
  digitalWrite(txenpin, LOW);
  //now read the answer!
  while(del<=maxdelay)
  {
    delay(1);
    del++;
    while (rs485->available()){
       str[ptr] = rs485->read();
       if(str[ptr] == 10){//wait for CR==13, LF==10
         retval = hex2val32(str+7, 8);
         return retval;
       }
       if(++ptr>19)
          return DinRail_ERROR; //error to much characters!
    }
  }
  return DinRail_TIMEOUT; //timeout!
}
*/


uint8_t DinRailDevice::getLastType()
{
	return m_packetbuf.content.cmd;
}

///////////////////////////////////////////////////////////////////////////////////////////
uint32_t DinRailDevice::read_sadr(uint8_t sadr, uint16_t maxdelay)
{
	uint32_t result = m_bus->read(m_address, sadr, maxdelay);
  m_packetbuf = m_bus->getLastPacket();
  return result;
}

///////////////////////////////////////////////////////////////////////////////////////////
uint32_t DinRailDevice::write_sadr(uint8_t sadr, uint32_t data, uint16_t maxdelay)
{
	uint32_t result = m_bus->write(m_address, sadr, data, maxdelay);
  m_packetbuf = m_bus->getLastPacket();
  return result;
}
