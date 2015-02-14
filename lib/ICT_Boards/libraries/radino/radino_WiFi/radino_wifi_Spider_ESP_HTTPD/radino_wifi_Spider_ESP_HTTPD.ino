/*
        Radino-WiFi ESP based webserver for In-Circuit radino WiFi modules on Spider
        for more information: www.in-circuit.de or www.radino.cc

	Copyright (c) 2014 In-Circuit GmbH

        v0.1 - 2014.12.19
	    Basic implementation - functions and comments are subject to change

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
  This example demonstrates how to build a communcation partner for the ESP webserver.
  
  The ESP will open an access point with ssid "RADINO-WIFI" and an empty password.
  In default configuration the IP adress of the ESP is 192.168.2.1
  
  The ESP will start an http server on port 80 and forward write and read requests to the Arduino.
  
  You can now access a simple website with your webbrowser, when entering your radino's IP adress.
  http://192.168.2.1/
  
  The SC16IS750 enables the use of baudrates greater than 57600 when communicating with ESP8266.
  (The Arduino runs at 8 Mhz, therfore limited to baudrates less than 57600)
  
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
#include <SoftwareSerial.h>
#include "SC16IS750.h"

// LED to show connection status
#define  LED        13

// Pin definitions of ESP8266
//       Pin-Name   Arduino-Pin-Number
#define  CH_PD      A4
#define  CH_RST     4
#define  GPIO0      9
//       Pin-Name   SC16IS750-GPIO-Number
#define  GPIO2      0

// Using Watchdogtimer f
#include <avr/wdt.h>

// Define Serial port that should be used for communication
// Valid values:
// USB-UART:       Serial
// Hardware-UART:  Serial1
#define dbgSerial Serial1
#define DBG_SERIAL_BAUDRATE 57600

// Set baudrate of SPI-UART chip to communicate with ESP8266
// e.g. Firmware Version 9.2.2 uses baudrate 9600 by default, but it can be modified by command
// older Firmware Versions use baudrate 115200
#define ESP8266_BAUDRATE  115200

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

// Buffer used to store received characters
String buffer;

// Echo esp printout to uart this long after an input
#define BRIDGE_SLIDING_TO 15000
unsigned long bridge_timeout=0;

/* Registers */
unsigned char reg1005 = 128;  //D5
char reg1105[] = {'O','U','T','\0'};
unsigned char reg1006 = 128;  //D6
char reg1106[] = {'O','U','T','\0'};
unsigned char reg1010 = 128;  //D10
char reg1110[] = {'O','U','T','\0'};

// Setup Serial connection and init ESP8266-Wifi-Module
void setup()
{
  dbgSerial.begin(DBG_SERIAL_BAUDRATE);  // Serial Baudrate 57600
  delay(100);  // Wait 100 ms
  
  dbgSerial.println("Hello");  // Print something to show that Serial connection has established
  
  SC16_init();  // Init SC16IS750 SPI-UART-Module
  SC16_setbaudRate(ESP8266_BAUDRATE);  // Set Baudrate of SPI-UART-Module
  
  delay(250);
  ESP8266_setNormalOperationMode();  // Put ESP8266 into normal operation mode
  SC16_flush();  // Clear UART-buffers
  
  //Set Pins
  pinMode(5,OUTPUT);
  pinMode(6,OUTPUT);
  pinMode(10,OUTPUT);
  
  dbgSerial.println("Setup done");
  bridge_timeout=0;
}


