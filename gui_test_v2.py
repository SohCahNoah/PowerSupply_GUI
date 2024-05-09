#!/usr/bin/env python
# -*- coding: utf-8 -*-
"""
Program Title:  Temperature Sensor GUI
Description:    This code uses tkinter to create a windows GUI. Background processes will
                communicate with an Arduino over a serial bus (USB), sending predefined commands.
                This is custom code and is meant to work with custom Arduino firmware to receive
                commands. The arduino in turn communicates via I2C to an LM75A temperture sensor
                module, sending data back to this GUI if necessary (such as in the GET_TEMP function
                below). Any bugs should be report to the author below.
Author:         Noah Roberts, SID: 932-989-402, robertno@oregonstate.edu
Date:           05/09/24
Version:        Version 2.1
"""


from tkinter import *
from tkinter import messagebox
from serial import Serial
from serial.tools.list_ports import comports
import time
import threading
import os

current_temp = '--'
current_temp_f = '--'
warning_temp = 80
temp_status = "GREEN"
temp_style = ('TkDefaultFont', 10)
label_style = ('TkDefaultFont', 10)

temp_buffer = None

#Dynamic detection of Arduino connected to system
def detect_arduino_port():
    ports_list = comports()
    for port, desc, hwid in sorted(ports_list):
        if "Arduino" in desc:
            return port
    return None

#Find the port that the arduino is on, if not found create an error window and exit the program
port = detect_arduino_port()
if port is None:
    print("Arduino not found...\n")
    messagebox.showerror("Error: Device not found", "Arduino was not found. Please connect device and run the application again.")
    exit()

#Open up the serial communication port
ser = Serial(port=port, baudrate=9600, timeout=0)
serial_lock = threading.Lock()

#Communicates with Arduino Firmware code to grab the current temperature
def read_temp():
    global ser
    with serial_lock:
        ser.write(b'GET_TEMP\n')
        temp_buffer = ser.readline().strip().decode('utf-8')
        if (temp_buffer.isdigit()):
            update_temp_readout(int(temp_buffer))
        else:
            temp_buffer = None

#Configures the warning temp readout to update with new_temp
def update_warning_temp_readout(new_temp):
    if (new_temp == ""):
        warning_temp_readout.config(text="0 °C")
    else:
        warning_temp_readout.config(text=f"{new_temp} °C")            

#Communicate with Arduino Firmware code to update the OS temperature value
def update_tempOS(new_temp):
    global ser
    ACK_Condition = False
    start_time = time.time()
    timeout = 1
    message = new_temp + "\n"
    
    with serial_lock:
        ser.write(b'SET_TEMP\n')
        line = None
        
        try:
            while True:
                if time.time() - start_time > timeout:
                    raise TimeoutError("Timeout Limit Reached during 'update_tempOS()' operation...")
                
                ser.write(message.encode())
                
                if (line == "ACK"):
                    update_warning_temp_readout(new_temp)
                    break
                else:
                    line = ser.readline().strip().decode('utf-8')
                time.sleep(0.25)
                
        except TimeoutError as e:
            print(e)

#Configures the ambient temp readout to update with new_temp
def update_temp_readout(new_temp):
    global current_temp, current_temp_f, temp_status
    current_temp = new_temp
    current_temp_f = round((current_temp*1.8) + 32)
    
    if(current_temp > warning_temp):
        temp_status = "RED"
        temp_style = ('TkDefaultFont', 10, 'bold')
        
    elif(current_temp > warning_temp - 25):
        temp_status = "dark orange"
        temp_style = ('TkDefaultFont', 10)
    else:
        temp_status = "GREEN"
        temp_style = ('TkDefaultFont', 10)


    amb_temp_readout.config(text=f"{current_temp} °C | {current_temp_f} °F", fg=temp_status, font=temp_style)
    
    root.update()

#Clears the user entry field when called
def clear_tempOS():
    temp_os_userEntry.delete(0, END)

