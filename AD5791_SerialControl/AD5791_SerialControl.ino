/*
AD5791_SerialControl - Library of SPI Functions to communicate with AD5791
====================
Utilises Serial Input to enable LabVIEW control of connected DAC devices

Command Structure:
------------------
Capitals are set commands, lower case are read commands

Send over serial ABn{d}

This translates as 
  A:Read/Set command character
  B:Secondary command character
  n:DAC Number (3=All)
  {d}: Auxiliary data (such as output register value)

Read/Set command characters:
E:Echo to serial port
S:Set register values (I,R,U,X,O,C,S)
R:Read register values (C,O)

Secondary command characters:
I:Initialise (Set uint8_t LinComp Mode 0-4)
X:Clear DAC (Set Software Register)
U:Update (Set Software Register)
R:Reset (Set Software Register)
C:CLR Register (Set/Get d=uint32_t)
O:DAC Output Register (Set/Get d=uint32_t)
S:Synchronous update of DAC Output Registers (Set d=uint32_t x3)


Created by J. Pritchard, jdpritchard@wisc.edu 5/16/2014
UW-Madison,Physics,SaffmanLab
 */

 
 
#include<SPI.h>
#include<AD5791.h>

// size of values in bytes for serial comm
#define LONG_SIZE 4
#define INT_SIZE 2
#define CHAR_SIZE 1

// serial comm packets
#define NACK 0x15
#define NL 0x0A
// maximum response length is 1 cmd + 1 field + 1 addr + 4 value + 1 null = 9
#define ECHO_BUFF_SIZE 16
// Serial controller timeout
#define TIMEOUT 100

//Serial Data Structure  
typedef union serialData {
  uint8_t barray[sizeof(uint32_t)];
  char carray[sizeof(uint32_t)];
  uint32_t lvalue;
} SERIALDATA;

//DAC Device Addresses
const int N_DAC = 3;
int DAC[] = {46,47,48};

//LDAC Pin (Used for Sync)
const int LDAC=53;

//Loop Variable
int i;

void setup(){
  
  //Initialise SPI
  SPI.begin();
  SPI.setBitOrder(MSBFIRST); //AD5791 Needs MSB First
  SPI.setClockDivider(SPI_CLOCK_DIV128); //Slow for scoping
  //Falling/rising edge stuff
  //MODE0:CPOL=0,CPHA=0 [CLK Low,Data Rising Edge]
  //MODE1:CPOL=0,CPHA=1 [CLK Low,Data Falling Edge] ***
  //MODE2:CPOL=1,CPHA=0 [CLK High,Data Rising Edge]
  //MODE3:CPOL=1,CPHA=1 [CLK High,Data Falling Edge]
  SPI.setDataMode(SPI_MODE1);
 
  //Initialise   Serial Port
  Serial.begin(9600);
  
  //Setup LDAC Pin
   pinMode(LDAC,OUTPUT);
  digitalWrite(LDAC,LOW);
  //Configure SS Pins, Setup DAC
  for(i=0;i<N_DAC;i=i+1){
    pinMode(DAC[i],OUTPUT);
    digitalWrite(DAC[i],HIGH);
    setDACconfig(DAC[i],0);  
  }  
 
   
}

//================================================================================
//main program loop
void loop() {
}


//================================================================================
// on serial event from usb channel parse the command according to the spreadsheet
void serialEvent() {
  char cmdChar = (char)Serial.read();
  switch (cmdChar) {
  // simple echo command
  case 'e':
  case 'E':
    EchoPrompt(Serial);
    break;   
  // set variable
  case 's':
  case 'S':
    SetPrompt(Serial);
    break;
  // return variable
  case 'r':
  case 'R':
    ReturnPrompt(Serial);
    break;
  // command not recognized
  default:
    UnknownPrompt(Serial);
  }
}

//================================================================================
// Echo prompt from controller
// returns 
int8_t EchoPrompt(HardwareSerial & ser) {
  char inChar ;
  String inputStr = "E";
  uint8_t bytecnt = 0;
  // wait until buffer has more data
  if ( WaitForData(ser,sizeof(uint8_t)) != 0 ) {
    return -1;
  }
  // read from buffer until newline or overflow
  while (ser.available() > 0) {
    inChar = (char)ser.read();
    inputStr += inChar;
    // done when newline is recieved
    if (inChar == NL) {
      ser.print(inputStr);  // appended with newline char
      return 0;
    }
    // didnt reieve newline in time
    if (bytecnt >= LONG_SIZE) {
      UnknownPrompt(ser);
      return -1;
    }
    bytecnt++;
  }
  return -1;
}

//================================================================================
// Set Function Wrappers

//Wrapper to InitialiseDAC
int8_t InitialiseDAC(int8_t dac,uint32_t lincomp){
  setDACconfig(dac,(int) lincomp);
  return 0;
}

