/*
	DinRailRelais example for In-Circuit DIN Rail Relais
	Information can be found on www.in-circuit.de or wiki.in-circuit.de

	This example demonstrates how to access DIN Rail Relais using the DinRailBus library

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

#include <SoftwareSerial.h>
#include <DinRailRelais.h>

void setup(void)
{
  Serial.begin(57600);
  //Wait until the serial port is available (useful only for the Leonardo)
  //As the Leonardo board is not reseted every time you open the Serial Monitor
  while(!Serial);
  delay(500);  //5 seconds delay for enabling to see the start up comments on the serial board
  Serial.println(F("Arduino setup"));
  Serial.println(F("Set line ending to newline to send data from the serial monitor"));
}

DinRailBus * railBus = new DinRailBus(11,10,12);  // Initialize the railbus interface with pins 11:TX, 10:RX, 12:TXEN (If RS485 functionality is not needed TXEN pin should be set to -1)
DinRailRelais *DinRailRelay1 = new DinRailRelais(railBus, 0x03);  // Connected on the railbus is a DinRailRelais with address 0x03
DinRailRelais *DinRailRelay2 = new DinRailRelais(railBus, 0x04);  // Connected on the railbus is a DinRailRelais with address 0x03

// the loop routine runs over and over again forever:
void loop() {
  uint32_t stat;
  static uint16_t i=0;
  static uint32_t  mask=0x0001;
  
  DinRailRelay1->relaySet(mask, 0, 0);  // Set relays temporary on DinRailRelay1
  
  stat = DinRailRelay1->relayGet(100);  // Read status of the relais on DinRailRelay1
  if (DinRailRelay1->getLastType() == 'K')  // If the answer from DinRailRelay1 was a read acknowledge
  {
    DinRailRelay2->relaySet(stat, 0, 0);  // Set relays temporary on DinRailRelay2 like the ones on DinRailRelay1
  }
  
  mask<<=1;
  if(mask>0x3F) {
    mask =0x0001;
  }
}

