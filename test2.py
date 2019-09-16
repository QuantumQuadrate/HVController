# -*- coding: utf-8 -*-
"""
Created on Wed Aug 21 13:49:27 2019

@author: Wendt
"""

import time
from hvcontroller import *
import serial

port = 'COM7'

with serial.Serial(port, timeout=1) as ser:
    
    reset(ser,3)
    initialize(ser,3,1)
    
    for i in range(0,12):
        print (i)
        setvoltage(ser,0,i)
        print (readall(ser))
        #time.sleep(4)
    setvoltage(ser,3,0)
    print (readall(ser))
    
