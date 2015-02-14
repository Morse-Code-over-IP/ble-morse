/*
        SC16IS750 SPI-to-UART-Chip on In-Circuit radino WiFi modules
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
 * Simple implementation of basic functions for
 * communication over SPI-UART, similar to 'Serial' library
 *
 * -> print(String)
 * -> println(String)
 * -> print(int)
 * -> println(int)
 * -> available()
 * -> txavailable()
 * -> read()
 * -> write(char)
 *
 * Refer to SC16IS750.h for pin and register definitions
*/

// Use SPI-library for communication with SC16IS750
#include <SPI.h>

// Init SC16IS750
uint8_t SC16_init()
{
  // start the SPI library:
  SPI.begin();  
  //Data is transmitted and received MSB first
  SPI.setBitOrder(MSBFIRST);  
  //SPI interface will run at 1MHz if 8MHz chip or 2Mhz if 16Mhz
  SPI.setClockDivider(SPI_CLOCK_DIV8);   
  //Data is clocked on the rising edge and clock is low when inactive
  SPI.setDataMode(SPI_MODE0);
  
  // initalize chip select pin:
  pinMode(SC16_CS,OUTPUT);  
  //Clear Chip Select
  digitalWrite(SC16_CS,HIGH);
  
  // Setup UART: 8N1
  // 8 databits, no parity, 1 stop-bit
  uint8_t uartSetting = (0<<LCR_DIVISOR) | (0<<LCR_BREAK) | (0<<LCR_PARITY_FORCED) | (0<<LCR_PARITY_TYPE) | (0<<LCR_PARITY_EN) | (0<<LCR_STOP) | (3<<LCR_WORD_LENGTH);  // 8N1
  
  ///Line Control Register:
  // Save UART settings
  writeRegister(LCR,uartSetting);
  
  //Modem Control Register
  //Normal Operating Mode
  writeRegister(MCR,0x00);
 
  //FIFO Control Register: Clear & Enable the FIFO and no other features
  SC16_flush();
}

// Clear RX & TX-buffer and enable FIFO
void SC16_flush()
{
  //FIFO Control Register: Clear & Enable the FIFO and no other features
  writeRegister(FCR,0x07);
}

void SC16_print(const String dataString)
{
  SC16_print(dataString.c_str());
}

void SC16_println(const String dataString)
{
  SC16_println(dataString.c_str());
}

// Print a string over UART
void SC16_print(const char * dataString)
{
  const unsigned char * pStr;
//  Serial1.print(dataString);
  for (pStr=(const unsigned char *)dataString ; *pStr!='\0' ; pStr++)
  {
    SC16_write(*pStr);
  }

//  uint8_t i;
//  for(i=0;i<dataString.length();i++)
//  {
//    SC16_write(dataString[i]);
//  }
}

// Print a string over UART and append CR & LF
void SC16_println(const char * dataString)
{
  SC16_print(dataString);
  SC16_write(13);  // CR - Carriage Return
  SC16_write(10);  // LF - Line Feed
}

// Print a number over UART as a string
void SC16_print(uint16_t dat)
{
  char number[10];  
  SC16_print(itoa(dat, number, 10));
}

// Print a number over UART as a string and append CR & LF
void SC16_println(uint16_t dat)
{
  SC16_print(dat);
  SC16_write(13);
  SC16_write(10);
}

// Get number of available bytes of the UART
// that wait to be received
uint8_t SC16_available(void)
{
  return readRegister(RXLVL);
}

// Get number of available bytes within the
// UART's TX-Buffer. If data is sent while this returns 0,
// all sent data will be lost and not transmitted, because
// the buffer is already full.
uint8_t SC16_txavailable(void)
{
  return readRegister(TXLVL);
}

// Read one Byte from UART
uint8_t SC16_read(void)
{
  return readRegister(RHR);
}

