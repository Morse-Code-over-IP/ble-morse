/*
        CC1101 radio-Chip on In-Circuit radino CC1101 modules
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

#include "CC1101_lib.h"

//Registersets for different frequencies
#define CC1101_FREQ_868           1
#define CC1101_FREQ_433           2
#define CC1101_FREQ_433_LOWSPEED  3
#define CC1101_FREQ_868_LOWSPEED  4

//Select from Registersets defined above
//Defaults to 868
#define CC1101_SELECTED_FREQUENCY 4

//Pins
#define SCK_PIN   SCK
#define MISO_PIN  MISO
#define MOSI_PIN  MOSI
#define SS_PIN    8
#define GDO0	  9
#define GDO2	  7


//Sets the registers this library depends on
//These override user defined configs from SetConfigRegisters()
void CC1101::SetMandatoryConfig(void)
{
  halRfWriteReg(IOCFG2,0x07);  //GDO2: Assert when packet with good crc received, deassert on fifo read
  halRfWriteReg(IOCFG0,0x06);  //GDO0: Asserts when sync word has been sent/received, de-asserts at the end of the packet or tx underflow
  halRfWriteReg(PKTLEN,0x3D);  //Maximum packet length is 61 byte so it fits into fifo completely
  halRfWriteReg(PKTCTRL1, (halRfReadReg(PKTCTRL1) | 0x0C));  //append RSSI LQI and CRC OK
  halRfWriteReg(PKTCTRL0, ((halRfReadReg(PKTCTRL0) & ~0x37) | 0x05));  //use FIFOs; CRC Enable; variable length packets
}

/************************** Configuration *********************************************/

static unsigned char patable[8] = {0x60 ,0x60 ,0x60 ,0x60 ,0x60 ,0x60 ,0x60 ,0x60};

