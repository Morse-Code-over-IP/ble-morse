/*
  IO-Test for In-Circuit radino modules
  for more information: www.in-circuit.de or www.radino.cc
  
  Every IO-Pin is turned on for N milliseconds in a loop
  D/IO[0..13] N=IO + 10ms   IO5 -> 15ms pulse
  A[0..5]     N=A + 30ms    A3 -> 35ms (Note that the JTAG-EN Fuse has to be reset!)
  USB-Test: connect uart via USB
  HF-Test: UART demo via Bloetooth, use Nordic App "nRF UART" to connect to the device
 */
 
// the setup routine runs once when you press reset:
void setup() {                
  // initialize the digital pins as an outputs
  pinMode(6, OUTPUT);  //radino pin 4 = Arduino D6   
  pinMode(10, OUTPUT); //radino pin 5 = Arduino IO10       
  pinMode(11, OUTPUT); //radino pin 6 = Arduino IO11       
  pinMode(13, OUTPUT); //radino pin 7 = LED = IO13   
  pinMode(5, OUTPUT);  //radino pin 8 = Arduino D5          
  pinMode(A5, OUTPUT); //radino pin 9 = Arduino A5           
  pinMode(0, OUTPUT);  //radino pin 12= Arduino D0/RXD          
  pinMode(1, OUTPUT);  //radino pin 13= Arduino D1/TXD          
  pinMode(A1, OUTPUT); //radino pin 19= Arduino A1
  pinMode(A2, OUTPUT); //radino pin 20= Arduino A2
  pinMode(A0, OUTPUT); //radino pin 21= Arduino A0    
  pinMode(A3, OUTPUT); //radino pin 22= Arduino A3    
  pinMode(2, OUTPUT);  //radino pin 23= Arduino D2/SDA   
  pinMode(3, OUTPUT);  //radino pin 24= Arduino D3/SCL   
  pinMode(12, OUTPUT); //radino pin 25= Arduino IO12    
}

// the loop routine runs over and over again forever:
void loop() {
  digitalWrite(6, HIGH);delay(16);  //D6 -> High pulse 10+6 = 16ms
  digitalWrite(6, LOW);
  
  digitalWrite(10, HIGH);delay(20);
  digitalWrite(10, LOW);
  
  digitalWrite(11, HIGH);delay(21);
  digitalWrite(11, LOW);

  digitalWrite(13, HIGH);delay(23);
  digitalWrite(13, LOW);
  
  digitalWrite(5, HIGH);delay(15);
  digitalWrite(5, LOW);

  digitalWrite(A5, HIGH);delay(35);  //A5 -> High pulse 30+5 = 35ms
  digitalWrite(A5, LOW);

  digitalWrite(0, HIGH);delay(10);
  digitalWrite(0, LOW);

  digitalWrite(1, HIGH);delay(11);
  digitalWrite(1, LOW);

  digitalWrite(A1, HIGH);delay(31);
  digitalWrite(A1, LOW);

  digitalWrite(A2, HIGH);delay(32);
  digitalWrite(A2, LOW);

  digitalWrite(A0, HIGH);delay(30);
  digitalWrite(A0, LOW);

  digitalWrite(A3, HIGH);delay(33);
  digitalWrite(A3, LOW);

  digitalWrite(2, HIGH);delay(12);
  digitalWrite(2, LOW);

  digitalWrite(3, HIGH);delay(13);
  digitalWrite(3, LOW);

  digitalWrite(12, HIGH);delay(22);
  digitalWrite(12, LOW);

  delay(90);
}