//Wrapper to ResetDAC
int8_t Reset(int8_t dac,uint32_t obsolete){
  resetDAC(dac);
  return 0;
}

//Wrapper to UpdateDAC
int8_t Update(int8_t dac,uint32_t obsolete){
  updateDAC(dac);
  return 0;
}

//Wrapper to ClearDAC
int8_t Clear(int8_t dac,uint32_t obsolete){
  clearDAC(dac);
  return 0;
}

//Wrapper to SetDAC
int8_t SetOutput(int8_t dac,uint32_t value){
  setDAC(dac,value);
  return 0;
}

//Wrapper to SetCLR
int8_t SetClear(int8_t dac,uint32_t value){
  setCLR(dac,value);
  return 0;
}

//================================================================================
// Set prompt from controller
int8_t SetPrompt(HardwareSerial & ser) {
  int dac = 0; // DAC Device
  uint32_t field = 0;// set value
  uint8_t f_size;// size of field in bytes
  // pointer to set function
  int8_t (*setValue)(int8_t, uint32_t);
  // input string
  uint8_t inputStr[ECHO_BUFF_SIZE] = {(uint8_t)'S'};
  uint8_t index = 1;
  //SerialData Structure
  SERIALDATA data;
  
  // wait for more data in buffer
  if ( WaitForData(ser,sizeof(uint8_t)) ) {
    return -1;
  }
 
   // Read Command Character
  char fChar = (char)ser.read();
  index = appendByte(inputStr,(uint8_t)fChar,index);
  
  //Read DAC Address
  if (SerialReadField(ser,CHAR_SIZE,dac) != 0) {
      return -1;
    }
  else if(dac<0 || dac>N_DAC) { //Ensure valid DAC address
    return -1;
  }
  
  index = appendByte(inputStr,dac,index);  
  
  switch (fChar) {
  // Initialise DAC
  case 'i':
  case 'I':
    f_size = CHAR_SIZE;
    setValue = &InitialiseDAC;
    break;
  //Reset DAC
  case 'r':
  case 'R':
    f_size = CHAR_SIZE;
    setValue = &Reset;
    break;
  //Update DAC
  case 'u':
  case 'U':
    f_size = CHAR_SIZE;
    setValue = &Update;
    break;
  //Clear DAC
  case 'x':
  case 'X':
    f_size = CHAR_SIZE;
    setValue = &Clear;
    break;   
  //DAC Output Register
  case 'o':
  case 'O':
    f_size = LONG_SIZE;
    setValue = &SetOutput;
    break;
  //ClR Output Register
  case 'c':
  case 'C':
    f_size = LONG_SIZE;
    setValue = &SetClear;
    break;
  //Synchronous Update via LDAC:Reads each DAC Value and updates synchronously
  case 's':
  case 'S':
    f_size = LONG_SIZE;
    //Set LDAC High
    digitalWrite(LDAC,HIGH);
    for(i=0;i<N_DAC;i=i+1){
      //Read Field
      if (SerialReadField(ser,f_size,field) != 0) {
          return -1;
      }
      //Update DAC
      SetOutput(DAC[i], field);
      //Add to serial return
      data.lvalue = 0;
      data.lvalue = field;
      for ( uint8_t j = 0; j < f_size; j++ ) {
        index = appendByte(inputStr,data.barray[f_size-j-1],index);
      }
      //index = appendByte(inputStr,(uint8_t)'\n',index);                
    }
    //Sync Output Updates
    digitalWrite(LDAC,LOW);
    // return input string to acknowledge reciept
    for ( i = 0; i < index; i++ ) {
      ser.print((char)inputStr[i]);
    }
    return 0;
  default:
    UnknownPrompt(ser);
    return -1;
  }
  
  // Read field
  if (SerialReadField(ser,f_size,field) != 0) {
    return -1;
  }
  // error if next byte is not newline char
  if ( WaitForData(ser,sizeof(uint8_t)) ) {
    return -1;
  }
  if (ser.read() != NL) {
    return -1;
  } 
  
  //Single or Multiple DAC Update
  if(dac == N_DAC) {
      for(i=0;i<N_DAC;i=i+1){
        setValue(DAC[i], field);     
      }     
  }
  else {
        setValue(DAC[dac], field); 
  }  
       
  // create remainder of input string for echo from field value
  data.lvalue = 0;
  data.lvalue = field;
  for ( uint8_t i = 0; i < f_size; i++ ) {
    index = appendByte(inputStr,data.barray[f_size-i-1],index);
  }
  index = appendByte(inputStr,(uint8_t)'\n',index);
  // return input string to acknowledge reciept
  for ( i = 0; i < index; i++ ) {
    ser.print((char)inputStr[i]);
  }
  return 0;
}

