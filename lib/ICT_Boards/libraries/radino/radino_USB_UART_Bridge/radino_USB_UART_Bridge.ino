/*
  USB-UART-Bridge for In-Circuit radino modules
  for more information: www.in-circuit.de or www.radino.cc
  
  This example turns your radino into an USB-UART-Bridge.
  All data send over USB will be passed through to the hardware
  UART/Serial port of your radino.
  
  Connections:
  
  Arduino Pin 0:  RX
  Arduino Pin 1:  TX
  
*/

//Setup routine
void setup() {
  // initialize both serial ports:
  Serial.begin(57600);//USB-UART at baudrate 57600
  
  Serial1.begin(57600);//Hardware-UARTt (Pins RXD1,TXD1) at baudrate 57600
}

void loop() {
  // read from Hardware-UART, send to USB-UART:
  if (Serial1.available()) {
    int inByte = Serial1.read();
    Serial.write(inByte); 
  }
  
  // read from USB-UART, send to Hardware-UART Serial1:
  if (Serial.available()) {
    int inByte = Serial.read();
    Serial1.write(inByte); 
  }
}

