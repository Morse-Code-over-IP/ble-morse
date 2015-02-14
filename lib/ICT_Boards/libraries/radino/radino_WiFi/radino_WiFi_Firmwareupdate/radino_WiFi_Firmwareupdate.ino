/*
	Radino-WiFi-Firmwareupdate Example for In-Circuit radino WiFi modules
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
  This example demonstrates how to build a WiFi-UART-Bridge with
  radino WiFi and setup the ESP8266 for a Firmwareupdate.
    
  This example enables you to directly communicate with the bootloader of the
  ESP8266 WiFi-chip which is integrated on the radino WiFi. It can be used to
  upload new firmware to the ESP8266 WiFi-module.
  
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

/*
 * Setup connections for Firmware-Update of ESP8266 WiFi-Chip
*/
void ESP8266_setFirmwareUpdateMode()
{
  pinMode(CH_PD, OUTPUT);  // Chip-Enable of ESP8266
  digitalWrite(CH_PD, HIGH);  // HIGH
  
  pinMode(GPIO0, OUTPUT);  // GPIO0 of ESP8266
  digitalWrite(GPIO0, LOW);  // LOW
  
  // GPIO of SC16 is connected to GPIO2 of ESP8266
  SC16_GPIO_pinMode(GPIO2, SC16_GPIO_OUTPUT);  // GPIO2 of ESP8266
  SC16_GPIO_digitalWrite(GPIO2, 1);  // HIGH
  
  // Perform RESET of ESP8266
  pinMode(CH_RST,OUTPUT);
  digitalWrite(CH_RST, LOW);
  delay(200); 
  digitalWrite(CH_RST, HIGH); 
}

void setup()
{
  progSerial.begin(57600);  // Start serial port at baudrate 57600
  delay(100);  
  progSerial.println("Hello");  // Print a welcome message

  // Init SPI-UART-Module
  SC16_init();  
  
  // Set baudrate to 115200
  // ESP8266 Bootloader for Firmware-Update
  // uses this baudrate
  SC16_setbaudRate(115200);
  
  // Put ESP8266 into FirmwareUpdateMode
  ESP8266_setFirmwareUpdateMode();
  
  delay(250);
  SC16_flush();  // Clear UART-buffers
}

/*
 * "Connect" external UART to SPI-UART
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

