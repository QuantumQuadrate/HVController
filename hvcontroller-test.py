# -*- coding: utf-8 -*-
"""
Created on Mon Aug 19 09:47:33 2019

@author: Wendt
"""

from hvcontroller import *
import serial

port = 'COM7'

with serial.Serial(port, timeout=1) as ser:
    ser.readline()
    setsync(ser,[5,2,1])
    print readall(ser)
    setvoltage(ser,NDAC,0)
    print readall(ser)
    print echo(ser)
    
#with serial.Serial(port, timeout=1) as ser:
#    print(ser.name)
#    ser.readline()
#    
#    print('Reading values:')
#    print(struct.unpack('>LLLc',readall(ser)))
#    
#    print('Setting voltage high:')
#    print(repr(setsync(ser,0,10)))
#    
#    print('Reading values:')
#    print(struct.unpack('>LLLc',readall(ser)))
#    
#    time.sleep(30)
#    
#    print('Setting voltage 0:')
#    print(repr(setvoltage(ser,3,0)))
#    
#    print('Reading values:')
#    print(struct.unpack('>LLLc',readall(ser)))