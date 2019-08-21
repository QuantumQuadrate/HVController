# -*- coding: utf-8 -*-
"""
Created on Fri Aug 16 11:12:38 2019

@author: Wendt
"""

import serial
import struct
import time

VrefP = 10.5 #TODO: measure to recalibrate
VrefN = 0
NDAC = 3

# Command characters defined in Arduino code
ECHO = 'E'
SET = 'S'
READ = 'R'

# Secondary command characters
INIT = 'I'
RESET = 'R'
UPDATE = 'U'
CLEAR = 'X'
OUTPUT = 'O'
SETCLR = 'C'
SYNC = 'S'

# ---------------------------------------------------------------------

# Convert from volts to DAC code according to conversion given below
def volt_to_dac(V):
    #V = (VrefP-VrefN)*D/(2^20-1)-VrefN
    #force V within valid range
    if V < VrefP:
        return int(V*(2**20-1)/(VrefP-VrefN)+VrefN)
    else:
        return 2**20-1

# Convert from DAC code to volts according to same conversion
def dac_to_volt(D):
    return (VrefP-VrefN)*D/(2**20-1)-VrefN

# Convert from volts to DAC code and set DAC output register 
def setvoltage(ser,addr,v):
    dacv = volt_to_dac(v)
    msg = struct.pack('>ccBLc',SET,OUTPUT,addr,dacv,'\n')
    ser.write(msg)
    return ser.readline()

# Read output register of addressed DAC
def readvoltage(ser,addr):
    msg = struct.pack('>ccBc',READ,OUTPUT,addr,'\n')
    ser.write(msg)
    returnmsg = ser.readline()
    try:
        if addr < NDAC:
            return dac_to_volt(struct.unpack('>Lc',returnmsg)[0])
        else:
            return [dac_to_volt(d) for d in struct.unpack('>LLLc',returnmsg)[0:NDAC]]
    except:
        return returnmsg

# Read output register of all DACs
def readall(ser):
    return readvoltage(ser,NDAC)

# Synchronous set DAC output registers
# Input: voltages must be ordered list of length NDAC
# if ur just gonna use one voltage then use setvoltage
def setsync(ser,voltages):
    d = [volt_to_dac(v) for v in voltages]
    msg = struct.pack('>ccB%dL' % NDAC,SET,SYNC,NDAC,*d)
    ser.write(msg)
    return ser.readline()

# Set CLR output register (value to set DAC when cleared)
def setclear(ser,addr,v):
    dacv = volt_to_dac(v)
    msg = struct.pack('>ccBLc',SET,SETCLR,addr,dacv,'\n')
    ser.write(msg)
    return ser.readline()

# Clear DAC to CLR value
# Per Arduino code, briefly write 1 to CLR software control register
# Won't see any actual effect
#TODO: Is this desired effect? Should Arduino code be edited?
def clear(ser,addr):
    msg = struct.pack('>ccBBc',SET,CLEAR,addr,0,'\n')
    ser.write(msg)
    return ser.readline()

# Read CLR output register
def readclear(ser,addr):
    msg = struct.pack('>ccBc',READ,SETCLR,addr,'\n')
    ser.write(msg)
    returnmsg = ser.readline()
    try:
        if addr < NDAC:
            returnmsg,g = struct.unpack('>Lc',returnmsg)
            return dac_to_volt(returnmsg)
        else: 
            return [dac_to_volt(d) for d in struct.unpack('>LLLc',returnmsg)[0:NDAC]]
    except:
        return returnmsg

# Initialize DAC: Sets linear compensation mode and sets DAC control register to value in Arduino
# Linear Compensation: 1: 10-12V, 2: 12-16V, 3: 16-19V, 4: 19-20V, default: 0-10V
def initialize(ser,addr):
    linComp = 1
    msg = struct.pack('>ccBBc',SET,INIT,addr,linComp,'\n')
    ser.write(msg)
    return ser.readline()

# Reset DAC: Returns DAC to state immediately after powering on
def reset(ser,addr):
    msg = struct.pack('ccBBc',SET,RESET,addr,0,'\n')
    ser.write(msg)
    return ser.readline()

# Echo: Echos a short message
def echo(ser):
    msg = struct.pack('c4s',ECHO,'cho\n')
    ser.write(msg)
    return ser.readline()