//================================================================================
// Return prompt from controller
int8_t ReturnPrompt(HardwareSerial & ser) {
  int dac = 0; // DAC address
  // pointer to get function
  int8_t (*getValue)(int, uint32_t&);
  // input string
  String inputStr = "R";
  // wait for more data in buffer (probably overkill)
  if ( WaitForData(ser,sizeof(uint8_t)) ) {
    return -1;
  }
  //Read Command Character
  char fChar = (char)ser.read();
  inputStr += fChar;
  
  //Read DAC Address
  if (SerialReadField(ser,CHAR_SIZE,dac) != 0) {
      return -1;
    }
  else if(dac<0 || dac>N_DAC) { //Ensure valid DAC address
    return -1;
  }  
  inputStr += (char) dac;
  
  switch (fChar) {
  //Get DAC Register Value
  case 'O':
  case 'o':
    getValue = &GetOutput;
    break;
  //Get CLR Register Value
  case 'C':
  case 'c':
    getValue = &GetClear;
    break;
  default:
    UnknownPrompt(ser);
    return -1;
  }
  // next char must be newline term
  if ( WaitForData(ser,sizeof(uint8_t)) ) {
    return -1;
  }
  if (ser.read() != NL) {
    UnknownPrompt(ser);
    return -1;
  }
  
  // using a union to send a byte at time during the write
  SERIALDATA data;
  data.lvalue = 0;
  
  //Single or Multiple DAC Update
  if(dac == N_DAC) {
      for(i=0;i<N_DAC;i=i+1){
        // get field
        if (getValue(DAC[i], data.lvalue) != 0) {
          ser.print(NACK);
          ser.print("\n");
          return -1;
        }
        //Send to Serial Port
        for ( uint8_t j=0; j<sizeof(uint32_t); j++) {
          ser.print(data.carray[sizeof(uint32_t)-j-1]);
        }
      }
      ser.print("\n");
      return 0;
  }
  else {
    // get field
    if (getValue(DAC[dac], data.lvalue) != 0) {
      ser.print(NACK);
      ser.print("\n");
      return -1;
    } 
    for ( uint8_t i=0; i<sizeof(uint32_t); i++) {
      ser.print(data.carray[sizeof(uint32_t)-i-1]);
    }
    ser.print("\n");
    return 0;
  }
}

int8_t GetOutput(int dac, uint32_t & value) {
  value = getDAC(dac);
  return 0;
}

int8_t GetClear(int dac, uint32_t & value) {
  value = getCLR(dac);
  return 0;
}

//================================================================================
// set prompt from controller
int8_t UnknownPrompt(HardwareSerial & ser) {
  DumpBuffer(ser);
  ser.print(NACK);
  ser.print("\n");
  return 0;
}

//================================================================================
// append a single character to a string
uint8_t appendByte(uint8_t * in, uint8_t b, uint8_t index) {
  if ( index < ECHO_BUFF_SIZE ) {
    in[index] = b;
    return ++index;
  }
  return -1;
}

//================================================================================
// wait until there is data on the buffer
int8_t WaitForData(HardwareSerial & ser, uint8_t size) {
  unsigned long startTime = millis();
  while (ser.available() < size ) {
    if (millis() > (TIMEOUT + startTime)) {
      UnknownPrompt(ser);
      return -1;
    }
  }
  delay(5);
  return 0;
}

//================================================================================
// read the number of bytes specified from the serial port as an int
int8_t SerialReadField(
    HardwareSerial & ser, 
    uint8_t size, 
    uint32_t & value) 
{
  // using a union to set a byte at time during the read
  SERIALDATA data ;
  data.lvalue = 0;
  // initialize reference
  value = 0;
  // wait for the data to come in
  if (WaitForData(ser,size) != 0) {
    return -1;
  }
  // fill up barray with struff from the serial port
  for (uint8_t i = 0; i<size; i++) {
    uint8_t in = ser.read();
    // append to value
    data.barray[size-i-1] = in;
  }
  value = data.lvalue;
  return 0;
}

//================================================================================
// read the number of bytes specified from the serial port as an int
int8_t SerialReadField(
    HardwareSerial & ser, 
    uint8_t size, 
    int & value) 
{
  if (size != 1) {
    return -1;
  }
  value = 0;
  if (WaitForData(ser,size) != 0) {
    return -1;
  }
  int in = ser.read();
  // append to value
  value = in;
  return 0;
}

//================================================================================
// reads the buffer until a timeout or a newline is encountered
void DumpBuffer(HardwareSerial & ser) {
  unsigned long startTime = millis();
  while ((ser.available() > 0) | (millis() < (TIMEOUT + startTime))){
    if (ser.read() == NL) {
      break;
    }
  }
}
