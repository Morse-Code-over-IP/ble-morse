/*
	DinRailRelais.h - Library for access to the DinRailrelais developed by In-Circuit using the DinRailBus protocol

	For information about the DIN Rail Bus and devices refer to:
	www.in-circuit.de
	Detailed information for the DIN Rail Relais can be found on:
  http://wiki.in-circuit.de/index.php5?title=DIN_Rail_Relais_6I6S

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

#include "DinRailRelais.h"

///////////////////////////////////////////////////////////////////////////////////////////
//return: upper bytes = state, lower byte = mask
uint32_t DinRailRelais::relaySet(uint8_t rmask, uint8_t permanent, uint16_t maxdelay)
{
	if (permanent)
		return write_sadr(1, rmask, maxdelay);
	else
    return write_sadr(0, rmask, maxdelay);
}

//wait at least for maxdelay ms to receive the answer
uint32_t DinRailRelais::relayGet(uint16_t maxdelay)
{
	return read_sadr(0, maxdelay);
}