// Write one Byte to UART
void SC16_write(uint8_t data)
{
  while(!SC16_txavailable()) delay(1);//wait until TX-buffer is ready to receive at least one character
//  Serial1.write(data);
  writeRegister(THR,data); 
}

// Set GPIO-Pin direction/mode
// 1 = Output == SC16_GPIO_INPUT
// 0 = Input == SC16_GPIO_OUTPUT
void SC16_GPIO_pinMode(uint8_t pin, uint8_t mode)
{
  if(pin>7) return;  // wrong pin number, return
  uint8_t state = readRegister(IODir); // Get current pin setup
  
  // Only modify the corresponding bit of the desired pin
  if(mode == SC16_GPIO_OUTPUT){
    state |= (1<<pin);  // Set Output
  }else {
    state &= ~(1<<pin);  // Set Input
  }
  writeRegister(IODir, state);  // Save new pin setup
}

// Set value of GPIO pin
// 0 = LOW
// 1 = HIGH
void SC16_GPIO_digitalWrite(uint8_t pin, uint8_t value)
{
  if(pin>7) return;  // wrong pin number, return
  uint8_t state = readRegister(IOState); // Get current pin setup
  
  // Only modify the corresponding bit of the desired pin
  if(value){
    state |= (1<<pin);  // Set High
  }else {
    state &= ~(1<<pin);  // Set Low
  }
  writeRegister(IOState, state);  // Save new pin value
}

// Read value of GPIO pin
// 0 = LOW
// 1 = HIGH
uint8_t SC16_GPIO_digitalRead(uint8_t pin)
{
  if(pin>7) return 0;  // wrong pin number, return
  uint8_t state = readRegister(IOState); // Get current pin setup
  
  state &= 1<<pin;  // Get corresponding bit
  
  if(state) return 1;
    else return 0;  
}

// Set Baudrate of SC16IS750
void SC16_setbaudRate(uint32_t baudrate)
{
  uint8_t help = 0x00;
  
  uint32_t divisor = 0;
  
  // e.g. Baudrate: 19200; divisor = (14.7456Mhz/ (16*19200) ) = 48
  divisor = (uint32_t)14745600 / 16;
  divisor = divisor / baudrate;
    
  help = readRegister(LCR);
  help |= (1<<7);  // Set Bit7 to 1 to enable special register set
  writeRegister(LCR,help);
  
  // DLL & DLH are part of the special register set
  writeRegister(DLL,divisor & 0xFF);  // Set Baudrate divisor, Low byte
  writeRegister(DLH,divisor >>8);  // High byte
  
  help &= ~(1<<7); // Set Bit7 to 0 to disable special register set
  writeRegister(LCR,help);
}

// Writes a value to a specified register over SPI
void writeRegister(uint8_t thisRegister, uint8_t thisValue) {
  // SC16IS740 expects a R/W  bit first, followed by the 4 bit
  // register address of the byte.
  // So shift the bits left by three bits:
  thisRegister = thisRegister << 3;
  // now combine the register address and the command into one byte:
  uint8_t dataToSend = thisRegister | WRITE;

  // take the chip select low to select the device:
  digitalWrite(SC16_CS, LOW);

  SPI.transfer(dataToSend); //Send register location
  SPI.transfer(thisValue);  //Send value to record into register

  // take the chip select high to de-select:
  digitalWrite(SC16_CS, HIGH);

}

// Reads a value from a specified register over SPI
byte readRegister(uint8_t thisRegister) {
  uint8_t readback;
  // SC16IS740 expects a R/W  bit first, followed by the 4 bit
  // register address of the byte.
  // So shift the bits left by three bits:
  thisRegister = thisRegister << 3;
  // now combine the register address and the command into one byte:
  uint8_t dataToSend = thisRegister | READ;

  // take the chip select low to select the device:
  digitalWrite(SC16_CS, LOW);
  
  SPI.transfer(dataToSend); //Send register location
  readback = SPI.transfer(0);  //Get Value from register
  
  digitalWrite(SC16_CS, HIGH);
  return(readback);
} 


