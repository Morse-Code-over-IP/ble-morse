/*
        radino CC1101 UART-Bridge
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

#include <SoftwareSerial.h>
#include <avr/wdt.h>
#include "CC1101_lib.h"

/******************************** Configuration *************************************/

#define NWID 0x01  //My networks ID
#define MID  0x00  //My ID
#define TID  0x00  //Default target ID
#define HOPS 0x01  //Max hops - currently unused

#define RECEIVE_TO 1000  //Wait at max this long in ms for packet arrival

#define DINRAILADAPTER 1  //Is a DinRailAdapter (Bridge SoftwareSerial on pins 10,11,12)
#define RADINOSPIDER   0  //Is a radino Spider (Bridge Serial1)

#define UART_BUFFSIZE 725  //Buffer for UART
#define UART_SENDTHRES 710  //Start sending when this full
#define RADIO_BUFFSIZE 725  //Buffer for radio
#define RADIO_SENDTHRES 710  //Start writing when this full

#define UART_TIMEOUT 10  //UART is considered empty after this time in ms
#define RESEND_TIMEOUT 75  //Wait this time in ms for an radio ack
#define PACKET_GAP 10 //Delay between receive and outgoing ack
#define MAXRETRIES 2  //Try so much retransmissions

//Used for error signaling (ON when restransmitting, OFF on receive success)
#define ERRLEDON() digitalWrite(13,HIGH)
#define ERRLEDOFF() digitalWrite(13,LOW)
#define ERRLEDINIT() pinMode(13, OUTPUT)

//Write messages on dbgSerial
//0:none
//1:Errors
//2:Information
#define DBGLVL 0

/***************************** Debug messages ***********************/

//Write out errors on dbgSerial
#if DBGLVL>=1
  #define DBGERR(x) dbgSerial.print(x)
  #define DBGERR2(x,y) dbgSerial.print(x,y)
  #define DBGERRWRITE(x) dbgSerial.write(x)
#else
  #define DBGERR(x) do{}while(0)
  #define DBGERR2(x,y) do{}while(0)
  #define DBGERRWRITE(x) do{}while(0)
#endif

//Write out information on dbgSerial
#if DBGLVL>=2
  #define DBGINFO(x) dbgSerial.print(x)
  #define DBGINFO2(x,y) dbgSerial.print(x,y)
  #define DBGWRITE(x) dbgSerial.write(x)
#else
  #define DBGINFO(x) do{}while(0)
  #define DBGINFO2(x,y) do{}while(0)
  #define DBGWRITE(x) do{}while(0)
#endif

/******************************* Module specific settings **********************/
#undef TXEN_PIN

#if DINRAILADAPTER  //DinRailAdapter
  #define dbgSerial Serial
  #define dbgSerialSpeed 57600
  
  #define RX_PIN 10
  #define TX_PIN 11
  SoftwareSerial bridge = SoftwareSerial(RX_PIN,TX_PIN);
  #define bridgeSpeed 9600
  #define bridgeBurst 10  //Chars written out per call
  #define bridgeDelay 0 //SofwareSerial writes blocking - no wait necessary
  #define TXEN_PIN 12  //Which pin to use for RS485 functionality - comment to disable
  
#elif RADINOSPIDER  //radino Spider
  #define dbgSerial Serial
  #define dbgSerialSpeed 57600
  
  #define bridge Serial1
  #define bridgeSpeed 9600
  #define bridgeBurst 10  //Chars written out per call
  #define bridgeDelay ((bridgeBurst*1000)/(bridgeSpeed/8))  //Time between calls - Let hardware serial write out async
  #define TXEN_PIN 2  //Which pin to use for RS485 functionality - comment to disable
  
#endif  //Module selection

//Utilize RS485 functionality?
#ifdef TXEN_PIN
  #define TXEN_ON() digitalWrite(TXEN_PIN, HIGH)
  #define TXEN_OFF() digitalWrite(TXEN_PIN, LOW)
  #define TXEN_INIT() pinMode(TXEN_PIN, OUTPUT)
#else
  #define TXEN_ON() do{}while(0)
  #define TXEN_OFF() do{}while(0)
  #define TXEN_INIT() do{}while(0)
#endif

/****************** Packet structures *******************/
//Header for our data packets
typedef struct
{
  uint8_t nwid;
  uint8_t src;
  uint8_t dest;
  uint8_t seq;
  uint8_t pseq;
  uint8_t hopc;
  uint8_t len;
  uint8_t crc;
} header_t;

#define INDEX_NWID 0  //Network ID
#define INDEX_SRC  1  //Source ID
#define INDEX_DEST 2  //Target ID
#define INDEX_SEQ  3  //Senders current sequence number
#define INDEX_PSEQ 4  //Last sequence number heard from target
#define INDEX_HOPC 5  //Hopcount
#define INDEX_SIZE 6  //Size of Packet
#define INDEX_CRC  7  //CRC
#define HEADERSIZE 8
// CONTENT

//Buff for radio packet handling
#define PAKETSIZE 61  //CC1101 adds LEN, LQI, RSSI -- stay under fifo size of 64 byte (CC1101 buggy)
#define MAXDATALEN PAKETSIZE-HEADERSIZE

