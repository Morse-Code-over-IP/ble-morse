/* Copyright (c) 2014, Nordic Semiconductor ASA
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/* Modified by In-Circuit GmbH to operate with In-Circuit radino nRF8001
  IO-Test for radino on ICradio Leonardo
  Every IO-Pin is turned on for N milliseconds in a loop
  D/IO[0..13] N=IO + 10ms   IO5 -> 15ms pulse
  A[0..5]     N=A + 30ms    A3 -> 35ms (Note that the JTAG-EN Fuse has to be reset!)
  USB-Test: connect uart via USB
  HF-Test: UART demo via Bloetooth, use Nordic App "nRF UART" to connect to the device
*/

#include <SPI.h>
#include <lib_aci.h>
#include <aci_setup.h>
#include "uart_over_ble.h"

/**
Put the nRF8001 setup in the RAM of the nRF8001.
*/
#include "services.h"
/**
Include the services_lock.h to put the setup in the OTP memory of the nRF8001.
This would mean that the setup cannot be changed once put in.
However this removes the need to do the setup of the nRF8001 on every reset.
*/

#include <SoftwareSerial.h>
#include <DinRailRelais.h>

#define RB_TXEN 12
#define RB_TX   11
#define RB_RX   10

DinRailBus * railBus = new DinRailBus(RB_TX, RB_RX, RB_TXEN);
char BLEstr[20] = {0};
uint8_t BLEstrsize = 0;

/*
Description:

In this template we are using the BTLE as a UART and can send and receive packets.
The maximum size of a packet is 20 bytes.
When a command it received a response(s) are transmitted back.
Since the response is done using a Notification the peer must have opened it(subscribed to it) before any packet is transmitted.
The pipe for the UART_TX becomes available once the peer opens it.
See section 20.4.1 -> Opening a Transmit pipe
In the master control panel, clicking Enable Services will open all the pipes on the nRF8001.

The ACI Evt Data Credit provides the radio level ack of a transmitted packet.
*/

void setup(void)
{
  Serial.begin(57600);
  //Wait until the serial port is available (useful only for the Leonardo)
  //As the Leonardo board is not reseted every time you open the Serial Monitor
  while(!Serial);
  delay(500);  //5 seconds delay for enabling to see the start up comments on the serial board
  Serial.println(F("Arduino setup"));
  Serial.println(F("Set line ending to newline to send data from the serial monitor"));
  
  setup_bluetooth();
  
  // initialize the digital pin as an output.
//  pinMode(6, OUTPUT);     
//  pinMode(RB_RX, INPUT);     //485-rxd
//  pinMode(RB_TX, OUTPUT);    //485-txd 
//  pinMode(13, OUTPUT);    
//  pinMode(5, OUTPUT);     
//  pinMode(A5, OUTPUT);     
//  pinMode(0, OUTPUT);     
//  pinMode(1, OUTPUT);     
//  pinMode(A1, OUTPUT);     
//  pinMode(A2, OUTPUT);     
//  pinMode(A0, OUTPUT);     
//  pinMode(A3, OUTPUT);     
//  pinMode(2, OUTPUT);     
//  pinMode(3, OUTPUT);     
//  pinMode(RB_TXEN, OUTPUT);  //485-txenable
}

///////////////////////////////////////////////////////////////////////////////////////////
uint8_t  hex2valnibble(char ch)
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
      default:  return 0xFF;//fehler!
  }
}

uint32_t hex2val32(char *str, uint8_t numnibbles)
{
uint8_t cnt, dummy;
uint32_t retval=0;
  if(numnibbles>8) return 0xFFFFFFFF;//error
  for(cnt=0;cnt<numnibbles;cnt++)
  {
    dummy = hex2valnibble(str[cnt]);
    if(dummy>15) return 0xFFFFFFFF;//string error
    retval<<=4;
    retval+=dummy;
  }
  return retval;
}
///////////////////////////////////////////////////////////////////////////////////////////

// the loop routine runs over and over again forever:
void loop() {
  uint32_t BLEval=0;
  static uint8_t module = 0x03;
  static uint32_t  mask=0x0000001; 
  DinRailRelais * dinRailRelay;
  
  doradiostuff(); 
  
  if(BLEstrsize)
  {
    switch(BLEstr[0]){
    // Set relays temporary
    case 's':
        if (BLEstrsize<5)
        {
          lib_aci_send_data(PIPE_UART_OVER_BTLE_UART_TX_TX, (uint8_t*)"ERR", 3);
          break;
        }
        BLEval = hex2val32(BLEstr+1, 4);
        module = BLEval>>8;
        mask = BLEval&0xFF;
        dinRailRelay = new DinRailRelais(railBus, module);
        dinRailRelay->relaySet(mask, 0, 100);
        if (dinRailRelay->getLastType()=='I')
          lib_aci_send_data(PIPE_UART_OVER_BTLE_UART_TX_TX, (uint8_t*)"OK", 2);
        else
          lib_aci_send_data(PIPE_UART_OVER_BTLE_UART_TX_TX, (uint8_t*)"NOK", 3);
        delete dinRailRelay;
        break;
    // Set relays and save in EEPROM
    case 'S':
        if (BLEstrsize<5)
        {
          lib_aci_send_data(PIPE_UART_OVER_BTLE_UART_TX_TX, (uint8_t*)"ERR", 3);
          break;
        }
        BLEval = hex2val32(BLEstr+1, 4);
        module = BLEval>>8;
        mask = BLEval&0xFF;
        dinRailRelay = new DinRailRelais(railBus, module);
        dinRailRelay->relaySet(mask, 1, 100);
        if (dinRailRelay->getLastType()=='I')
          lib_aci_send_data(PIPE_UART_OVER_BTLE_UART_TX_TX, (uint8_t*)"OK", 2);
        else
          lib_aci_send_data(PIPE_UART_OVER_BTLE_UART_TX_TX, (uint8_t*)"NOK", 3);        
        delete dinRailRelay;
        break;        
    // count up relays
    case '+':
        ++mask &= 0x3F;
        if (mask==0x3F) mask=0;
        dinRailRelay = new DinRailRelais(railBus, module);
        dinRailRelay->relaySet(mask, 0, 100);
        if (dinRailRelay->getLastType()=='I')
          lib_aci_send_data(PIPE_UART_OVER_BTLE_UART_TX_TX, (uint8_t*)"OK", 2);
        else  
          lib_aci_send_data(PIPE_UART_OVER_BTLE_UART_TX_TX, (uint8_t*)"NOK", 3);
        delete dinRailRelay;
        break;
    // shift relays
    case '*':
        mask<<=1;
        mask &= 0x3F;
        if(mask==0) mask =1;
        dinRailRelay = new DinRailRelais(railBus, module);
        dinRailRelay->relaySet(mask, 0, 100);
        if (dinRailRelay->getLastType()=='I')
          lib_aci_send_data(PIPE_UART_OVER_BTLE_UART_TX_TX, (uint8_t*)"OK", 2);
        else  
          lib_aci_send_data(PIPE_UART_OVER_BTLE_UART_TX_TX, (uint8_t*)"NOK", 3);        
        delete dinRailRelay;
        break;                
   } // switch
   BLEstrsize=0;
  }
}

