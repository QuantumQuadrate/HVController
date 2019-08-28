# -*- coding: utf-8 -*-
"""
Created on Wed Aug 21 13:49:27 2019

@author: Wendt
"""

import time
from hvcontroller import *
import serial
import numpy as np

port = 'COM3'

with serial.Serial(port, timeout=1) as ser:
    
    reset(ser,3)
    initialize(ser,3,1)
    
    for i in np.arange(0,11,1):
        print i
        setvoltage(ser,0,i)
        print readall(ser)
        time.sleep(20)
    
    setvoltage(ser,3,0)
    print readall(ser)
