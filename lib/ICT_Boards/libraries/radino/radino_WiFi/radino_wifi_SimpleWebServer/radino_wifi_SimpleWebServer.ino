/*
        Radino-WiFi SimpleWebServer for In-Circuit radino WiFi modules
        for more information: www.in-circuit.de or www.radino.cc

	Copyright (c) 2014 In-Circuit GmbH

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
  This example demonstrates how to build a simple Webserver accesible over WiFi.
  
  Your radino WiFi will try to connect to a defined WiFi-Network and listen on Port 80.
  Your WiFi-Network should provide an IP-address via DHCP to your radino WiFi.
  
  You can now access a simple website with your webbrowser, when entering your radino's
  IP-address, e.g.:
  
  http://192.168.2.23/
  
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

// Enter your WiFi's SSID/name and password
// radino WiFi will connect to this network and
// obtain an IP-address via DHCP
#define SSID  "xxxxxxxxxxxx"
#define PASS  "xxxxxxxxxxxxxxxxxx"

// Use SPI-library for communication with SC16IS750
#include <SPI.h>
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
#define dbgSerial Serial

// Set baudrate of SPI-UART chip to communicate with ESP8266
// e.g. Firmware Version 9.2.2 uses baudrate 9600 by default, but it can be modified by command
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

// Buffer used to store received characters
String buffer;

// Setup Serial connection and init ESP8266-Wifi-Module
void setup()
{
  dbgSerial.begin(57600);  // Serial Baudrate 57600
  delay(100);  // Wait 100 ms
  while(!dbgSerial);  // Wait for Serial port to open, only important when using USB-Serial
  delay(1000);  // Wait 1 second
  
  dbgSerial.println("Hello");  // Print something to show that Serial connection has established
  
  pinMode(LED, OUTPUT);  // Set Status LED as OUTPUT
  digitalWrite(LED, LOW);  // Disable Status LED

  SC16_init();  // Init SC16IS750 SPI-UART-Module
  SC16_setbaudRate(ESP8266_BAUDRATE);  // Set Baudrate of SPI-UART-Module
  ESP8266_setNormalOperationMode();  // Put ESP8266 into normal operation mode
  
  delay(250);
  SC16_flush();  // Clear UART-buffers
  
  wdt_enable(WDTO_8S);  // Enable Watchdog-Timer, Watchdog reset after 8 seconds
  
  // Test if ESP8266-Wifi-Module is ready
  // Send command "AT" and wair for response "OK"
  while(!sendAndWait( "AT","OK","OK", 100, 100, 0));
  wdt_reset();  // Reset Watchdog Timer
  
  // Set ESP8266-Wifi-Module to Mode 1
  // --> 
  while(!sendAndWait("AT+CWMODE=1","no change","OK", 500, 100, 0));
  wdt_disable();  // Disable Watchdog Timer
 
  // Try to connect to specified WiFi-Network
  // This function uses Watchdog-Timer, if connection does not
  // occur within 8 seconds, radino WiFi will reset
  reconnect(); 
  
  delay(100);
  SC16_flush();  // Clear UART-buffers
}

// Wait for HTTP-request and send webpage of requested
void loop()
{ 
  char ch;  // Received Char on dbgSerial
  
  static char lastchars[4] = {0};  // Buffer for last 4 received characters from ESP8266-Wifi-Module
  
  //Process received data from ESP8266
  while (SC16_available() >0 )
  {
    // Read and save new character
    char c = SC16_read();
    lastchars[0]=    lastchars[1];
    lastchars[1]=    lastchars[2];
    lastchars[2]=    c;
    
    // Check last chars for HTTP-Request "GET"
    // and send website
    if (strcmp(lastchars, "GET")==0){
      dbgSerial.print("Send Web Request");
      
      // Wait until ESP8266 is ready
      if(Wait("OK","System Ready",0,1000, 1)==1){
        delay(50);
        SC16_flush();  // Clear UART-buffers
        simple_webserver();  // Send Website
      }      
    }
  }
  
  // Some implemented commands for the radio to be sent by the dbgSerial UART
  if(dbgSerial.available()){
    ch=dbgSerial.read();
    if(ch=='L')
      sendAndWait("AT+CIFSR", ".", ".", 500, 3000,2);  // Get and print IP address
    if(ch=='S')
      sendAndWait("AT+CIPSTATUS", "OK", "UNLINK", 500, 3000,2);  // Get and print Connection status
    if(ch=='R'){
      reconnect();  // Reconnect to specified WiFi-Network
    }    
  }  
}

//  Reconnect to specified WiFi-Network
//  This function shows how a connection could be established
//  using radino WiFi
void reconnect(){
  wdt_enable(WDTO_8S);  // Enable Watchdog-Timer, Watchdog reset after 8 seconds
  digitalWrite(LED, LOW); //  Disable LED: No Connection
  
  //  Try to join network
  dbgSerial.print("connect...");
  connectWiFi(SSID,PASS);
  dbgSerial.println("done");

  //  Set the multiple connection mode
  wdt_reset();  // Reset Watchdog Timer
  dbgSerial.print("Multiple connection mode...");
  while(!sendAndWait("AT+CIPMUX=1","OK","OK",500, 500,0));
  dbgSerial.println("done");
   
  //  Set the server of port 80, wait for response "no change" or "OK"
  wdt_reset();  // Reset Watchdog Timer
  dbgSerial.print("start server...");
  while(!sendAndWait("AT+CIPSERVER=1,80","no change","OK",500,500,0));
  dbgSerial.println("done");
   
  //Wait for IP-address and print it
  wdt_reset();  // Reset Watchdog Timer
  while(!sendAndWait("AT+CIFSR", ".", ".", 500, 3000,0)) delay(1000);
  dbgSerial.println("ip address:");
  sendAndWait("AT+CIFSR", ".", ".", 500, 3000,2);  
  dbgSerial.println();   
  dbgSerial.println( "Webserver restarted!" );
  
  digitalWrite(LED, HIGH); //  Enable LED: Connection established
  wdt_disable();  // Disable Watchdog Timer
}

// Send command 'AT_Command' to ESP8266 and wait for 'AT_Response1' or 'AT_Response2'
// InitialWait:  describes the delay before 'Waiting' starts
// maxwait:  descibres the maximal wait duration, [ms]
// printout:  1:  Print partial response, including found 'AT_Response'
//           >1:  Print whole response from ESP8266
//            0:  Don't print repsonse
// 
// Returns   0=timeout
//           1= response 1 received
//           2= response 2 received
uint8_t sendAndWait(String AT_Command, char *AT_Response1, char *AT_Response2, int initialwait, int maxwait, uint8_t printout){
uint8_t ret;
  buffer = "";  // Clear global buffer
//  dbgSerial.print(AT_Command);  // Debug output of whole communication
  SC16_flush();  // Clear UART-buffers
  SC16_println(AT_Command);  // Send command to ESP8266
  ret = Wait(AT_Response1, AT_Response2, initialwait, maxwait, printout);  // Wait for response
  return(ret);
}

// Wait for 'AT_Response1' or 'AT_Response2'
// InitialWait:  describes the delay before 'Waiting' starts
// maxwait:  descibres the maximal wait duration, [ms]
// printout:  1:  Print partial response, including found 'AT_Response'
//           >1:  Print whole response from ESP8266
//            0:  Don't print repsonse
// 
// Returns   0=timeout
//           1= response 1 received
//           2= response 2 received
uint8_t Wait(char *AT_Response1, char *AT_Response2, int initialwait, int maxwait, uint8_t printout){
int wait = maxwait;
int ch;
  buffer = "";  // Clear global buffer
  delay(initialwait);  // Wait 'initialwait' milliseconds before starting to look for responses
  
  // Only perform limited number of cycles, defined by 'wait'
  while(wait--){
    while ( SC16_available() > 0 ) {
      ch = SC16_read();
      if(ch)  buffer += (char)ch;  // Save new char to buffer if valid (not zero)

      // Check if the desired response 1 was already received
      if(buffer.indexOf(AT_Response1,0)>-1){                          
          if(printout>1) {
            // Read all remaining characters to complete the response within the buffer
            delay(1);
            while ( SC16_available() > 0 ) {
              ch = SC16_read();
              if(ch) buffer += (char)ch;  // Save new char to buffer if valid (not zero)
              delay(1);
            }
            dbgSerial.print(buffer);  // Print whole response
          }          
          if(printout){
            // Print that Response1 was found
            dbgSerial.print(" --> ");
            dbgSerial.println(AT_Response1);
          }
          SC16_flush();  // Clear UART-buffers
          return 1;  // Return 1: Response 1 received
      }
      // Check if the desired response 2 was already received
      if(buffer.indexOf(AT_Response2,0)>-1){
          if(printout>1) {
            // Read all remaining characters to complete the response within the buffer
            delay(1);
            while ( SC16_available() > 0 ) {
              ch = SC16_read();
              if(ch) buffer += (char)ch;  // Save new char to buffer if valid (not zero)
              delay(1);
            }
            dbgSerial.print(buffer);  // Print whole response
          }          
          if(printout){   
            // Print that Response2 was found     
            dbgSerial.print(" --> ");
            dbgSerial.println(AT_Response2);
          }
          SC16_flush();  // Clear UART-buffers
          return 2;  // Return 2: Response 2 received
      }
    }
    delay(1);  // Delay each cycle of 'wait--' by 1 ms
  }
  if(printout) dbgSerial.print(buffer);  // Print whole response, AT_Response1 or AT_Response2 was not found
  return 0;  //  Return 0:  Timeout
}


//  Build and send command to connect to specified Wifi-Network
//  Wait until connected
boolean connectWiFi(String NetworkSSID,String NetworkPASS)
{
  String cmd = "AT+CWJAP=\"";
  cmd += NetworkSSID;
  cmd += "\",\"";
  cmd += NetworkPASS;
  cmd += "\"";   
  while(!sendAndWait(cmd,"OK","OK",500,10000, 0));
} 

// Send Website to ESP8266
void simple_webserver(void) {
static uint32_t numcalls=0;  // Counts how often the Website was called
String website;  // String that contains the 'Website' HTML-Code
// char ch;
 char temp1[10];
 uint8_t ret;
 dtostrf(++numcalls,1,0,temp1);
 website = "<title>radino Webserver</title><H3>Welcome to radino Web Server</H3><p>numcalls = "+ String(temp1) + "</p>";
 // Send Website String
 SC16_print("AT+CIPSEND=0,");
 SC16_println(website.length());
 
 ret=Wait(">","busy",0,1000, 2);  // Wait if 
  switch(ret){
    case 1:
        // Received '>': Last character was sent
        // Now wait until Data was sent
        dbgSerial.print("send...");
        ret = sendAndWait(website,"SEND OK","Unlink",0,1000, 0);
        switch(ret){
        case 1:
          // Received 'SEND OK'-> Everything went as expected
          dbgSerial.println("OK");
          dbgSerial.println("Close");
          sendAndWait("AT+CIPCLOSE=0","OK","OK",500,1, 0);
          break;
        case 2:
          // Received 'Unlink' -> Connection lost
          dbgSerial.println("Unlink->Restart ESP8266");
          //the ESP8266 will not answer after this happened!
          //--> reset it!
          dbgSerial.println("Reset");
          digitalWrite(CH_RST, LOW);  // Pull RESET of WIFI-Chip
          delay(200);
          digitalWrite(CH_RST, HIGH);  // Disable RESET of WIFI-Chip
          reconnect();  // Reconnect to specified WiFi-Network
          break;
        default:
          dbgSerial.println("Timeout!!!");
          break;  
        }
      break;
    case 2:
      // Received 'busy'
      dbgSerial.println("BUSY!->abort");
      break;
    default:
      break; 
  }  
}  
