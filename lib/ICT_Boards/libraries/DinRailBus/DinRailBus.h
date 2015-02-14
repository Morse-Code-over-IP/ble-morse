/*
	DinRailBus.h - Library for access to the DinRailBus developed by In-Circuit

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

#ifndef DINRAILBUS_H
#define DINRAILBUS_H

#include <inttypes.h>
#include <Arduino.h>
#include <SoftwareSerial.h>

#define DINRAIL_ERROR   0xFFFFFFFF
#define DINRAIL_TIMEOUT 0xFEFFFFFF
#define DINRAIL_CONVERSION_EROR  0xFF
#define DINRAIL_MINDELAY 5  // Wait at least this long for an answer on the bus

// Possible commands==packet types
enum DinRailBus_commands
{
	DRBC_READ		= 'R',
	DRBC_WRITE		= 'W',
	DRBC_SYNC		= 'S',
	DRBC_READ_ACK	= 'K',
	DRBC_WRITE_ACK	= 'I',
	DRBC_NACK		= 'N',
	DRBC_NACK_DATA	= 'n',
};

// Possible execution results
enum DinRailBus_nack_types
{
	DRB_nACK_RECV		= 0x01,		// Req received
	DRB_nACK_RETRANS		= 0x02,		// Req vorher schonmal erhalten und wird bearbeitet
	DRB_nACK_DONE_BEFORE	= 0x03,		// Req vorher schonmal erhalten und wurde abgeschlossen
	DRB_nNACK_UNKNOWN	= 0x06,		// Register unbekannt
	DRB_nNACK_FORBIDD	= 0x07,		// zugriff verboten (WriteOnly / Readonly)
	DRB_nNACK_BUSY		= 0x08,		// Req erhalten aber bereits mit anderem Cmd beschäftigt
	DRB_nNACK_INVALID	= 0x09,		// Req ungültig (ungültiger Wert, init fehler..)
	DRB_nNACK_ABORT		= 0x0A,		// Ausführung abgebrochen (Fehler, Timeout..)
	DRB_nNACK_ABORT_BEF	= 0x0B,		// Ausführung wurde schonmal abgebrochen
	DRB_nACK_SYNCED		= 0x10,		// Sync erfolgreich
};

// Structure of a railbus packet
struct _dinRailBusContent
{
	uint8_t mw[2];
	uint8_t addr[2];
	uint8_t cmd;
	uint8_t sadr[2];
	uint8_t data[8];
	uint8_t chk[2];
};

// union to access indexed bytes in the railbus packet
typedef union
{
	uint8_t bytes[sizeof(struct _dinRailBusContent)];
  struct _dinRailBusContent content;
} dinRailBusPacket;

// Class modelling the Communication interface to the railbus
class DinRailBus
{
private:
  SoftwareSerial * mp_rs485;
  dinRailBusPacket m_packetbuf;
	uint8_t m_parserPos;

  uint8_t  checkpacket(uint8_t chr);

	uint8_t  ishex(uint8_t chr);
  uint8_t  hex2valnibble(uint8_t ch);
  uint32_t hex2val32(uint8_t *str, uint8_t numnibbles);
public:
  uint8_t txpin;
  uint8_t rxpin;
  int8_t txenpin;
  DinRailBus(uint8_t tx=0, uint8_t rx=1, int8_t txen=2)
  {
	  txpin = tx;
	  rxpin = rx;
	  txenpin = txen;
	  mp_rs485 = new SoftwareSerial(rxpin,txpin);
	  pinMode(rxpin, INPUT);     //RS485-rxd
	  pinMode(txpin, OUTPUT);    //RS485-txd
	  if (txenpin>=0) pinMode(txenpin, OUTPUT);  //RS485-txenable
	  mp_rs485->begin(9600);
	}
  ~DinRailBus()
	{
		delete mp_rs485;
	}

  uint32_t read(uint8_t madr, uint8_t sadr, uint16_t maxdelay);
  uint32_t write(uint8_t madr, uint8_t sadr, uint32_t data, uint16_t maxdelay);

  dinRailBusPacket getLastPacket();
	uint8_t getLastType();
};

#endif