//Structure of our data packets
typedef struct
{
  header_t header;
  uint8_t data[MAXDATALEN];
} packet_t;

//Packet format delivered by the CC1101 RX
typedef struct
{
  uint8_t len;
  packet_t packet;
  uint16_t appended;
} transfer_t;

/****************** Vars *******************************/
transfer_t RX_buffer = {0,};
packet_t TX_packet = {0,};

char uartBuf[UART_BUFFSIZE] = {0,};
unsigned short uartBufLen = 0;
uint32_t uart_timeout = 0;

char radioBuf[RADIO_BUFFSIZE] = {0,};
unsigned short radioBufLen = 0;
unsigned short radio_writeout = 0xFFFF;
unsigned long radioOut_delay = 0;

unsigned long resend_to = 0;

CC1101 cc1101;  //The CC1101 device

/************************* Functions **********************/

//Initialize the system
void setup()
{
  wdt_enable(WDTO_8S);  //Watchdog 8s
  dbgSerial.begin(dbgSerialSpeed);
  dbgSerial.print("\r\n\r\nHello\r\n\r\n");
  
  bridge.begin(bridgeSpeed);
  TXEN_INIT();
  
  ERRLEDINIT(); ERRLEDOFF();
  
  cc1101.Init();
  cc1101.StartReceive(RECEIVE_TO);
}

