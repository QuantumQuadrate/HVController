# -*- coding: utf-8 -*-
"""
Created on Fri Aug 16 11:12:38 2019

This library defines functions for serial communication with the HVController 
Arduino board.

A serial.Serial serial connection is required as an argument for each function.

@author: Danny Wendt
"""

import serial
import struct

VrefP = 10.38
#measured on board powered with 12V/5V, 10.5 precision reference designed for 15V/5V

VrefN = 0
#set to GND

NDAC = 3
#number of DAC boards

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

def volt_to_dac(V):
    """
    Convert from volts to DAC code according to conversion below 
    and force V within valid range.
    V = (VrefP-VrefN)*D/(2^20-1)-VrefN
    
    :param V: Voltage in volts to be converted to DAC code
    :return: DAC voltage code corresponding to given voltage
    """
    if V < VrefP:
        return int(V*(2**20-1)/(VrefP-VrefN)+VrefN)
    else:
        return 2**20-1

# Convert from DAC code to volts according to same conversion
def dac_to_volt(D):
    """
    Convert from DAC code to voltage according to same conversion.
    
    :param D: DAC voltage code to be converted to voltage
    :return: Voltage in volts corresponding to given DAC code
    """
    return (VrefP-VrefN)*D/(2**20-1)-VrefN

def setvoltage(ser,addr,v):
    """
    Set DAC output register to value given in volts 
    (converts to DAC code within function)
    
    :param ser: Serial port for communication
    :param addr: Address of DAC to be addressed (from 0 to NDAC-1, or NDAC to address all)
    :param v: Voltage in volts to set DAC output register to
    :return: Serial output from Arduino, either echo of command or error code
    """
    dacv = volt_to_dac(v)
    msg = struct.pack('>ccBLc',SET,OUTPUT,addr,dacv,'\n')
    ser.write(msg)
    return ser.read(len(msg))

def readvoltage(ser,addr):
    """
    Read DAC output register of addressed DAC
    
    :param ser: Serial port for communication
    :param addr: Address of DAC to be addressed
    :return: Voltage(s) listed in DAC output register addressed, or error code
    """
    msg = struct.pack('>ccBc',READ,OUTPUT,addr,'\n')
    ser.write(msg)
    try:
        if addr < NDAC:
            returnmsg = ser.read(4+1)
            return dac_to_volt(struct.unpack('>Lc',returnmsg)[0])
        else:
            returnmsg = ser.read(3*4+1)
            return [dac_to_volt(d) for d in struct.unpack('>LLLc',returnmsg)[0:NDAC]]
    except Exception as e:
        print e
        return repr(returnmsg)

# Read output register of all DACs
def readall(ser):
    """
    Quick function to read voltages of all DACs.
    """
    return readvoltage(ser,NDAC)

def setsync(ser,voltages):
    """
    Synchronously set DAC output registers.
    Each DAC is set to its own value, and they all update synchronously.
    
    :param ser: Serial port for communication
    :param voltages: List of voltages (in volts) of length NDAC, in order
    :return: Serial output from Arduino, either echo of command or error code
    """
    d = [volt_to_dac(v) for v in voltages]
    msg = struct.pack('>ccB%dL' % NDAC,SET,SYNC,NDAC,*d)
    ser.write(msg)
    return ser.read(len(msg))

def setclear(ser,addr,v):
    """
    Set CLR output register (value to pulse when cleared)
    
    :param ser: Serial port for communication
    :param addr: Address of DAC to be addressed
    :param v: Voltage in volts to set CLR output register to
    :return: Serial output from Arduino, either echo of command or error code
    """
    dacv = volt_to_dac(v)
    msg = struct.pack('>ccBLc',SET,SETCLR,addr,dacv,'\n')
    ser.write(msg)
    return ser.read(len(msg))

# Clear DAC to CLR value
# Per Arduino code, briefly write 1 to CLR software control register
# Pulses CLR register voltage then returns to DAC register voltage
def clear(ser,addr):
    """
    Pulse voltage in CLR register, then return to DAC register voltage.
    Voltage cannot be held at CLR value.
    
    :param ser: Serial port for communication
    :param addr: Address of DAC to be addressed
    :return: Serial output from Arduino, either echo of command or error code
    """
    msg = struct.pack('>ccBBc',SET,CLEAR,addr,0,'\n')
    ser.write(msg)
    return ser.read(len(msg))

def readclear(ser,addr):
    """
    Read CLR output register of addressed DAC
    
    :param ser: Serial port for communication
    :param addr: Address of DAC to be addressed
    :return: Voltage(s) listed in CLR output register addressed, or error code
    """
    msg = struct.pack('>ccBc',READ,SETCLR,addr,'\n')
    ser.write(msg)
    returnmsg = ser.read(3*4+1)
    try:
        if addr < NDAC:
            returnmsg,g = struct.unpack('>Lc',returnmsg)
            return dac_to_volt(returnmsg)
        else: 
            return [dac_to_volt(d) for d in struct.unpack('>LLLc',returnmsg)[0:NDAC]]
    except:
        return returnmsg

def initialize(ser,addr,linComp):
    """
    Initialize DAC: Sets linear compensation mode and resets DAC control 
    register to value coded into Arduino (Old Labview code uses mode 1)
    Linear Compensation modes: 1: 10-12V, 2: 12-16V, 3: 16-19V, 4: 19-20V, default: 0-10V
    
    :param ser: Serial port for communication
    :param addr: Address of DAC to be addressed
    :param linComp: Linear compensation mode - see above list
    :return: Serial output from Arduino, either echo of command or error code
    """
    msg = struct.pack('>ccBBc',SET,INIT,addr,linComp,'\n')
    ser.write(msg)
    return ser.read(len(msg))

# Reset DAC: Returns DAC to state immediately after powering on
def reset(ser,addr):
    """
    Reset DAC: Returns DAC to state immediately after powering on
    Control registers are all reset to defaults
    
    :param ser: Serial port for communication
    :param addr: Address of DAC to be addressed
    :return: Serial output from Arduino, either echo of command or error code
    """
    msg = struct.pack('ccBBc',SET,RESET,addr,0,'\n')
    ser.write(msg)
    return ser.read(len(msg))

# Echo: Echos a short message
def echo(ser):
    """
    Asks Arduino to echo a short message (up to 4 characters, set to 'Echo' here)
    Tests serial connection with Arduino.
    
    :param ser: Serial port for communication
    :return: Serial output from Arduino, either echo of command or error code
    """
    msg = struct.pack('c4s',ECHO,'cho\n')
    ser.write(msg)
    return ser.readline()