//set config registers, copy and paste of RF-Studio generated code
void CC1101::SetConfigRegisters(void) 
{
#if CC1101_SELECTED_FREQUENCY==CC1101_FREQ_868
    // 250kBaud @ 868Mhz
    //
    // Rf settings for CC1101
    //
    halRfWriteReg(IOCFG2,0x05);        //GDO2 Output Pin Configuration
//    halRfWriteReg(IOCFG1,0x2E);        //GDO1 Output Pin Configuration
    halRfWriteReg(IOCFG0,0x06);        //GDO0 Output Pin Configuration
//    halRfWriteReg(FIFOTHR,0x07);       //RX FIFO and TX FIFO Thresholds
//    halRfWriteReg(SYNC1,0xD3);         //Sync Word, High Byte
//    halRfWriteReg(SYNC0,0x91);         //Sync Word, Low Byte
    halRfWriteReg(PKTLEN,0x3D);        //Packet Length
    halRfWriteReg(PKTCTRL1,0x04);      //Packet Automation Control
    halRfWriteReg(PKTCTRL0,0x05);      //Packet Automation Control
    halRfWriteReg(ADDR,0x00);          //Device Address
    halRfWriteReg(CHANNR,0x00);        //Channel Number
    halRfWriteReg(FSCTRL1,0x0C);       //Frequency Synthesizer Control
    halRfWriteReg(FSCTRL0,0x00);       //Frequency Synthesizer Control
    halRfWriteReg(FREQ2,0x21);         //Frequency Control Word, High Byte
    halRfWriteReg(FREQ1,0x62);         //Frequency Control Word, Middle Byte
    halRfWriteReg(FREQ0,0x76);         //Frequency Control Word, Low Byte
    halRfWriteReg(MDMCFG4,0x2D);       //Modem Configuration
    halRfWriteReg(MDMCFG3,0x3B);       //Modem Configuration
    halRfWriteReg(MDMCFG2,0x13);       //Modem Configuration
    halRfWriteReg(MDMCFG1,0x22);       //Modem Configuration
    halRfWriteReg(MDMCFG0,0xF8);       //Modem Configuration
    halRfWriteReg(DEVIATN,0x62);       //Modem Deviation Setting
    halRfWriteReg(MCSM2,0x07);         //Main Radio Control State Machine Configuration
    halRfWriteReg(MCSM1,0x30);         //Main Radio Control State Machine Configuration
    halRfWriteReg(MCSM0,0x18);         //Main Radio Control State Machine Configuration
    halRfWriteReg(FOCCFG,0x1D);        //Frequency Offset Compensation Configuration
    halRfWriteReg(BSCFG,0x1C);         //Bit Synchronization Configuration
    halRfWriteReg(AGCCTRL2,0xC7);      //AGC Control
    halRfWriteReg(AGCCTRL1,0x00);      //AGC Control
    halRfWriteReg(AGCCTRL0,0xB0);      //AGC Control
//    halRfWriteReg(WOREVT1,0x87);       //High Byte Event0 Timeout
//    halRfWriteReg(WOREVT0,0x6B);       //Low Byte Event0 Timeout
    halRfWriteReg(WORCTRL,0xFB);       //Wake On Radio Control
    halRfWriteReg(FREND1,0xB6);        //Front End RX Configuration
//    halRfWriteReg(FREND0,0x10);        //Front End TX Configuration
    halRfWriteReg(FSCAL3,0xEA);        //Frequency Synthesizer Calibration
    halRfWriteReg(FSCAL2,0x2A);        //Frequency Synthesizer Calibration
    halRfWriteReg(FSCAL1,0x00);        //Frequency Synthesizer Calibration
    halRfWriteReg(FSCAL0,0x1F);        //Frequency Synthesizer Calibration
//    halRfWriteReg(RCCTRL1,0x41);       //RC Oscillator Configuration
//    halRfWriteReg(RCCTRL0,0x00);       //RC Oscillator Configuration
//    halRfWriteReg(FSTEST,0x59);        //Frequency Synthesizer Calibration Control
//    halRfWriteReg(PTEST,0x7F);         //Production Test
//    halRfWriteReg(AGCTEST,0x3F);       //AGC Test
    halRfWriteReg(TEST2,0x88);         //Various Test Settings
    halRfWriteReg(TEST1,0x31);         //Various Test Settings
    halRfWriteReg(TEST0,0x09);         //Various Test Settings
//    halRfWriteReg(PARTNUM,0x00);       //Chip ID
//    halRfWriteReg(VERSION,0x04);       //Chip ID
//    halRfWriteReg(FREQEST,0x00);       //Frequency Offset Estimate from Demodulator
//    halRfWriteReg(LQI,0x00);           //Demodulator Estimate for Link Quality
//    halRfWriteReg(RSSI,0x00);          //Received Signal Strength Indication
//    halRfWriteReg(MARCSTATE,0x00);     //Main Radio Control State Machine State
//    halRfWriteReg(WORTIME1,0x00);      //High Byte of WOR Time
//    halRfWriteReg(WORTIME0,0x00);      //Low Byte of WOR Time
//    halRfWriteReg(PKTSTATUS,0x00);     //Current GDOx Status and Packet Status
//    halRfWriteReg(VCO_VC_DAC,0x00);    //Current Setting from PLL Calibration Module
//    halRfWriteReg(TXBYTES,0x00);       //Underflow and Number of Bytes
//    halRfWriteReg(RXBYTES,0x00);       //Overflow and Number of Bytes
//    halRfWriteReg(RCCTRL1_STATUS,0x00);//Last RC Oscillator Calibration Result
//    halRfWriteReg(RCCTRL0_STATUS,0x00);//Last RC Oscillator Calibration Result

#elif CC1101_SELECTED_FREQUENCY==CC1101_FREQ_433
    // 250kBaud @ 433Mhz
    //
    // Rf settings for CC1101
    //
    halRfWriteReg(IOCFG2,0x29);        //GDO2 Output Pin Configuration
//    halRfWriteReg(IOCFG1,0x2E);        //GDO1 Output Pin Configuration
    halRfWriteReg(IOCFG0,0x06);        //GDO0 Output Pin Configuration
//    halRfWriteReg(FIFOTHR,0x07);       //RX FIFO and TX FIFO Thresholds
//    halRfWriteReg(SYNC1,0xD3);         //Sync Word, High Byte
//    halRfWriteReg(SYNC0,0x91);         //Sync Word, Low Byte
    halRfWriteReg(PKTLEN,0x3D);        //Packet Length
    halRfWriteReg(PKTCTRL1,0x04);      //Packet Automation Control
    halRfWriteReg(PKTCTRL0,0x05);      //Packet Automation Control
    halRfWriteReg(ADDR,0x00);          //Device Address
    halRfWriteReg(CHANNR,0x00);        //Channel Number
    halRfWriteReg(FSCTRL1,0x0C);       //Frequency Synthesizer Control
    halRfWriteReg(FSCTRL0,0x00);       //Frequency Synthesizer Control
    halRfWriteReg(FREQ2,0x10);         //Frequency Control Word, High Byte
    halRfWriteReg(FREQ1,0xA7);         //Frequency Control Word, Middle Byte
    halRfWriteReg(FREQ0,0x62);         //Frequency Control Word, Low Byte
    halRfWriteReg(MDMCFG4,0x2D);       //Modem Configuration
    halRfWriteReg(MDMCFG3,0x3B);       //Modem Configuration
    halRfWriteReg(MDMCFG2,0x13);       //Modem Configuration
    halRfWriteReg(MDMCFG1,0x22);       //Modem Configuration
    halRfWriteReg(MDMCFG0,0xF8);       //Modem Configuration
    halRfWriteReg(DEVIATN,0x62);       //Modem Deviation Setting
    halRfWriteReg(MCSM2,0x07);         //Main Radio Control State Machine Configuration
    halRfWriteReg(MCSM1,0x30);         //Main Radio Control State Machine Configuration
    halRfWriteReg(MCSM0,0x18);         //Main Radio Control State Machine Configuration
    halRfWriteReg(FOCCFG,0x1D);        //Frequency Offset Compensation Configuration
    halRfWriteReg(BSCFG,0x1C);         //Bit Synchronization Configuration
    halRfWriteReg(AGCCTRL2,0xC7);      //AGC Control
    halRfWriteReg(AGCCTRL1,0x00);      //AGC Control
    halRfWriteReg(AGCCTRL0,0xB0);      //AGC Control
//    halRfWriteReg(WOREVT1,0x87);       //High Byte Event0 Timeout
//    halRfWriteReg(WOREVT0,0x6B);       //Low Byte Event0 Timeout
    halRfWriteReg(WORCTRL,0xFB);       //Wake On Radio Control
    halRfWriteReg(FREND1,0xB6);        //Front End RX Configuration
//    halRfWriteReg(FREND0,0x10);        //Front End TX Configuration
    halRfWriteReg(FSCAL3,0xEA);        //Frequency Synthesizer Calibration
    halRfWriteReg(FSCAL2,0x2A);        //Frequency Synthesizer Calibration
    halRfWriteReg(FSCAL1,0x00);        //Frequency Synthesizer Calibration
    halRfWriteReg(FSCAL0,0x1F);        //Frequency Synthesizer Calibration
//    halRfWriteReg(RCCTRL1,0x41);       //RC Oscillator Configuration
//    halRfWriteReg(RCCTRL0,0x00);       //RC Oscillator Configuration
//    halRfWriteReg(FSTEST,0x59);        //Frequency Synthesizer Calibration Control
//    halRfWriteReg(PTEST,0x7F);         //Production Test
//    halRfWriteReg(AGCTEST,0x3F);       //AGC Test
    halRfWriteReg(TEST2,0x88);         //Various Test Settings
    halRfWriteReg(TEST1,0x31);         //Various Test Settings
    halRfWriteReg(TEST0,0x09);         //Various Test Settings
//    halRfWriteReg(PARTNUM,0x00);       //Chip ID
//    halRfWriteReg(VERSION,0x04);       //Chip ID
//    halRfWriteReg(FREQEST,0x00);       //Frequency Offset Estimate from Demodulator
//    halRfWriteReg(LQI,0x00);           //Demodulator Estimate for Link Quality
//    halRfWriteReg(RSSI,0x00);          //Received Signal Strength Indication
//    halRfWriteReg(MARCSTATE,0x00);     //Main Radio Control State Machine State
//    halRfWriteReg(WORTIME1,0x00);      //High Byte of WOR Time
//    halRfWriteReg(WORTIME0,0x00);      //Low Byte of WOR Time
//    halRfWriteReg(PKTSTATUS,0x00);     //Current GDOx Status and Packet Status
//    halRfWriteReg(VCO_VC_DAC,0x00);    //Current Setting from PLL Calibration Module
//    halRfWriteReg(TXBYTES,0x00);       //Underflow and Number of Bytes
//    halRfWriteReg(RXBYTES,0x00);       //Overflow and Number of Bytes
//    halRfWriteReg(RCCTRL1_STATUS,0x00);//Last RC Oscillator Calibration Result
//    halRfWriteReg(RCCTRL0_STATUS,0x00);//Last RC Oscillator Calibration Result

#elif CC1101_SELECTED_FREQUENCY==CC1101_FREQ_433_LOWSPEED
  //  @ 433Mhz
  //
  // Rf settings for CC1101
  //
  halRfWriteReg(IOCFG0,0x06);  //GDO0 Output Pin Configuration
  halRfWriteReg(FIFOTHR,0x47); //RX FIFO and TX FIFO Thresholds
  halRfWriteReg(PKTCTRL0,0x05);//Packet Automation Control
  halRfWriteReg(FSCTRL1,0x06); //Frequency Synthesizer Control
  halRfWriteReg(FREQ2,0x10);   //Frequency Control Word, High Byte
  halRfWriteReg(FREQ1,0xA7);   //Frequency Control Word, Middle Byte
  halRfWriteReg(FREQ0,0x62);   //Frequency Control Word, Low Byte
  halRfWriteReg(MDMCFG4,0xCA); //Modem Configuration
  halRfWriteReg(MDMCFG3,0x83); //Modem Configuration
  halRfWriteReg(MDMCFG2,0x13); //Modem Configuration
  halRfWriteReg(DEVIATN,0x35); //Modem Deviation Setting
  halRfWriteReg(MCSM0,0x18);   //Main Radio Control State Machine Configuration
  halRfWriteReg(FOCCFG,0x16);  //Frequency Offset Compensation Configuration
  halRfWriteReg(AGCCTRL2,0x43);//AGC Control
  halRfWriteReg(WORCTRL,0xFB); //Wake On Radio Control
  halRfWriteReg(FSCAL3,0xE9);  //Frequency Synthesizer Calibration
  halRfWriteReg(FSCAL2,0x2A);  //Frequency Synthesizer Calibration
  halRfWriteReg(FSCAL1,0x00);  //Frequency Synthesizer Calibration
  halRfWriteReg(FSCAL0,0x1F);  //Frequency Synthesizer Calibration
  halRfWriteReg(TEST2,0x81);   //Various Test Settings
  halRfWriteReg(TEST1,0x35);   //Various Test Settings
  halRfWriteReg(TEST0,0x09);   //Various Test Settings
  
  // Aditional settings
  halRfWriteReg(IOCFG2,0x29);        //GDO2 Output Pin Configuration
  halRfWriteReg(PKTLEN,0x3D);        //Packet Length
  halRfWriteReg(PKTCTRL1,0x04);      //Packet Automation Control
  halRfWriteReg(PKTCTRL0,0x05);      //Packet Automation Control
  
  halRfWriteReg(FSCTRL0,0x00);       //Frequency Synthesizer Control
  halRfWriteReg(BSCFG,0x1C);         //Bit Synchronization Configuration
  halRfWriteReg(FREND1,0xB6);        //Front End RX Configuration
  
#elif CC1101_SELECTED_FREQUENCY==CC1101_FREQ_868_LOWSPEED
  //
  // Rf settings for CC1101
  //
  halRfWriteReg(IOCFG0,0x06);  //GDO0 Output Pin Configuration
  halRfWriteReg(FIFOTHR,0x47); //RX FIFO and TX FIFO Thresholds
  halRfWriteReg(PKTCTRL0,0x05);//Packet Automation Control
  halRfWriteReg(FSCTRL1,0x06); //Frequency Synthesizer Control
  halRfWriteReg(FREQ2,0x21);   //Frequency Control Word, High Byte
  halRfWriteReg(FREQ1,0x62);   //Frequency Control Word, Middle Byte
  halRfWriteReg(FREQ0,0x76);   //Frequency Control Word, Low Byte
  halRfWriteReg(MDMCFG4,0xCA); //Modem Configuration
  halRfWriteReg(MDMCFG3,0x83); //Modem Configuration
  halRfWriteReg(MDMCFG2,0x13); //Modem Configuration
  halRfWriteReg(DEVIATN,0x35); //Modem Deviation Setting
  halRfWriteReg(MCSM0,0x18);   //Main Radio Control State Machine Configuration
  halRfWriteReg(FOCCFG,0x16);  //Frequency Offset Compensation Configuration
  halRfWriteReg(AGCCTRL2,0x43);//AGC Control
  halRfWriteReg(WORCTRL,0xFB); //Wake On Radio Control
  halRfWriteReg(FSCAL3,0xE9);  //Frequency Synthesizer Calibration
  halRfWriteReg(FSCAL2,0x2A);  //Frequency Synthesizer Calibration
  halRfWriteReg(FSCAL1,0x00);  //Frequency Synthesizer Calibration
  halRfWriteReg(FSCAL0,0x1F);  //Frequency Synthesizer Calibration
  halRfWriteReg(TEST2,0x81);   //Various Test Settings
  halRfWriteReg(TEST1,0x35);   //Various Test Settings
  halRfWriteReg(TEST0,0x09);   //Various Test Settings 
  
  // Aditional settings
  halRfWriteReg(IOCFG2,0x29);        //GDO2 Output Pin Configuration
  halRfWriteReg(PKTLEN,0x3D);        //Packet Length
  halRfWriteReg(PKTCTRL1,0x04);      //Packet Automation Control
  halRfWriteReg(PKTCTRL0,0x05);      //Packet Automation Control
  
  halRfWriteReg(FSCTRL0,0x00);       //Frequency Synthesizer Control
  halRfWriteReg(BSCFG,0x1C);         //Bit Synchronization Configuration
  halRfWriteReg(FREND1,0xB6);        //Front End RX Configuration
  
#else
  //GDO2 7-nc ; 6-Invert? 5:0-Mode (table41 page62)
//  SpiWriteReg(IOCFG2, 0x0B);  //serial clock.synchronous to the data in synchronous serial mode
  halRfWriteReg(IOCFG2, 0x05);  //Assert on TX underflow, deassert on fifo flush
  //GDO1 used as MISO
  //GDO0 7-TempSensor? ; 6-Invert? ; 5:0-Mode (table41 page62)
//  halRfWriteReg(IOCFG0,0x06); //GDO0 Output Pin Configuration
//  SpiWriteReg(IOCFG0, 0x06);  //asserts when sync word has been sent/received, de-asserts at the end of the packet or tx underflow
  halRfWriteReg(IOCFG0, 0x06);
  
  //FIFOTHR
  //SYNC1
  //SYNC0
  
  //PKTLEN
  halRfWriteReg(PKTLEN, 0x3D);  //61 bytes max length
  //PKTCTRL
  halRfWriteReg(PKTCTRL1, 0x04);  //append RSSI LQI and CRC OK, No address check
//  halRfWriteReg(PKTCTRL0,0x05);  //Packet Automation Control
  halRfWriteReg(PKTCTRL0, 0x05);  //whitening off; use FIFOs; CRC Enable; variable length packets
  //ADDR
  halRfWriteReg(ADDR, 0x00);  //address used for packet filtration.
  //CHANNR
  halRfWriteReg(CHANNR, 0x00);  //Channel number (uses cahnnel spacing)
  
  //FSCTRL
  halRfWriteReg(FSCTRL1,0x0C); //Frequency Synthesizer Control
  
  //FREQ
  halRfWriteReg(FREQ2,0x21);   //Frequency Control Word, High Byte
  halRfWriteReg(FREQ1,0x62);   //Frequency Control Word, Middle Byte
  halRfWriteReg(FREQ0,0x76);   //Frequency Control Word, Low Byte
  
  //MDMCFG 4-0
  halRfWriteReg(MDMCFG4,0x2D); //Modem Configuration
  halRfWriteReg(MDMCFG3,0x3B); //Modem Configuration
  halRfWriteReg(MDMCFG2,0x13); //Modem Configuration

  //DEVIATN
  halRfWriteReg(DEVIATN,0x62); //Modem Deviation Setting
  //MCSM 2-0
  halRfWriteReg(MCSM0,0x18);   //Main Radio Control State Machine Configuration
  
  //FOCCFG
  halRfWriteReg(FOCCFG,0x1D);  //Frequency Offset Compensation Configuration
  //BSCFG
  halRfWriteReg(BSCFG,0x1C);   //Bit Synchronization Configuration
  //AGCCTRL 2-0
  halRfWriteReg(AGCCTRL2,0xC7);//AGC Control
  halRfWriteReg(AGCCTRL1,0x00);//AGC Control
  halRfWriteReg(AGCCTRL0,0xB0);//AGC Control
  //WOREVT 1-0
  //WORCTRL
  halRfWriteReg(WORCTRL,0xFB); //Wake On Radio Control
  //FREND 1-0
  halRfWriteReg(FREND1,0xB6);  //Front End RX Configuration
  //FSCAL 3-0
  halRfWriteReg(FSCAL3,0xEA);  //Frequency Synthesizer Calibration
  halRfWriteReg(FSCAL2,0x2A);  //Frequency Synthesizer Calibration
  halRfWriteReg(FSCAL1,0x00);  //Frequency Synthesizer Calibration
  halRfWriteReg(FSCAL0,0x1F);  //Frequency Synthesizer Calibration
  
  //RCCTRL 1-0
  
  //FSTEST - for test only???
//  SpiWriteReg(CC1101_FSTEST, 0x59);
  //PTEST
  //AGCTEST - for test only
  
  //TEST 2-0
  halRfWriteReg(TEST2, 0x81);
  halRfWriteReg(TEST1, 0x35);
  halRfWriteReg(TEST0,0x09);  //Various Test Settings
  halRfWriteReg(TEST0, 0x09);
#endif
}