//Main loop is called over and over again
void loop()
{
  static uint8_t seqnr=0;  //next seqnr
  static uint8_t lastseqnr=0;  //last sent seqnr
  static uint8_t partnerseqnr=0;  //received seqnr
  static uint8_t lastseqnrs[256]={0,};  //table of last acked seqnr
#define lastpartnerseqnr lastseqnrs[TID]
  static uint8_t retrycnt=0;  //counter for retries
  
  unsigned char crc, cnt;
  unsigned short i;
  float rssi;
  packet_t * pPacket;
  header_t * pHeader;
  
  //Check for freezes
  wdt_reset();  //At least arduino is running
  cc1101.StartReceive(RECEIVE_TO);
  
/************** radio to UART ************************/
  
  //No chars received since UARTTIMEOUT ms AND have chars in outputbuffer
  //writes bridgeBurst chars every bridgeDelay ms (Let the hardware serial write out async)
  if (uart_timeout<millis() && radio_writeout<radioBufLen && radioOut_delay<millis())
  {
    TXEN_ON();
    radioOut_delay = millis()+bridgeDelay;
    for ( i=0 ; i<bridgeBurst && radio_writeout<(radioBufLen-1) ; radio_writeout++, i++ )
    {
      bridge.write(radioBuf[radio_writeout]);
    }
    if (radio_writeout==(radioBufLen-1))  //This is the last char
    {
      bridge.write(radioBuf[radioBufLen-1]);
      radio_writeout = 0xFFFF;  //signal write complete to radio reception
      radioBufLen = 0;
      bridge.flush();
      TXEN_OFF();
    }
  }
  
  //CC1101 finished reception of a packet
  if (cc1101.GetState() == CCGOTPACKET)
  {
    cnt=cc1101.GetData((uint8_t*)&RX_buffer);  //read the RX FiFo (returns number of read chars)
    
    if(cnt>0)
    {
      DBGINFO("Receive(");
      DBGINFO(cnt);
      DBGINFO("): ");
      for (i=0;i<cnt && i<sizeof(header_t)+1;i++)
      {
        DBGINFO2(((uint8_t*)&RX_buffer)[i],HEX);
        DBGWRITE(' ');
      }
      DBGINFO("-> ");
      
      //Is the packet length valid?
      if((RX_buffer.len>=sizeof(header_t)) && (RX_buffer.len<=sizeof(packet_t)))
      {
        pPacket = &RX_buffer.packet;
        pHeader = &pPacket->header;
        
        //our network? our ID addressed?
        if((pHeader->nwid==NWID) && (pHeader->dest==MID))
        {
          //Todo actually implement crc check
          cnt = pHeader->crc;
          pHeader->crc = 0;
          crc=42;
          for(i=0 ; i<RX_buffer.len ; i++) crc+=((uint8_t*)pPacket)[i];
          
          //valid packet crc
          if (crc==cnt)
          {
            resend_to = millis()+PACKET_GAP;  //short delay on answers
            partnerseqnr = pHeader->seq;  //remember their sequence number
            if (lastseqnr+1 == pHeader->pseq) ERRLEDOFF();  //got ack for our last data
            lastseqnr = pHeader->pseq;  //remember last acked sequence number
            
            //Calculate RSSI
            crc = pPacket->data[pHeader->len];
            rssi = crc;
            if (crc&0x80) rssi -= 256;
            rssi /= 2;
            rssi -= 74;
            
            DBGINFO(" RSSI: ");
            DBGINFO(rssi);
            DBGINFO("dBm");
            
            //did they alter their sequence number? implies they got an ack
            if (partnerseqnr!=lastpartnerseqnr)
            {
              for(i=0; i<pHeader->len && radioBufLen<RADIO_BUFFSIZE ;i++)  //fill uart buffer
              {
                radioBuf[radioBufLen++] = pPacket->data[i];
                if (radio_writeout == 0xFFFF) radio_writeout = 0;  //signal the uart handler to start writing out
              }
            }
            else if (pHeader->len && partnerseqnr==lastpartnerseqnr)  //retransmission -> retransmit ack
            {
              lastpartnerseqnr--;
            }
            
            DBGINFO("\r\n");
          } else { //end if CRC ok
            DBGERR("!CRC\r\n");
          }
        } else { //end if NWID && TID ok
          DBGERR("!DEST-ID\r\n");
        }
      } else { //end if size valid
        DBGERR("!SIZE\r\n");
      }
    }
  }
  
/************** UART to radio ************************/
  
  //read incoming chars from UART
  crc = bridge.available();
  while(uartBufLen<UART_BUFFSIZE && crc)  //still have bufferspace and remaining chars on uart
  {
    cnt = bridge.read();
    crc--;
    //last char within timeout OR fresh block
    if (uart_timeout>millis() || uartBufLen==0)
    {
      uartBuf[uartBufLen++] = cnt;
      uart_timeout = millis()+UART_TIMEOUT;
    }
    if (!crc) crc = bridge.available();
  }
  
  //Buffer has content AND since timeout no chars || Buffer over filllevel || lastseqnr waiting ack || partner send new seqnr
  if (resend_to<millis() && ((uartBufLen>0 && uart_timeout<millis()) || uartBufLen>=UART_SENDTHRES || seqnr!=lastseqnr || partnerseqnr!=lastpartnerseqnr))
  {
    DBGINFO2(millis()&0xFF,HEX);
    DBGINFO(" ");
    //Init packet
    pPacket = &TX_packet;
    pHeader = &pPacket->header;
    pHeader->nwid = NWID;
    pHeader->src = MID;
    pHeader->dest = TID;
    pHeader->pseq = partnerseqnr;
    pHeader->hopc = HOPS;
    
    //Still no ack for last sent sequence number -> resend
    if (seqnr!=lastseqnr)
    {
      //retried too often
      if (retrycnt>=MAXRETRIES)
      {
        ERRLEDON();
        lastseqnr = seqnr;  //start next transmission with a different sequence number
        pHeader->seq = seqnr;
        retrycnt = 0;  //reset retries
        uartBufLen = 0;  //kill buffer
        DBGERR("reached max Retries\r\n");
        cc1101.StartReceive(RECEIVE_TO);
        return;
      } else {
        ERRLEDON();
        lastpartnerseqnr = partnerseqnr;  //update ack signal
        DBGINFO("Retry #");
        DBGINFO(retrycnt++);
        DBGINFO(" (");
      }
    }
    
    //Next data chunk [Last packet was acked AND ( (we have data AND uart timed out) OR uart beyond threshold) ]
    else if (seqnr==lastseqnr && ((uartBufLen>0 && uart_timeout<millis()) || uartBufLen>UART_SENDTHRES))
    {
      lastpartnerseqnr = partnerseqnr;  //update ack signal
      
      //Refill packet buffer with new data
      seqnr++;  //new data -> increase sequence number
      pHeader->seq = seqnr;
      pHeader->len = uartBufLen<MAXDATALEN ? uartBufLen : MAXDATALEN;  //length
      for ( i=0 ; i<pHeader->len ; i++ )
      {
        pPacket->data[i] = uartBuf[i];
        uartBuf[i] = uartBuf[pHeader->len+i];
      }
      //shift buffer
      uartBufLen -= i;
      for ( ; i<uartBufLen ; i++ )
      {
        uartBuf[i] = uartBuf[pHeader->len+i];
      }
      
      DBGINFO("Sending (");
      retrycnt = 0;
    }
    
    //Last packet was acked and no new data to send -> send only ack
    else if (seqnr==lastseqnr)
    {
      lastpartnerseqnr = partnerseqnr;
      pHeader->len = 0;
      DBGINFO("SendAck (");
    }
    
    //Should not get here... but you never know
    else
    {
      DBGERR("unexpected send\r\n");
      cc1101.StartReceive(RECEIVE_TO);
      return;
    }
    
    //TODO calculate real crc
    pHeader->crc=0;
    crc=42;
    for( i=0 ; i<(sizeof(header_t)+pHeader->len) ; i++)
    {
      crc+=((uint8_t*)&TX_packet)[i];
    }
    pHeader->crc=crc;
    
    DBGINFO(i);
    DBGINFO(",");
    DBGINFO(pHeader->len);
    DBGINFO(")\r\n");
    
    //Leave receive mode
    if (cc1101.StopReceive())
    {
      if (cc1101.Transmit((uint8_t*)&TX_packet,i))
      {
        resend_to = millis()+RESEND_TIMEOUT;
      } else {
        //Todo? Transmit error... ignore -> counts as failed retry
        resend_to = millis()+RESEND_TIMEOUT;
      }
    }
    else
    {
      DBGERR("ERROR Stop Receive\r\n");
    }
  }
}

