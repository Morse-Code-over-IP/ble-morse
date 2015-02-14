/*
	DinRailBus.cpp - Library for access to the DinRailBus developed by In-Circuit

	Accesses the hardware bus using a SoftwareSerial as UART or in RS485 mode.
	For the protokoll specification refer to
	http://wiki.in-circuit.de/index.php5?title=DIN_Rail_BUS_Protocol

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

#include "DinRailBus.h"

uint8_t DinRailBus::getLastType()
{
	return m_packetbuf.content.cmd;
}

dinRailBusPacket DinRailBus::getLastPacket()
{
	return m_packetbuf;
}

// successively build a command packet from parsed chars
uint8_t DinRailBus::checkpacket(uint8_t chr)
{
	if (m_parserPos>=sizeof(dinRailBusPacket))
  {memset(&m_packetbuf,0,sizeof(dinRailBusPacket)); m_parserPos=0;}
  m_packetbuf.bytes[m_parserPos] = chr;

	switch (m_parserPos)
	{
		// Magic word part 1
		case 0:
			if (chr!='R')
			{memset(&m_packetbuf,0,sizeof(dinRailBusPacket)); m_parserPos=0; return 0;}
			break;
		// Magic word part 2
    case 1:
			if (chr!='P')
			{memset(&m_packetbuf,0,sizeof(dinRailBusPacket)); m_parserPos=0; return checkpacket(chr);}
			break;
		// Device address
    case 2:
    case 3:
			if (!ishex(chr))
			{memset(&m_packetbuf,0,sizeof(dinRailBusPacket)); m_parserPos=0; return checkpacket(chr);}
			break;
		// Command
		case 4:
			if (chr!=DRBC_READ && chr!=DRBC_WRITE && chr!=DRBC_SYNC && chr!=DRBC_READ_ACK && chr!=DRBC_WRITE_ACK && chr!=DRBC_NACK && chr!=DRBC_NACK_DATA)
			{memset(&m_packetbuf,0,sizeof(dinRailBusPacket)); m_parserPos=0; return checkpacket(chr);}
			break;
		// Sub address
    case 5:
			if (m_packetbuf.content.cmd=='R' && !ishex(chr))
      {memset((&m_packetbuf)+1,0,sizeof(dinRailBusPacket)-1); m_parserPos=1; return checkpacket(chr);}
    case 6:
			if (!ishex(chr))
			{memset(&m_packetbuf,0,sizeof(dinRailBusPacket)); m_parserPos=0; return checkpacket(chr);}
			break;
		// data and/or checksum
    case 7:
    case 8:
    case 9:
    case 10:
    case 11:
    case 12:
    case 13:
    case 14:
    case 15:
    case 16:
			if (!ishex(chr))
			{memset(&m_packetbuf,0,sizeof(dinRailBusPacket)); m_parserPos=0; return checkpacket(chr);}
			break;
	}
	// check if correct dataless packet
	if (m_parserPos==8)
	{
		// We currently ignore read commands
		if (m_packetbuf.content.cmd == DRBC_READ)
		{memset(&m_packetbuf,0,sizeof(dinRailBusPacket)); m_parserPos=0; return 0;}
		// We currently ignore sync commands
		if (m_packetbuf.content.cmd == DRBC_SYNC)
		{memset(&m_packetbuf,0,sizeof(dinRailBusPacket)); m_parserPos=0; return 0;}
		// Checksum currently is ignored so we got a correct packet
		if (m_packetbuf.content.cmd == DRBC_NACK)
		{m_parserPos=sizeof(dinRailBusPacket); return 1;}
	}

	// check if correct packet with data
	if (m_parserPos==16)
	{
		// checksum currently is ignored so we got a valid packet
		m_parserPos=sizeof(dinRailBusPacket);
		return 1;
	}

	m_parserPos++;
	return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////
uint32_t DinRailBus::read(uint8_t madr, uint8_t sadr, uint16_t maxdelay)
{
  if (txenpin>=0) digitalWrite(txenpin, HIGH);
  mp_rs485->print("RP");
  mp_rs485->print( madr>>4, HEX);
  mp_rs485->print(madr & 0x0F, HEX);
  mp_rs485->write(DRBC_READ);
  mp_rs485->print(sadr>>4, HEX);
  mp_rs485->print(sadr & 0x0F, HEX);
  mp_rs485->print("00"); //keine checksumme!
  if (txenpin>=0) digitalWrite(txenpin, LOW);

  //now read the answer!
  if (maxdelay<DINRAIL_MINDELAY)
		maxdelay=DINRAIL_MINDELAY;
	checkpacket(0); // Resets parser buf
	for (uint16_t i=0; i<maxdelay ; i++)
  {
    delay(1);
    while (mp_rs485->available())
		{
			// packet complete?
      if (checkpacket(mp_rs485->read()))
			{
				return hex2val32(m_packetbuf.content.data,8);
			}
		}
	}

	return DINRAIL_TIMEOUT; //timeout!
}

///////////////////////////////////////////////////////////////////////////////////////////
uint32_t DinRailBus::write(uint8_t madr, uint8_t sadr, uint32_t data, uint16_t maxdelay)
{
  if (txenpin>=0) digitalWrite(txenpin, HIGH);
  mp_rs485->print("RP");
  mp_rs485->print( madr>>4, HEX);
  mp_rs485->print(madr & 0x0F, HEX);
  mp_rs485->write(DRBC_WRITE);
  mp_rs485->print(sadr>>4, HEX);
  mp_rs485->print(sadr & 0x0F, HEX);
  mp_rs485->print(data>>28 & 0x0F, HEX);
  mp_rs485->print(data>>24 & 0x0F, HEX);
  mp_rs485->print(data>>20 & 0x0F, HEX);
  mp_rs485->print(data>>16 & 0x0F, HEX);
  mp_rs485->print(data>>12 & 0x0F, HEX);
  mp_rs485->print(data>>8 & 0x0F, HEX);
  mp_rs485->print(data>>4 & 0x0F, HEX);
  mp_rs485->print(data & 0x0F, HEX);
  mp_rs485->print("00"); //keine checksumme!
  if (txenpin>=0) digitalWrite(txenpin, LOW);

  //now read the answer!
  if (maxdelay<DINRAIL_MINDELAY)
		maxdelay=DINRAIL_MINDELAY;
	checkpacket(0); // Resets parser buf
	for (uint16_t i=0; i<maxdelay ; i++)
  {
    delay(1);
    while (mp_rs485->available())
		{
			// packet complete?
      if (checkpacket(mp_rs485->read()))
			{
				return hex2val32(m_packetbuf.content.data,8);
			}
		}
	}

	return DINRAIL_TIMEOUT; //timeout!
}

uint8_t DinRailBus::ishex(uint8_t chr)
{
	if (chr>='0' && chr<='9') return 1;
	if (chr>='a' && chr<='f') return 1;
	if (chr>='A' && chr<='F') return 1;
	return 0;
}

uint8_t  DinRailBus::hex2valnibble(uint8_t ch)
{
  switch (ch)
  {
      case'a':
      case'b':
      case'c':
      case'd':
      case'e':
      case'f': return ch-'a'+10;
      case'A':
      case'B':
      case'C':
      case'D':
      case'E':
      case'F': return ch-'A'+10;
      case'0':
      case'1':
      case'2':
      case'3':
      case'4':
      case'5':
      case'6':
      case'7':
      case'8':
      case'9': return ch-'0';
      default:  return DINRAIL_CONVERSION_EROR;
  }

}
uint32_t DinRailBus::hex2val32(uint8_t *str, uint8_t numnibbles)
{
uint8_t cnt, dummy;
uint32_t retval=0;
  if(numnibbles>8) return DINRAIL_ERROR;//error
  for(cnt=0;cnt<numnibbles;cnt++)
  {
    dummy = hex2valnibble(str[cnt]);
    if(dummy>15) return DINRAIL_ERROR;//string error
    retval<<=4;
    retval+=dummy;
  }
  return retval;
}
