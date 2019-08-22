/*
AD5791 - Library of SPI Functions to communicate with AD5791
====== 
All functions require an input dac to define SS pin.

Created by J. Pritchard, jdpritchard@wisc.edu 5/16/2014
UW-Madison,Physics,SaffmanLab
 */
#include "Arduino.h"
#include "SPI.h"
#include "AD5791.h"
//#include "stdint.h"

//AD5971 Register Addresses
const uint8_t DAC_ADDR=0x10;
const uint8_t CTL_ADDR=0x20;
const uint8_t CLR_ADDR=0x30;
//AD5971 Read/Write Bit
const uint8_t DAC_WRITE=0x0;
const uint8_t DAC_READ=0x80;

//Reconfigure DAC from Power On State to enable output and accept binary
void setDACconfig(int dac, int linComp) {
  //Update Control Register: Bit Definitions Included Below,* is power on state
  //
  //  0 = Reserved(0)
  //  1 = RBUF {0=Internal Amplifier,*1=External Amplifier)
  //  2 = OPNGND {0=Normal,*1=GND}
  //  3 = DACTRI {0=Normal,*1=Tristate}
  //  4 = BIN/I2C {*0=TwosCompliment,1=Binary)
  //  5 = SODIS (*0=Enabled,1=Disabled)
  //  9-6 = LINCOMP {*0000=0-10V,1001=10-12V,1010=12-16V,1011=16-19V,1100=19-20V}
  //  11-19 = Reserved(0)
  
  uint32_t configuration=0x0;
  
  //Set RBUF=1,OPNGND=0,DACTRI=0,BIN/I2C=1,SODIS=0 --> B010010 = 0x12
  configuration|=0x12;
  
  //Set Linear Compensation Mode
  switch(linComp) {
    case 1: //10-12V [1001]
      configuration|=0x240;
      break;
    case 2: //12-16V [1010]
      configuration|=0x280;
      break;
    case 3: //16-19V [1011]
      configuration|=0x2C0; 
      break;     
    case 4: //19-20V [1100]
      configuration|=0x300; 
      break;      
    default:;
      //0-10V [0000]
  }  
  
  //Update Control Register
  writeRegister(dac, DAC_WRITE,CTL_ADDR, configuration);
}

//Get DAQ CTL Register
uint32_t getDACconfig(int dac) {
  return readRegister(dac, CTL_ADDR);
}

//Software Update via software control register (LDAC=DB0)
void updateDAC(int dac) {
  writeRegister(dac,DAC_WRITE,0x40,0x1);
  writeRegister(dac,DAC_WRITE,0x40,0x0);
}
//Software Clear via software control register (CLR=DB2)
void clearDAC(int dac) {
  writeRegister(dac,DAC_WRITE,0x40,0x2);
  writeRegister(dac,DAC_WRITE,0x40,0x0);
}

//Software Reset via software control register (Reset=DB2)
void resetDAC(int dac) {
  writeRegister(dac,DAC_WRITE,0x40,0x4);
  writeRegister(dac,DAC_WRITE,0x40,0x0);
}

//Set DAC Value
void setDAC(int dac, uint32_t data) {
  writeRegister(dac,DAC_WRITE, DAC_ADDR, data);
}

//Get DAC Value
uint32_t getDAC(int dac) {
  return readRegister(dac, DAC_ADDR);
}

//Set CLR Value
void setCLR(int dac, uint32_t data) {
  writeRegister(dac,DAC_WRITE, CLR_ADDR, data);
}

//Get CLR Value
uint32_t getCLR(int dac) {
  return readRegister(dac, CLR_ADDR);
}

//Write to Register 
void writeRegister(int dac, uint8_t rw,uint8_t addr,uint32_t data) {
  digitalWrite(dac,LOW);
  SPI.transfer(rw | addr | (data>>16));
  SPI.transfer(data>>8);
  SPI.transfer(data);
  digitalWrite(dac,HIGH);
 }

//Read register
uint32_t readRegister(int dac, uint8_t addr) {
  uint32_t data=0x0;
  byte b;
  
  //Send Readout Command
  writeRegister(dac,DAC_READ,addr,data);
  
  //Readout Data
  digitalWrite(dac,LOW);
  for (int a=0; a<3; a++) {
    b=SPI.transfer(0x0);
    //Serial.println(b);
    data = data << 8;
    data |= b;
  }
  digitalWrite(dac,HIGH); 
  
  //Mask 20 useful bits
  data&=0x000FFFFF ;
  return data;     
 }