/**
Handle requests from the esp8266
regNum: register to access - extracted from parameter register in http get request
value: value to write - extracted from parameter nval in http get request
wrReq: is it a write (1) or read (0) access
**/
void handleRequest(unsigned short regNum, String * value, unsigned char wrReq)
{
  static unsigned short i,j;
  static unsigned long tmp;
  switch (regNum)
  {
    case 1005:  //value pin5
      if (wrReq)
      {
        switch ((*value)[0])
        {
          case 'a': reg1005 -= 10; break;
          case 'b': reg1005 -= 1; break;
          case 'c': reg1005 += 1; break;
          case 'd': reg1005 += 10; break;
          default: reg1005 = value->toInt(); break;
        }
        if (strcmp(reg1105,"OUT")==0)
        {
          if (reg1005==0) digitalWrite(5,LOW);
          else if (reg1005==255) digitalWrite(5,HIGH);
          else analogWrite(5,reg1005);
        }
      }
      SC16_print("OK,");
      if (strcmp(reg1105,"DIN")==0)
        SC16_println(digitalRead(5));
      else if (strcmp(reg1105,"AIN")==0)
        SC16_println(analogRead(5));
      else
        SC16_println(reg1005);
      break;
      
    case 1105:  //mode pin5
      if (wrReq)
      {
        if (*value == String("DIN"))
        {
          strcpy(reg1105,"DIN");
          pinMode(5,INPUT);
        }
        else if (*value == String("AIN"))
        {
          strcpy(reg1105,"AIN");
          pinMode(5,INPUT);
        }
        else if (*value == String("OUT"))
        {
          strcpy(reg1105,"OUT");
          pinMode(5,OUTPUT);
        }
      }
      SC16_print("OK,");
      SC16_println(reg1105);
      break;
      
    case 1006:  //value pin6
      if (wrReq)
      {
        switch ((*value)[0])
        {
          case 'a': reg1006 -= 10; break;
          case 'b': reg1006 -= 1; break;
          case 'c': reg1006 += 1; break;
          case 'd': reg1006 += 10; break;
          default: reg1006 = value->toInt(); break;
        }
        if (strcmp(reg1106,"OUT")==0)
        {
          if (reg1006==0) digitalWrite(6,LOW);
          else if (reg1006==255) digitalWrite(6,HIGH);
          else analogWrite(6,reg1006);
        }
      }
      SC16_print("OK,");
      if (strcmp(reg1106,"DIN")==0)
        SC16_println(digitalRead(6));
      else if (strcmp(reg1106,"AIN")==0)
        SC16_println(analogRead(6));
      else
        SC16_println(reg1006);
      break;
      
    case 1106:  //mode pin6
      if (wrReq)
      {
        if (*value == String("DIN"))
        {
          strcpy(reg1106,"DIN");
          pinMode(6,INPUT);
        }
        else if (*value == String("AIN"))
        {
          strcpy(reg1106,"AIN");
          pinMode(6,INPUT);
        }
        else if (*value == String("OUT"))
        {
          strcpy(reg1106,"OUT");
          pinMode(6,OUTPUT);
        }
      }
      SC16_print("OK,");
      SC16_println(reg1106);
      break;
      
      case 1010:  //value pin10
      if (wrReq)
      {
        switch ((*value)[0])
        {
          case 'a': reg1010 -= 10; break;
          case 'b': reg1010 -= 1; break;
          case 'c': reg1010 += 1; break;
          case 'd': reg1010 += 10; break;
          default: reg1010 = value->toInt(); break;
        }
        if (strcmp(reg1110,"OUT")==0)
        {
          if (reg1010==0) digitalWrite(10,LOW);
          else if (reg1010==255) digitalWrite(10,HIGH);
          else analogWrite(10,reg1010);
        }
      }
      SC16_print("OK,");
      if (strcmp(reg1110,"DIN")==0)
        SC16_println(digitalRead(10));
      else if (strcmp(reg1110,"AIN")==0)
        SC16_println(analogRead(10));
      else
        SC16_println(reg1010);
      break;
      
    case 1110:  //mode pin10
      if (wrReq)
      {
        if (*value == String("DIN"))
        {
          strcpy(reg1110,"DIN");
          pinMode(10,INPUT);
        }
        else if (*value == String("AIN"))
        {
          strcpy(reg1110,"AIN");
          pinMode(10,INPUT);
        }
        else if (*value == String("OUT"))
        {
          strcpy(reg1110,"OUT");
          pinMode(10,OUTPUT);
        }
      }
      SC16_print("OK,");
      SC16_println(reg1110);
      break;
      
    default:
      SC16_println("Err");
  }
}

String regBuf;
unsigned short regNum=0;
unsigned char regParsing=0;
unsigned char writeRequest=0;
unsigned char bootup=1;

#define FLASH_TIMEOUT 5000
#define FLASH_TIME_WAIT 5000
unsigned char flashmode = 0;
unsigned long flash_to = 0;

