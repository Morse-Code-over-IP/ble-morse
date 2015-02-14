/*
        Pin, register and other definitions for SC16IS750 SPI-UART chip on In-Circuit radino WiFi modules
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

// Pin definitions
#define  SC16_CS    8  // CS connected to Arduino D8
//#define  SC16_MOSI  MOSI
//#define  SC16_MISO  MISO
//#define  SC16_SCK   SCK

#define  SC16_IRQ   7  // IRQ at D7 / INT6

// Defines
#define  WRITE  0x00  // SPI 'write'-mask
#define  READ   0x80  // SPI 'read'-mask

#define SC16_GPIO_INPUT   0x00  // GPIO defined as 'input'
#define SC16_GPIO_OUTPUT  0x01  // GPIO defined as 'output'

// Register definitions

#define  RHR  0x00  // Receive Holding Register; Read only
#define  THR  0x00  // Transmit Holding Register; Write only

#define  IER  0x01  // Interrupt Enable Register; R/W

#define  FCR  0x02  // FIFO Control Register; Write only
#define  IIR  0x02  // Interrupt Identification Register; Read only

#define  LCR  0x03  // Line Control Register; R/W

#define  LCR_DIVISOR        7  // Define for each bit of LCR register
#define  LCR_BREAK          6
#define  LCR_PARITY_FORCED  5
#define  LCR_PARITY_TYPE    4
#define  LCR_PARITY_EN      3
#define  LCR_STOP           2
#define  LCR_WORD_LENGTH    0  // 2 Bits!

#define  LSR  0x05  // Line Status Register; Read only

#define  MCR  0x04  // Modem Control Register; R/W
#define  MSR  0x06  // Modem Status Register; Read only

#define  SPR  0x07  // Scratch Pad Register; Can be used to store data - no effect to the device; R/W

// Only accessible when MCR[2]=1 and EFR[4]=1
#define  TCR  0x06  // R/W
#define  TLR  0x07  // Trigger Level Register; R/W

#define  TXLVL  0x08  // Transmit FIFO level; Read only
#define  RXLVL  0x09  // Receive FIFO level; Read only

#define  IODir      0x0A  // I/O dicrection; R/W
#define  IOState    0x0B  // R/W
#define  IOIntEna   0x0C  // I/O Interrupt Enable; R/W
#define  IOControl  0x0E  // I/O control; R/W

#define  EFCR  0x0F  // Extra Features Control; R/W

// Special register set
// Only accessible when LCR[7]=1
#define  DLL  0x00  // Baud rate divisor; Low byte; R/W
#define  DLH  0x01  // Baud rate divisor; High byte, R/W

// Enhanced register set
// Only accessible when LCR=0xBF
#define  EFR   0x02  // Extra feature register; R/W
#define  XON1  0x04  // R/W
#define  XON2  0x05  // R/W
#define  XOFF1 0x06  // R/W
#define  XOFF2 0x07  // R/W