/************************** Configuration end *****************************************/

void CC1101::Init(void)
{
  unsigned char mode=0;
  m_state = CCIDLE;
  
  //SPI
  pinMode(MISO_PIN, INPUT);
  pinMode(MOSI_PIN, OUTPUT);
  digitalWrite(MOSI_PIN, LOW);
  pinMode(SCK_PIN, OUTPUT);
  digitalWrite(SCK_PIN, HIGH);
  pinMode(SS_PIN, OUTPUT);
  digitalWrite(SS_PIN, HIGH);
  
  // enable SPI Master, MSB, SPI mode 0, FOSC/4
  SPCR = 0;
  SPCR = (mode & 0x7F) | (1<<SPE) | (1<<MSTR);
  
  //GDO Pins
  pinMode(GDO0, INPUT);
  pinMode(GDO2, INPUT);
  
  Reset();
  SetConfigRegisters();
  SetMandatoryConfig();
  halRfWriteBurstReg(PATABLE,patable,8);
  delay(1);  //let it set
}

//Reset routine
void CC1101::Reset (void)
{
  digitalWrite(SS_PIN, LOW);
  delay(1);
  digitalWrite(SS_PIN, HIGH);
  delay(1);
  digitalWrite(SS_PIN, LOW);
  while(digitalRead(MISO_PIN)) {};
  SpiTransfer(SRES);
  while(digitalRead(MISO_PIN)) {};
  digitalWrite(SS_PIN, HIGH);
}

