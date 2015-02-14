/*
        Radino-WiFi ESP based webserver for In-Circuit radino WiFi modules on Spider
        for more information: www.in-circuit.de or www.radino.cc

	Copyright (c) 2014 In-Circuit GmbH

        v0.2 - 2014.12.19
            Some bugfixes and optimizations
            Included DinRail PWM-Driver
            Included DinRail Relais 6I6S
        v0.1 - 2014.10.28
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
unsigned char reg1013 = 128;  //D13 (onboard LED)
char reg1113[] = {'O','U','T','\0'};

//Registers for LED-Dimmer
unsigned char reg6000 = 0;  //Module address
unsigned long reg6001 = 0;  //Ch1
unsigned long reg6002 = 0;  //Ch2
unsigned long reg6003 = 0;  //Ch3
unsigned long reg6004 = 0;  //Ch4
unsigned long reg6005 = 0;  //Ch5
unsigned long reg6006 = 0;  //Ch6
unsigned long reg6007 = 0;  //Ch7
unsigned long reg6008 = 0;  //Ch8
                            //6009: Set all channels
unsigned long reg6010 = 0;  //Available dim steps

//Registers for Rail Relais
unsigned char reg6109 = 0;  //Module address
//reg6100 Register 0
unsigned long reg6101 = 0;  //status cache/Register 1
#define RAILRELAIS_CACHE_TO 1000
unsigned long relaisTimeout = 0;  //cache timeout
//6103 Relais 1
//6104 Relais 1
//6105 Relais 1
//6106 Relais 1
//6107 Relais 1

//Registers for PWM-Driver
//unsigned long reg6201 = 0;  //Channel 1
// ...
//unsigned long reg6215 = 0;  //Channel 15
//unsigned long reg6221 = 0;  //Channel 1 enable
// ...
//unsigned long reg6235 = 0;  //Channel 15 enable
unsigned char reg6250 = 0;  //Module address
unsigned long reg6251 = 0;  //Max Val
//reg6252  //Set all channels
//reg6253  //Global enable
unsigned long reg6254 = 0;  //Enable cache - bitmask
#define RAILPWM_CACHE_TO 1000
unsigned long pwmTimeout = 0;
unsigned long reg6255 = 0;  //Min Val

//Settings for RS485
#define SWSERIAL_SPEED 9600
#define SWSERIAL_BUFFSIZE 192
unsigned char swSerialPins[] = {10,11,12};  //RX,TX,TXEN - Set TEXEN to 0xFF if not used
char swSerialOut[SWSERIAL_BUFFSIZE+4] = {0,};
char swSerialIn[SWSERIAL_BUFFSIZE+4] = {0,};
SoftwareSerial * swSerial;

// Setup Serial connection and init ESP8266-Wifi-Module
void setup()
{
  dbgSerial.begin(DBG_SERIAL_BAUDRATE);  // Serial Baudrate 57600
  delay(100);  // Wait 100 ms
  while(!dbgSerial);  // Wait for Serial port to open, only important when using USB-Serial
//  delay(1000);  // Wait 1 second
  
  dbgSerial.println("Hello");  // Print something to show that Serial connection has established
  
  SC16_init();  // Init SC16IS750 SPI-UART-Module
  SC16_setbaudRate(ESP8266_BAUDRATE);  // Set Baudrate of SPI-UART-Module
  
  delay(250);
  ESP8266_setNormalOperationMode();  // Put ESP8266 into normal operation mode
  SC16_flush();  // Clear UART-buffers
  
  //Set Pins
  pinMode(LED, OUTPUT);  // Set Status LED as OUTPUT
  digitalWrite(LED, LOW);  // Disable Status LED
  
  //Init RS485
  if (swSerialPins[2]!=0xFF) pinMode(swSerialPins[2],OUTPUT);
  swSerial = new SoftwareSerial(swSerialPins[0],swSerialPins[1]);
  swSerial->begin(SWSERIAL_SPEED);
  
  dbgSerial.println("Setup done");
  bridge_timeout=0;
}

//Convert hex char to uint
unsigned char fromhexch(unsigned char ch)
{
  if (ch>='0' && ch<='9')  return (ch-'0');
  if (ch>='a' && ch<='f')  return (ch-'a'+10);
  if (ch>='A' && ch<='F')  return (ch-'A'+10);
  return 0;
}
//convert array of hexchars to uint
unsigned long fromhexar(unsigned char * str, unsigned char len)
{
  unsigned long ret=0;
  unsigned char i=0;
  for (i=0;i<len;i++)
  {
    ret = (ret<<4) + fromhexch(str[i]);
  }
  return ret;
}
//convert string of hexchars to uint
unsigned long fromhex(String * str)
{
  unsigned char i;
  unsigned long ret=0;
  for (i=0 ; i<str->length() ; i++)
  {
    if (str->charAt(i)>='0' && str->charAt(i)<='9')
      ret = (ret<<4) + (str->charAt(i)-'0');
    else if (str->charAt(i)>='a' && str->charAt(i)<='f')
      ret = (ret<<4) + 10+(str->charAt(i)-'a');
    else if (str->charAt(i)>='A' && str->charAt(i)<='F')
      ret = (ret<<4) + 10+(str->charAt(i)-'A');
    else
      break;
  }
  return ret;
}
//is valid hex digit?
#define ishex(x) ((x>='0'&&x<='9')||(x>='a'&&x<='f')||(x>='A'&&x<='F'))
//convert lower nibble ob uint8 to hexchar
#define tohex(x) ((((x)&0xF)<=9) ? (((x)&0xF)+'0') : (((x)&0xF)+'A'-10))
//convert unsigned long to padded hexstring
String tohexlong(unsigned long val)
{
  char ret[9];
  ret[0] = tohex(val>>28);
  ret[1] = tohex(val>>24);
  ret[2] = tohex(val>>20);
  ret[3] = tohex(val>>16);
  ret[4] = tohex(val>>12);
  ret[5] = tohex(val>>8);
  ret[6] = tohex(val>>4);
  ret[7] = tohex(val>>0);
  ret[8] = '\0';
  return ret;
}


//buffer for railbus command
struct rbCmd {
  unsigned char mw[2];
  unsigned char id[2];
  unsigned char type[1];
  unsigned char reg[2];
  unsigned char val[8];
  unsigned char crc[2];
};

//parse swSerialBuffer for railbuscommands
//cnt: number of chars read since last parse
//pCmd: buffer to construct the command in
char rbParse(unsigned short cnt, struct rbCmd * pCmd)
{
  static unsigned char pos = 0;
  unsigned char * buf = (unsigned char *)pCmd;
  unsigned char chr;
  //used to reset the parser
  if (cnt==0 && pCmd==0)
  {
    pos=0;
    return 0;
  }
  //parse new characters
  for ( ; cnt>0 ; cnt--)
  {
    chr = swSerialIn[strlen(swSerialIn)-cnt];
    buf[pos++] = chr;
    switch (pos-1)
    {
      // Magic word part 1
      case 0:
      if (chr!='R')
        pos=0;
      break;
      // Magic word part 2
      case 1:
      if (chr!='P')
        {pos=0;cnt++;}
      break;
      // Device address
      case 2:
      case 3:
      if (!ishex(chr))
        {pos=0;cnt++;}
      break;
      // Command
      case 4:
      if (chr!='R' && chr!='W' && chr!='S' && chr!='K' && chr!='I' && chr!='N' && chr!='n')
        {pos=0;cnt++;}
      break;
      // Sub address
      case 5:
      case 6:
      if (!ishex(chr))
        {pos=0;cnt++;}
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
        {pos=0;cnt++;}
      break;
    } //switch pos
    
    // check if correct dataless packet
    if (pos==8)
    {
      // We currently ignore read commands
      if (pCmd->type[0] == 'R')
        {pos=0;return 0;}
      // We currently ignore sync commands
      if (pCmd->type[0] == 'S')
        {pos=0;return 0;}
      // Checksum currently is ignored so we got a correct packet
      if (pCmd->type[0] == 'N')
        {pos=0;return pCmd->type[0];}
    }
    // check if correct packet with data
    if (pos==16)
    {
      // checksum currently is ignored so we got a valid packet
      {pos=0;return pCmd->type[0];}
    }
  } //while cnt
  return 0;
}

/*
write command to the railbus and interpret answer
id: device ID
wr: is a write request
reg: target register
val: input value and buffer for responses value
crc: do crc
*/
char doRBCmd(unsigned char id, unsigned char wr, unsigned char reg, unsigned long * val, unsigned char crc)
{
  static char cmd[18] = {0,};
  static struct rbCmd * pCmd = (struct rbCmd *)cmd;
  unsigned short i;
  char ret = 0;
  unsigned long timeout=500;
  //Build command
  cmd[0] =  'R';
  cmd[1] =  'P';
  cmd[2] =  tohex(id>>4);
  cmd[3] =  tohex(id>>0);
  cmd[5] =  tohex(reg>>4);
  cmd[6] =  tohex(reg>>0);
  if (wr)
  {
    cmd[4] =  'W';
    cmd[7] =  tohex(*val>>28);
    cmd[8] =  tohex(*val>>24);
    cmd[9] =  tohex(*val>>20);
    cmd[10] = tohex(*val>>16);
    cmd[11] = tohex(*val>>12);
    cmd[12] = tohex(*val>>8);
    cmd[13] = tohex(*val>>4);
    cmd[14] = tohex(*val>>0);
    cmd[15] = tohex(crc>>4);
    cmd[16] = tohex(crc>>0);
    cmd[17] = '\0';
  } else {
    cmd[4] =  'R';
    cmd[7] = tohex(crc>>4);
    cmd[8] = tohex(crc>>0);
    cmd[9] = '\0';
  }
  //Write command on Serial
  strcat(swSerialOut,cmd);
  timeout += millis();
  i=0;
  rbParse(0,0);
  //Wait for response
  while (timeout>millis())
  {
    if (rbParse(handleSwSerial(),pCmd))
    {
      if (fromhexar(pCmd->id,2)!=id) continue;
      switch (pCmd->type[0])
      {
        default:
          break;
        case 'N':
          return -1;
        case 'n':
          return -2;
        case 'K':
          *val=fromhexar(pCmd->val,8);
          return 'K';
        case 'I':
          *val=fromhexar(pCmd->val,8);
          return 'I';
      }
    }
  }
  return 0;
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
    case 6000:  //ID of LED-Dimmer
      if (wrReq)
        reg6000 = (fromhex(value)&0xFF);
      SC16_print("OK,");
      SC16_write(tohex(reg6000>>4));
      SC16_write(tohex(reg6000));
      SC16_println("");
      break;
      
    case 6010:  //Max dimvalue of connected LED-Dimmer
      if (wrReq) { SC16_println("ERR"); break; }
      if (0>=doRBCmd(reg6000, 0, 0x30, &tmp, 0))
      {
        reg6010 = 0;
      } else {
        reg6010 = tmp;
      }
      SC16_print("OK,");
      SC16_println(reg6010);
      break;
      
    case 6009:  //Set all channels of LED-Dimmer
      if (!wrReq) { SC16_println("ERR"); break; }
      tmp=value->toInt();
      if (tmp>reg6010) { SC16_println("ERR"); break; }
      if (0>=doRBCmd(reg6000, 1, 0x39, &tmp, 0)) { SC16_println("ERR"); break; }
      if (tmp!=value->toInt()) { SC16_println("ERR"); break; }
      reg6001=reg6002=reg6003=reg6004=reg6005=reg6006=reg6007=reg6008 = tmp;
      SC16_print("OK,");
      SC16_println(reg6001);
      break;
      
    case 6001:  //LED-Dimmer channel 1
      i = 0x31;
      if (!wrReq)
      {
        if (0>=doRBCmd(reg6000, 0, i, &tmp, 0)) { SC16_println("ERR"); break; }
      } else {
        tmp=value->toInt();
        if (tmp>reg6010) { SC16_println("ERR"); break; }
        if (0>=doRBCmd(reg6000, 1, i, &tmp, 0)) { SC16_println("ERR"); break; }
        if (tmp!=value->toInt()) { SC16_println("ERR"); break; }
      }
      SC16_print("OK,");
      SC16_println(tmp);
      reg6001 = tmp;
      break;
      
    case 6002:  //LED-Dimmer channel 2
      i = 0x32;
      if (!wrReq)
      {
        if (0>=doRBCmd(reg6000, 0, i, &tmp, 0)) { SC16_println("ERR"); break; }
      } else {
        tmp=value->toInt();
        if (tmp>reg6010) { SC16_println("ERR"); break; }
        if (0>=doRBCmd(reg6000, 1, i, &tmp, 0)) { SC16_println("ERR"); break; }
        if (tmp!=value->toInt()) { SC16_println("ERR"); break; }
      }
      SC16_print("OK,");
      SC16_println(tmp);
      reg6002 = tmp;
      break;
      
    case 6003:  //LED-Dimmer channel 3
      i = 0x33;
      if (!wrReq)
      {
        if (0>=doRBCmd(reg6000, 0, i, &tmp, 0)) { SC16_println("ERR"); break; }
      } else {
        tmp=value->toInt();
        if (tmp>reg6010) { SC16_println("ERR"); break; }
        if (0>=doRBCmd(reg6000, 1, i, &tmp, 0)) { SC16_println("ERR"); break; }
        if (tmp!=value->toInt()) { SC16_println("ERR"); break; }
      }
      SC16_print("OK,");
      SC16_println(tmp);
      reg6003 = tmp;
      break;
      
    case 6004:  //LED-Dimmer channel 4
      i = 0x34;
      if (!wrReq)
      {
        if (0>=doRBCmd(reg6000, 0, i, &tmp, 0)) { SC16_println("ERR"); break; }
      } else {
        tmp=value->toInt();
        if (tmp>reg6010) { SC16_println("ERR"); break; }
        if (0>=doRBCmd(reg6000, 1, i, &tmp, 0)) { SC16_println("ERR"); break; }
        if (tmp!=value->toInt()) { SC16_println("ERR"); break; }
      }
      SC16_print("OK,");
      SC16_println(tmp);
      reg6004 = tmp;
      break;
      
    case 6005:  //LED-Dimmer channel 5
      i = 0x35;
      if (!wrReq)
      {
        if (0>=doRBCmd(reg6000, 0, i, &tmp, 0)) { SC16_println("ERR"); break; }
      } else {
        tmp=value->toInt();
        if (tmp>reg6010) { SC16_println("ERR"); break; }
        if (0>=doRBCmd(reg6000, 1, i, &tmp, 0)) { SC16_println("ERR"); break; }
        if (tmp!=value->toInt()) { SC16_println("ERR"); break; }
      }
      SC16_print("OK,");
      SC16_println(tmp);
      reg6005 = tmp;
      break;
      
    case 6006:  //LED-Dimmer channel 6
      i = 0x36;
      if (!wrReq)
      {
        if (0>=doRBCmd(reg6000, 0, i, &tmp, 0)) { SC16_println("ERR"); break; }
      } else {
        tmp=value->toInt();
        if (tmp>reg6010) { SC16_println("ERR"); break; }
        if (0>=doRBCmd(reg6000, 1, i, &tmp, 0)) { SC16_println("ERR"); break; }
        if (tmp!=value->toInt()) { SC16_println("ERR"); break; }
      }
      SC16_print("OK,");
      SC16_println(tmp);
      reg6006 = tmp;
      break;
      
    case 6007:  //LED-Dimmer channel 7
      i = 0x37;
      if (!wrReq)
      {
        if (0>=doRBCmd(reg6000, 0, i, &tmp, 0)) { SC16_println("ERR"); break; }
      } else {
        tmp=value->toInt();
        if (tmp>reg6010) { SC16_println("ERR"); break; }
        if (0>=doRBCmd(reg6000, 1, i, &tmp, 0)) { SC16_println("ERR"); break; }
        if (tmp!=value->toInt()) { SC16_println("ERR"); break; }
      }
      SC16_print("OK,");
      SC16_println(tmp);
      reg6007 = tmp;
      break;
      
    case 6008:  //LED-Dimmer channel 8
      i = 0x38;
      if (!wrReq)
      {
        if (0>=doRBCmd(reg6000, 0, i, &tmp, 0)) { SC16_println("ERR"); break; }
      } else {
        tmp=value->toInt();
        if (tmp>reg6010) { SC16_println("ERR"); break; }
        if (0>=doRBCmd(reg6000, 1, i, &tmp, 0)) { SC16_println("ERR"); break; }
        if (tmp!=value->toInt()) { SC16_println("ERR"); break; }
      }
      SC16_print("OK,");
      SC16_println(tmp);
      reg6008 = tmp;
      break;
      
    case 6109:  //ID of Rail Relais
      if (wrReq)
        reg6109 = (fromhex(value)&0xFF);
      SC16_print("OK,");
      SC16_write(tohex(reg6109>>4));
      SC16_write(tohex(reg6109));
      SC16_println("");
      relaisTimeout = 0;
      break;
      
    case 6100:  //rail Relais Register 0 (set relais temporary)
      i = 0x00;
      if (!wrReq)
      {
        if (0>=doRBCmd(reg6109, 0, i, &tmp, 0)) { SC16_println("ERR"); break; }
      } else {
        tmp=fromhex(value);
        if (0>=doRBCmd(reg6109, 1, i, &tmp, 0)) { SC16_println("ERR"); break; }
      }
      SC16_print("OK,");
      SC16_println(tohexlong(tmp));
      //Cache results
      reg6101 = tmp;
      relaisTimeout = millis()+RAILRELAIS_CACHE_TO;
      break;
      
    case 6101:  //rail Relais Register 1 (set relais persistent)
      i = 0x01;
      if (!wrReq)
      {
        if (0>=doRBCmd(reg6109, 0, i, &tmp, 0)) { SC16_println("ERR"); break; }
      } else {
        tmp=fromhex(value);
        if (0>=doRBCmd(reg6109, 1, i, &tmp, 0)) { SC16_println("ERR"); break; }
      }
      SC16_print("OK,");
      SC16_println(tohexlong(tmp));
      //Cache results
      reg6101 = tmp;
      relaisTimeout = millis()+RAILRELAIS_CACHE_TO;
      break;
      
    case 6103:  //rail Relais relais 1
      i = 0x00;  //DinRailBusRegister
      j = 0x01;  //Bitmask
      if (relaisTimeout<millis())  //refresh cache
      {
        if (0>=doRBCmd(reg6109, 0, i, &tmp, 0)) { SC16_println("ERR"); break; }
        reg6101 = tmp;
        relaisTimeout = millis()+RAILRELAIS_CACHE_TO;
      }
      tmp = reg6101;
      if (wrReq)
      {
        if(fromhex(value)) tmp |= j;
        else tmp &= ~j;
        if (0>=doRBCmd(reg6109, 1, i, &tmp, 0)) { SC16_println("ERR"); break; }
        relaisTimeout = millis()+RAILRELAIS_CACHE_TO;
      }
      SC16_print("OK,");
      if (tmp&j) SC16_println("ON");
      else SC16_println("OFF");
      //Cache results
      reg6101 = tmp;
      break;
      
    case 6104:  //rail Relais relais 2
      i = 0x00;  //DinRailBusRegister
      j = 0x02;  //Bitmask
      if (relaisTimeout<millis())  //refresh cache
      {
        if (0>=doRBCmd(reg6109, 0, i, &tmp, 0)) { SC16_println("ERR"); break; }
        reg6101 = tmp;
        relaisTimeout = millis()+RAILRELAIS_CACHE_TO;
      }
      tmp = reg6101;
      if (wrReq)
      {
        if(fromhex(value)) tmp |= j;
        else tmp &= ~j;
        if (0>=doRBCmd(reg6109, 1, i, &tmp, 0)) { SC16_println("ERR"); break; }
        relaisTimeout = millis()+RAILRELAIS_CACHE_TO;
      }
      SC16_print("OK,");
      if (tmp&j) SC16_println("ON");
      else SC16_println("OFF");
      //Cache results
      reg6101 = tmp;
      break;
      
    case 6105:  //rail Relais relais 3
      i = 0x00;  //DinRailBusRegister
      j = 0x04;  //Bitmask
      if (relaisTimeout<millis())  //refresh cache
      {
        if (0>=doRBCmd(reg6109, 0, i, &tmp, 0)) { SC16_println("ERR"); break; }
        reg6101 = tmp;
        relaisTimeout = millis()+RAILRELAIS_CACHE_TO;
      }
      tmp = reg6101;
      if (wrReq)
      {
        if(fromhex(value)) tmp |= j;
        else tmp &= ~j;
        if (0>=doRBCmd(reg6109, 1, i, &tmp, 0)) { SC16_println("ERR"); break; }
        relaisTimeout = millis()+RAILRELAIS_CACHE_TO;
      }
      SC16_print("OK,");
      if (tmp&j) SC16_println("ON");
      else SC16_println("OFF");
      //Cache results
      reg6101 = tmp;
      break;
      
    case 6106:  //rail Relais relais 4
      i = 0x00;  //DinRailBusRegister
      j = 0x08;  //Bitmask
      if (relaisTimeout<millis())  //refresh cache
      {
        if (0>=doRBCmd(reg6109, 0, i, &tmp, 0)) { SC16_println("ERR"); break; }
        reg6101 = tmp;
        relaisTimeout = millis()+RAILRELAIS_CACHE_TO;
      }
      tmp = reg6101;
      if (wrReq)
      {
        if(fromhex(value)) tmp |= j;
        else tmp &= ~j;
        if (0>=doRBCmd(reg6109, 1, i, &tmp, 0)) { SC16_println("ERR"); break; }
        relaisTimeout = millis()+RAILRELAIS_CACHE_TO;
      }
      SC16_print("OK,");
      if (tmp&j) SC16_println("ON");
      else SC16_println("OFF");
      //Cache results
      reg6101 = tmp;
      break;
      
    case 6107:  //rail Relais relais 5
      i = 0x00;  //DinRailBusRegister
      j = 0x10;  //Bitmask
      if (relaisTimeout<millis())  //refresh cache
      {
        if (0>=doRBCmd(reg6109, 0, i, &tmp, 0)) { SC16_println("ERR"); break; }
        reg6101 = tmp;
        relaisTimeout = millis()+RAILRELAIS_CACHE_TO;
      }
      tmp = reg6101;
      if (wrReq)
      {
        if(fromhex(value)) tmp |= j;
        else tmp &= ~j;
        if (0>=doRBCmd(reg6109, 1, i, &tmp, 0)) { SC16_println("ERR"); break; }
        relaisTimeout = millis()+RAILRELAIS_CACHE_TO;
      }
      SC16_print("OK,");
      if (tmp&j) SC16_println("ON");
      else SC16_println("OFF");
      //Cache results
      reg6101 = tmp;
      break;
      
    case 6108:  //rail Relais relais 6
      i = 0x00;  //DinRailBusRegister
      j = 0x20;  //Bitmask
      if (relaisTimeout<millis())  //refresh cache
      {
        if (0>=doRBCmd(reg6109, 0, i, &tmp, 0)) { SC16_println("ERR"); break; }
        reg6101 = tmp;
        relaisTimeout = millis()+RAILRELAIS_CACHE_TO;
      }
      tmp = reg6101;
      if (wrReq)
      {
        if(fromhex(value)) tmp |= j;
        else tmp &= ~j;
        if (0>=doRBCmd(reg6109, 1, i, &tmp, 0)) { SC16_println("ERR"); break; }
        relaisTimeout = millis()+RAILRELAIS_CACHE_TO;
      }
      SC16_print("OK,");
      if (tmp&j) SC16_println("ON");
      else SC16_println("OFF");
      //Cache results
      reg6101 = tmp;
      break;
      
    case 6250:  //ID of PWM-Driver
      if (wrReq)
        reg6250 = (fromhex(value)&0xFF);
      SC16_print("OK,");
      SC16_write(tohex(reg6250>>4));
      SC16_write(tohex(reg6250));
      SC16_println("");
      relaisTimeout = 0;
      break;
      
    case 6251:  //PWM-Driver max val
      if (wrReq) { SC16_println("ERR"); break; }
      if (0>=doRBCmd(reg6250, 0, 0xC0, &tmp, 0))
      {
        reg6251 = 0;
      } else {
        reg6251 = tmp;
      }
      SC16_print("OK,");
      SC16_println(reg6251);
      break;
      
    case 6255:  //PWM-Driver min val
      if (wrReq) { SC16_println("ERR"); break; }
      if (0>=doRBCmd(reg6250, 0, 0xC1, &tmp, 0))
      {
        reg6255 = 0;
      } else {
        reg6255 = tmp;
      }
      SC16_print("OK,");
      SC16_println(reg6255);
      break;
      
    case 6252:  //Set all PWM-Driver channels
      if (!wrReq) { SC16_println("ERR"); break; }
      tmp=value->toInt();
      if (tmp>reg6251) { SC16_println("ERR"); break; }
      if (0>=doRBCmd(reg6250, 1, 0xA0, &tmp, 0)) { SC16_println("ERR"); break; }
      if (tmp!=value->toInt()) { SC16_println("ERR"); break; }
      SC16_print("OK,");
      SC16_println(tmp);
      break;
      
    case 6253:  //Global enable
      i= 0xB0;
      if (!wrReq)
      {
        if (0>=doRBCmd(reg6250, 0, i, &tmp, 0)) { SC16_println("ERR"); break; }
      } else {
        tmp=value->toInt();
        if (0>=doRBCmd(reg6250, 1, i, &tmp, 0)) { SC16_println("ERR"); break; }
        if (tmp!=value->toInt()) { SC16_println("ERR"); break; }
      }
      SC16_print("OK,");
      SC16_println(tmp);
      break;
      
    case 6201:  //PWM-Driver Channel 1
    case 6202:
    case 6203:
    case 6204:
    case 6205:
    case 6206:
    case 6207:
    case 6208:
    case 6209:
    case 6210:
    case 6211:
    case 6212:
    case 6213:
    case 6214:
    case 6215:
      //Target register: 0x (1+(r-1)/5) (1+(r-1)%5)
      regNum = (regNum-1)%6200;
      i= (( 1+((regNum)/5) )<<4) + ( 1+((regNum)%5) );
      if (!wrReq)
      {
        if (0>=doRBCmd(reg6250, 0, i, &tmp, 0)) { SC16_println("ERR"); break; }
      } else {
        tmp=value->toInt();
        if (tmp>reg6251) { SC16_println("ERR"); break; }
        if (0>=doRBCmd(reg6250, 1, i, &tmp, 0)) { SC16_println("ERR"); break; }
        if (tmp!=value->toInt()) { SC16_println("ERR"); break; }
      }
      SC16_print("OK,");
      SC16_println(tmp);
      break;
      
    case 6221:  //PWM-Driver Channel 1 enable
    case 6222:
    case 6223:
    case 6224:
    case 6225:
    case 6226:
    case 6227:
    case 6228:
    case 6229:
    case 6230:
    case 6231:
    case 6232:
    case 6233:
    case 6234:
    case 6235:
      // RB_PWM_ALL_EN_MASK	= 0xA3
      i = 0xA3;
      j = (1<<(regNum-6221));
      if (pwmTimeout<millis())  //refresh cache
      {
        if (0>=doRBCmd(reg6250, 0, i, &tmp, 0)) { SC16_println("ERR"); break; }
        reg6254 = tmp;
        pwmTimeout = millis()+RAILPWM_CACHE_TO;
      }
      tmp = reg6254;
      if (wrReq)
      {
        if(fromhex(value)) tmp |= j;
        else tmp &= ~j;
        if (0>=doRBCmd(reg6250, 1, i, &tmp, 0)) { SC16_println("ERR"); break; }
        pwmTimeout = millis()+RAILPWM_CACHE_TO;
      }
      SC16_print("OK,");
      if (tmp&j) SC16_println("ON");
      else SC16_println("OFF");
      //Cache results
      reg6254 = tmp;
      break;
      
    case 5555:  //Access RS485
      if (wrReq)
      {
        for (i=0 ; i<value->length() && strlen(swSerialOut)<SWSERIAL_BUFFSIZE ; i++)
        {
          swSerialOut[strlen(swSerialOut)+1] = '\0';
          swSerialOut[strlen(swSerialOut)] = value->charAt(i);
        }
      }
      SC16_print("OK,");
      SC16_println(swSerialIn);
      break;
      
    case 5556:  //access railbus address
      if (!wrReq)
      {
        if (value->length()<4) { SC16_println("ERR"); break; }
        if (0>=doRBCmd(fromhex(&value->substring(0,2)), 0, fromhex(&value->substring(2,4)), &tmp, 0)) { SC16_println("ERR"); break; }
      } else {
        if (value->length()<5) { SC16_println("ERR"); break; }
        tmp = fromhex(&value->substring(4));
        if (0>=doRBCmd(fromhex(&value->substring(0,2)), 1, fromhex(&value->substring(2,4)), &tmp, 0)) { SC16_println("ERR"); break; }
      }
      SC16_print("OK,");
      SC16_println(tohexlong(tmp));
      break;
      
    case 1013:  //value pin13(LED)
      if (wrReq)
      {
        switch ((*value)[0])
        {
          case 'a': reg1013 -= 10; break;
          case 'b': reg1013 -= 1; break;
          case 'c': reg1013 += 1; break;
          case 'd': reg1013 += 10; break;
          default: reg1013 = value->toInt(); break;
        }
        if (strcmp(reg1113,"OUT")==0)
        {
          if (reg1013==0) digitalWrite(13,LOW);
          else if (reg1013==255) digitalWrite(13,HIGH);
          else analogWrite(13,reg1013);
        }
      }
      SC16_print("OK,");
      if (strcmp(reg1113,"DIN")==0)
        SC16_println(digitalRead(13));
      else if (strcmp(reg1113,"AIN")==0)
        SC16_println(analogRead(13));
      else
        SC16_println(reg1013);
      break;
      
    case 1113:  //mode pin13(LED)
      if (wrReq)
      {
        if (*value == String("DIN"))
        {
          strcpy(reg1113,"DIN");
          pinMode(13,INPUT);
        }
        else if (*value == String("AIN"))
        {
          strcpy(reg1113,"AIN");
          pinMode(13,INPUT);
        }
        else if (*value == String("OUT"))
        {
          strcpy(reg1113,"OUT");
          pinMode(13,OUTPUT);
        }
      }
      SC16_print("OK,");
      SC16_println(reg1113);
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

unsigned char handleSwSerial()
{
  char ch;
  unsigned short ret = 0;
  unsigned short i;
  while (swSerial->available())
  {
    //shift if buffer full
    while ((i=strlen(swSerialIn))>=SWSERIAL_BUFFSIZE)
    {
      for(i=0 ; swSerialIn[i]!='\0' ; i++)
      {
        swSerialIn[i] = swSerialIn[i+1];
      }
    }
    ch = swSerial->read();
    //Escape special chars
    switch (ch)
    {
      case '\0':
        swSerialIn[i++] = '\\'; ch = '0';
        ret++;
        break;
      case '\r':
        swSerialIn[i++] = '\\'; ch = 'r';
        ret++;
        break;
      case '\n':
        swSerialIn[i++] = '\\'; ch = 'n';
        ret++;
        break;
      case '\\':
        swSerialIn[i++] = '\\'; ch = '\\';
        ret++;
        break;
    }
    swSerialIn[i++] = ch;
    ret++;
    swSerialIn[i] = '\0';
    return ret;
  }
  //there is something to write
  if (strlen(swSerialOut))
  {
    if (swSerialPins[2]!=0xFF) digitalWrite(swSerialPins[2],HIGH);
    swSerial->write(swSerialOut[0]);
    for (i=0;i<strlen(swSerialOut);i++) swSerialOut[i]=swSerialOut[i+1];
  }
  if (!strlen(swSerialOut) && swSerialPins[2]!=0xFF) digitalWrite(swSerialPins[2],LOW);
  return 0;
}

// Wait for requests
void loop()
{
  char ch;
  unsigned short i;
  
  static char lastchars[5] = {0,};  // Buffer for last received characters from ESP8266
#define LAST_UART_BUFSZ 8
  static char lastuart[LAST_UART_BUFSZ+2] = {0,};  // Last chars on uart
  
  //Handle software serial
  if (!flashmode)
  {
    handleSwSerial();
  }
  
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

