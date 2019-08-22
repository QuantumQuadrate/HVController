/*
  AD5791 - Library of SPI Functions to communicate with AD5791
  ====== 
  All functions require an input dac to define SS pin.
  
  Created by J. Pritchard, jdpritchard@wisc.edu 5/16/2014
  UW-Madison,Physics,SaffmanLab
 */
 
 #ifndef AD5791_h
 #define AD5791_h
 
#include <inttypes.h>
#include <Arduino.h>
#include <SPI.h>

//DAC Configuration
void setDACconfig(int, int);
uint32_t getDACconfig(int);

//SoftwareControl
void updateDAC(int);
void clearDAC(int);
void resetDAC(int);

//Set/Get DAC Output
void setDAC(int, uint32_t);
uint32_t getDAC(int);

//Set/Get CLR Output
void setCLR(int, uint32_t);
uint32_t getCLR(int);

//Read/Write Commands (need to be removed from public declaration)
void writeRegister(int, uint8_t,uint8_t,uint32_t);
uint32_t readRegister(int, uint8_t);

 #endif