unsigned char CC1101::ReadState(void)
{
  return halRfReadStatus(MARCSTATE)&0x1F;
}

static unsigned long rx_timeout = 0;
CC_STATE CC1101::GetState(void)
{
  switch (m_state)
  {
    default:
    case CCIDLE:
    case CCGOTPACKET:
      break;
      
    case CCSENDING:
      if (!digitalRead(GDO0))
      {
        halRfStrobe(SFTX);  //Flush FIFIO
        m_state = CCIDLE;
      }
      break;
      
    case CCWAITINGRX:
      if (digitalRead(GDO0))
      {
        m_state = CCRECEIVING;
      }
    case CCRECEIVING:
      if (digitalRead(GDO2))
      {
        m_state = CCGOTPACKET;
      }
      else if(rx_timeout<millis())
      {
        halRfStrobe(SIDLE);
        m_state = CCIDLE;
        halRfStrobe(SFRX);
      }
      break;
  }
  return m_state;
}

//Start/Stop receiving
unsigned char CC1101::StartReceive(unsigned long timeout_ms)
{
  if (GetState() != CCIDLE)
    return 0;
  if (timeout_ms)
    rx_timeout = millis()+timeout_ms;
  else
    rx_timeout = 0-1;
  m_state = CCWAITINGRX;
  halRfStrobe(SRX);
  return 1;
}
unsigned char CC1101::StopReceive(void)
{
  if (GetState() == CCWAITINGRX)
  {
    halRfStrobe(SIDLE);
    m_state = CCIDLE;
  }
  else if (GetState() == CCRECEIVING)
    rx_timeout = millis()+5;
  else if (GetState() != CCIDLE)
    return 0;
  return 1;
}