#Sends the user entry to be configured as the new temp_os in the Arduino code
def confirm_tempOS():
    new_tempOS = temp_os_userEntry.get()
    temp_os_userEntry.delete(0, END)
    if (not new_tempOS.isdigit()):
        return
    update_tempOS(new_tempOS)

#Runs on a background thread to grab the current ambient temp, allowing for other calls (such as SET_TEMP) to be made at the same time
def background_task():
    global temp_buffer
    while True:
        read_temp()
        if temp_buffer is not None:
            update_temp_readout(temp_buffer)
            temp_buffer = None
        time.sleep(0.25)

#-----------------------------------------------
#   Windows Application Functions and Objects
#-----------------------------------------------

#--- Initialize Window and Frame Objects
root = Tk()
root.geometry("960x540")                    #Sets window size
root.resizable(width=False, height=False)   #Restricts scaling of window
if os.path.exists("icon.ico"):
    root.iconbitmap('icon.ico')

#Frame object for all temperature display data
temps_frame = Frame(root, highlightbackground="black", highlightthickness=1)
temps_frame.grid(row=0, column=0, sticky=W+E)

#Frame object for all control data
controls_frame = Frame(root)
controls_frame.grid(row=1, column=0, sticky=W+E)

#Frame object for temp_os data
temp_os_entry_frame = Frame(root)
temp_os_entry_frame.grid(row=2, column=0, sticky=W+E)

#Frame object for temp_os buttons
temp_os_button_frame = Frame(root)
temp_os_button_frame.grid(row=3, column=0, sticky=W+E)

#--- Entry Validation Logic
def validate_int(text):
    #Restricts text entry to integers and restricts to a max length of 3 characters
    if (text.isdigit() or text == "") and len(text) <= 3:   
        return True
    else:
        return False
    
#Registers function with underlying tcl logic
#NOTE: I don't understand why this needs to be here, but the code doesn't work with out it...
validation = root.register(validate_int)

#---Temperature Data Widgets ---
amb_temp_label = Label(temps_frame, text="Ambient Temperature: ", font=label_style)
amb_temp_label.grid(row=0, column=0, padx=10, pady=10)

amb_temp_readout = Label(temps_frame, text=f"{current_temp} °C | {current_temp_f} °F", fg=temp_status)
amb_temp_readout.grid(row=0, column=1, padx=10, pady = 0)

warning_temp_label = Label(temps_frame, text="Current Warning Temperature: ", font=label_style)
warning_temp_label.grid(row=1, column=0, padx=10, pady=10)

warning_temp_readout = Label(temps_frame, text=f"{warning_temp} °C")
warning_temp_readout.grid(row=1, column=1, padx=10, pady=0)

#--- OS Temp Data Widgets
temp_os_userEntry_label = Label(temp_os_entry_frame, text="Warning Temperature Adjustment:")
temp_os_userEntry_label.grid(row=0, column=0, padx=50, pady=10)

temp_os_userEntry = Entry(temp_os_entry_frame, relief=SUNKEN, width=10, validate="key", validatecommand=(validation, '%P'))
temp_os_userEntry.grid(row=1, column=0, padx=10, pady=0)

temp_os_clearBtn = Button(temp_os_button_frame, text="Clear", relief=RAISED, command=clear_tempOS)
temp_os_clearBtn.grid(row=2, column=0, padx=(90,0), pady=10)

temp_os_confirmBtn = Button(temp_os_button_frame, text="Confirm", relief=RAISED, command=confirm_tempOS)
temp_os_confirmBtn.grid(row=2, column=1, padx=10, pady=10)

#--- Background Threading Processes
serial_thread = threading.Thread(target=background_task)
serial_thread.daemon = True
serial_thread.start()

root.mainloop()     #Runs main loop

# What I have done:
# - Frames for both temp and for controls have been created
# - Logic for updating the temperature output has been created
# - Lable for temperature output has been created
# - Temp color changes depending on current value
# - Connected this code to the serial output code
# - Implemented listening logic to update temperature value as data is received
# - Implemented control function where the user can update the warning temperature, as well as clear and confirm buttons
# - Implemented communication with device to updated T_OS within the update_tempOS() function