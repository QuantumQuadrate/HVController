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
    
    print 'TESTING INITIALIZE: initializing all DACs'
    initialize(ser,3)
    
    print '\nTESTING RESET: resetting and re-initializing all DACs'
    reset(ser,3)
    initialize(ser,3)
    
    print '\nTESTING SYNC: setting to (5,2,1)'
    setsync(ser,[5,2,1])
    print 'Voltages:', readall(ser)
    
    time.sleep(30)
    
    print '\nTESTING CLEAR: setting clear of DAC0 to 2 and clearing'
    print 'Should momentarily set to cleared value and display no error in returned message'
    setclear(ser,0,2)
    print 'Clear values:', readclear(ser,NDAC)
    print repr(clear(ser,0))
    print 'Should set back to DAC output voltages'
    print 'Voltages:', readall(ser)
    
    print '\nTESTING SETVOLTAGE: setting all to 0'
    setvoltage(ser,NDAC,0)
    print 'Voltages:', readall(ser)
    
    print '\nTESTING ECHO'
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