//start packet transmission
unsigned char CC1101::Transmit(unsigned char *txBuf, unsigned char cnt)
{
  unsigned long i;
  if (GetState() != CCIDLE)
    return 0;
  halRfWriteReg(TXFIFO,cnt);  //write len
  halRfWriteBurstReg(TXFIFO,txBuf,cnt);  //fill FIFO
  halRfStrobe(STX);  //do send
  
  i = millis()+10;
  while (millis()<i && !digitalRead(GDO0)) {};  //wait for transfer start
  if (i)
  {
    m_state = CCSENDING;
    return 1;
  }
  halRfStrobe(SIDLE);
  m_state = CCIDLE;
  halRfStrobe(SFTX);
  return 0;
}

//get RXFIFO content if any
unsigned char CC1101::GetData(unsigned char *rxBuf)
{
  unsigned char cnt;
  if (GetState() != CCGOTPACKET) return 0;
  m_state = CCIDLE;
  cnt = halRfReadStatus(RXBYTES);
  if (cnt & 0x80)  // Overflow
  {
    halRfStrobe(SFRX);
  }
  else if (cnt)
  {
    halRfReadBurstReg(RXFIFO,rxBuf,cnt);
    halRfStrobe(SFRX);
    return cnt;
  }
  return 0;
}

