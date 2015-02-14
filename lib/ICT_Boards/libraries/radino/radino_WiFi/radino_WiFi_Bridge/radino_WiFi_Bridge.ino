/*
	Radino-WiFi-Bridge-Example for In-Circuit radino WiFi modules
        for more information: www.in-circuit.de or www.radino.cc

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
/* 
  This example demonstrates how to setup the UART-communication with the
  ESP8266 Wifi-Chip.
  
  When connecting to radino WiFi, always set baudrate to 57600 max.
  This baudrate does not effect the baudrate of ESP8266 WiFi-Chip.
  
  This example enables you to directly communicate with the ESP8266 WiFi-
  chip which is integrated on the radino WiFi. This example can
  be used for debugging, e.g. testing the various commands available
  to communicate with the ESP8266 WiFi-chip.
  
  The SC16IS750 enables the use of baudrates greater than 57600 when
  communicating with ESP8266. (The Arduino runs at 8 Mhz, therfore limited
  to baudrates less than 57600)
  
- radino WiFi setup:
  
  Arduino <---SPI---> SC16IS750 <---UART---> ESP8266
  
- Internal connections between Arduino(ATmega32U4),SC16IS750 and ESP8266:
  
  Arduino          ESP8266        SC16IS750
  
  A4               CH_PD          ---
  D4               RST            ---
  D9               GPIO0          ---
  ---              GPIO2          GPIO0
  
- Pin setup for ESP8266:

  Pin            normal operation        Firmware-Update-Mode
  
  CH_PD          HIGH                    HIGH
  GPIO0          HIGH                    LOW
  GPIO2          HIGH                    HIGH
  RST            HIGH                    HIGH
  
*/

// Use SPI-library for communication with SC16IS750
#include <SPI.h>

// Include definitions and functions for SC16IS750
// SPI-to-UART-chip
#include "SC16IS750.h"

// Pin definitions of ESP8266
//       Pin-Name   Arduino-Pin-Number
#define  CH_PD      A4
#define  CH_RST     4
#define  GPIO0      9
//       Pin-Name   SC16IS750-GPIO-Number
#define  GPIO2      0

// Define Serial port that should be used for communication
// Valid values:
// USB-UART:       Serial
// Hardware-UART:  Serial1
#define progSerial Serial

// Set baudrate of SPI-UART chip to communicate with ESP8266
// e.g. Firmware Version 9.2.2 uses baudrate 9600 by default, but it can be modified by command AT+CIOBAUD=9600
// older Firmware Versions use baudrate 115200
#define ESP8266_BAUDRATE  9600

/*
 * Setup connections for normal operation of ESP8266 WiFi-Chip
*/
void ESP8266_setNormalOperationMode()
{
  pinMode(CH_PD, OUTPUT);  // Chip-Enable of ESP8266
  digitalWrite(CH_PD, HIGH);  // HIGH
  
  pinMode(GPIO0, OUTPUT);  // GPIO0 of ESP8266
  digitalWrite(GPIO0, HIGH);  // HIGH
  
  // GPIO of SC16 is connected to GPIO2 of ESP8266
  SC16_GPIO_pinMode(GPIO2, SC16_GPIO_OUTPUT);  // GPIO2 of ESP8266
  SC16_GPIO_digitalWrite(GPIO2, 1);  // HIGH
  
  // Perform RESET of ESP8266
  pinMode(CH_RST,OUTPUT);
  digitalWrite(CH_RST, LOW);  // Pull RESET of WIFI-Chip
  delay(200);
  digitalWrite(CH_RST, HIGH);  // Disable RESET of WIFI-Chip
}

void setup()
{
  // When connecting to radino WiFi, always set baudrate to 57600 max.
  // This baudrate does not effect the baudrate of ESP8266 WiFi-Chip
  progSerial.begin(57600);  // Start serial port at baudrate 57600
  delay(100);  
  progSerial.println("Hello");  // Print a welcome message

  // Init SPI-UART-Module
  SC16_init();  
  
  // Set Baudrate of SPI-UART-Module
  SC16_setbaudRate(ESP8266_BAUDRATE);
  
  // Put ESP8266 into normal operation mode
  ESP8266_setNormalOperationMode();
  
  delay(250);
  SC16_flush();  // Clear UART-buffers
}

/*
 * Connect external UART to ESP8266-UART through SC17IS750 SPI-UART-Chip
 * This enables direct communication with ESP8266 WiFi-Chip
*/
void loop()
{ 
  char data = '0';
  
  // If data available from ESP8266, pass through
  // to external UART
  if(SC16_available())
  {
    data = SC16_read();
    progSerial.write(data); 
  }
  
  // If data available from external UART, pass through
  // to ESP8266
  if(progSerial.available())
  {
    data = progSerial.read();
    SC16_write(data); 
  }
}