// Wait for requests
void loop()
{
  char ch;
  unsigned short i;
  
  static char lastchars[5] = {0,};  // Buffer for last received characters from ESP8266
#define LAST_UART_BUFSZ 8
  static char lastuart[LAST_UART_BUFSZ+2] = {0,};  // Last chars on uart
  
  //We are currently uploading the website
  if (flashmode==0xFF)
  {
    while(dbgSerial.available())
    {
      ch=dbgSerial.read();
      SC16_write(ch);
    }
    while (SC16_available()>0)
    {
      char c = SC16_read();
      lastchars[0]=    lastchars[1];
      lastchars[1]=    lastchars[2];
      lastchars[2]=    lastchars[3];
      lastchars[3]=    c;
      dbgSerial.write(c);
      if (strcmp(lastchars, "$FHT")==0)
      {
        flashmode=0;
      }
    }
    bridge_timeout = millis()+BRIDGE_SLIDING_TO;
  }
  else if (flashmode==1)
  {
    //Init Flashing
    dbgSerial.println("Flashing Start in 5s");
    delay(5000);
    digitalWrite(LED, HIGH);
    flash_to = millis() + FLASH_TIME_WAIT;
    ESP8266_setFirmwareUpdateMode();
    flashmode=5;
  }
  else if (flashmode==2)
  {
    //Restart Flashing
    dbgSerial.println("Flashing Restart");
    delay(1000);
    digitalWrite(LED, HIGH);
    flash_to = millis() + FLASH_TIME_WAIT;
    ESP8266_setFirmwareUpdateMode();
    flashmode=5;
  }
  else if (flashmode==5)
  {
    if (flash_to<millis())
    {
      //Transfer End
      dbgSerial.println("Flashing End");
      digitalWrite(LED, LOW);
      ESP8266_setNormalOperationMode();
//      flashmode=0;  //End flashmode
      flashmode=2;  //Repeat flashing
      SC16_flush();
      return;
    }
    while(dbgSerial.available())
    {
      ch=dbgSerial.read();
      SC16_write(ch);
      flash_to = millis()+FLASH_TIMEOUT;
    }
    while (SC16_available())
    {
      ch=SC16_read();
      dbgSerial.write(ch);
    }
  }
  else
  {
    //Process received data from ESP8266
    while (SC16_available()>0)
    {
      // Read and save new character
      char c = SC16_read();
      
      lastchars[0]=    lastchars[1];
      lastchars[1]=    lastchars[2];
      lastchars[2]=    lastchars[3];
      lastchars[3]=    c;
      
      if (regParsing==1)
      {
        //parsing target register
        if (c==',')
        {
          if (writeRequest)
          {
            regParsing++;
          } else {
            //malformed
          }
        }
        else if (c=='\n')
        {
          if (writeRequest)
          {
            //malformed
          } else {
            handleRequest(regNum,0,0);
            regParsing=0;
          }
        }
        else if (c!='\r')
        {
          regNum *= 10;
          regNum += c-'0';
        }
      }
      else if (regParsing==2)
      {
        //parsing parameter for write command
        if (c=='\n')
        {
          handleRequest(regNum,&regBuf,1);
          regParsing=0;
        }
        else if (c!='\r')
        {
          regBuf += c;
        }
      }
      else if (strcmp(lastchars, "SET,")==0)
      {
        writeRequest = 1;
        regNum = 0;
        regBuf = "";
        regParsing=1;
      }
      else if (strcmp(lastchars, "GET,")==0)
      {
        writeRequest = 0;
        regNum = 0;
        regBuf = "";
        regParsing=1;
      }
      //we are still in the echo window
      if (bridge_timeout>millis())
      {
        bridge_timeout = millis()+BRIDGE_SLIDING_TO;
        dbgSerial.write(c);
      }
      else if (bootup)
      {
        if (bootup==2)
          dbgSerial.write(c);
        //Force echo after ESP startup signal
        if (strcmp(lastchars, "$S\r\n")==0)
          bootup=2;
        //ESP established network
        if (strcmp(lastchars, "$P\r\n")==0)
          bootup=0;
      }
      
      //website upload started
      if (!regParsing && strcmp(lastchars, "$FST")==0)
      {
        flashmode = 0xFF;
        return;
      }
    }
    //input from external serial
    while(dbgSerial.available())
    {
      ch=dbgSerial.read();
      
      lastuart[LAST_UART_BUFSZ+1] = 0;
      for (i=LAST_UART_BUFSZ;i>0;i--)
      {
        lastuart[i] = lastuart[i-1];
      }
      lastuart[0] = ch;
      
      if (strstr(lastuart, "HSALF")==lastuart)
      {
        // Start Flashing of ESP8266
        flashmode = 1;
      }
      else if (strstr(lastuart, "OHCEF")==lastuart)
      {
        // Force echo for esp
        bootup = 2;
        dbgSerial.println("Echo forced");
      }
      else
      {
        SC16_write(ch);
        if (ch=='\r')
        {
          SC16_write('\n');
          delay(2);
          if (dbgSerial.available())
          {
            ch=dbgSerial.read();
            if (ch!='\n')
            {
              SC16_write(ch);
            }
          }
        }
      }
      bridge_timeout = millis()+BRIDGE_SLIDING_TO;
    }
  }
}