/********************************* Internal Functions  ***********************************/

//Write and read a byte
uint8_t CC1101::SpiTransfer(unsigned char wr)
{
  SPDR = wr;
  while (!(SPSR & (1<<SPIF)));
  return SPDR;
}

//write single byte register
void CC1101::halRfWriteReg(unsigned char addr, unsigned char wr)
{
  digitalWrite(SS_PIN, LOW);
  while(digitalRead(MISO_PIN)) {};
  SpiTransfer(addr);
  SpiTransfer(wr);
  digitalWrite(SS_PIN, HIGH);
}

//write burst register
void CC1101::halRfWriteBurstReg(unsigned char addr, unsigned char *buf, unsigned char cnt)
{
  unsigned char i;
  digitalWrite(SS_PIN, LOW);
  while(digitalRead(MISO_PIN)) {};
  SpiTransfer(addr|WRITE_BURST);
  for (i=0 ; i<cnt ; i++)
  {
    SpiTransfer(buf[i]);
  }
  digitalWrite(SS_PIN, HIGH);
}

//write strobe cmd
void CC1101::halRfStrobe(unsigned char strobe)
{
  digitalWrite(SS_PIN, LOW);
  while(digitalRead(MISO_PIN)) {};
  SpiTransfer(strobe);
  digitalWrite(SS_PIN, HIGH);
}

//read single byte register
unsigned char CC1101::halRfReadReg(unsigned char addr) 
{
  unsigned char ret;
  digitalWrite(SS_PIN, LOW);
  while(digitalRead(MISO_PIN)) {};
  SpiTransfer(addr|READ_SINGLE);
  ret=SpiTransfer(0);
  digitalWrite(SS_PIN, HIGH);
  return ret;
}

//read burst register into buffer
void CC1101::halRfReadBurstReg(unsigned char addr, unsigned char *buf, unsigned char cnt)
{
  unsigned char i;
  digitalWrite(SS_PIN, LOW);
  while(digitalRead(MISO_PIN)) {};
  SpiTransfer(addr|READ_BURST);
  for(i=0;i<cnt;i++)
  {
    buf[i]=SpiTransfer(0);
  }
  digitalWrite(SS_PIN, HIGH);
}

//get status register
unsigned char CC1101::halRfReadStatus(unsigned char addr)
{
  unsigned char ret;
  digitalWrite(SS_PIN, LOW);
  while(digitalRead(MISO_PIN)) {};
  SpiTransfer(addr|READ_BURST);
  ret=SpiTransfer(0);
  digitalWrite(SS_PIN, HIGH);
  return ret;
}

