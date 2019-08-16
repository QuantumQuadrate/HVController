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
#TODO: add better interface for specifying port, in case of changes
port = 'COM7' 

def volt_to_dac(V):
    #V = (VrefP-VrefN)*D/(2^20-1)-VrefN
    #force V within valid range
    if V < VrefP:
        return int(V*(2**20-1)/(VrefP-VrefN)+VrefN)
    else:
        return 2**20-1

def dac_to_volt(D):
    return (VrefP-VrefN)*D/(2**20-1)-VrefN

def readvoltage(ser,addr):
    msg = struct.pack('>ccBc','R','O',addr,'\n')
    ser.write(msg)
    return ser.readline()

def readall(ser):
    return readvoltage(ser,NDAC)

def setvoltage(ser,addr,v):
    dacv = volt_to_dac(v)
    msg = struct.pack('>ccBLc','S','O',addr,dacv,'\n')
    ser.write(msg)
    return ser.readline()

with serial.Serial('COM7', timeout=1) as ser:
    print(ser.name)
    ser.readline()
    
    print('Reading values:')
    print(struct.unpack('>LLLc',readall(ser)))
    
    print('Setting voltage high:')
    print(repr(setvoltage(ser,1,10)))
    
    print('Reading values:')
    print(struct.unpack('>LLLc',readall(ser)))
    
    time.sleep(4)
    
    print('Setting voltage 0:')
    print(repr(setvoltage(ser,1,0)))
    
    print('Reading values:')
    print(struct.unpack('>LLLc',readall(ser)))