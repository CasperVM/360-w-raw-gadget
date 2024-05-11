#!/usr/bin/env python3
from evdev import InputDevice, categorize, ecodes
import time

# Path to your controller device (find using `ls /dev/input/`)
gamepad = InputDevice('/dev/input/eventX')

# Same NULL_CHAR and write_report from your script
NULL_CHAR = chr(0)

def write_report(report):
    with open('/dev/hidg0', 'rb+') as fd:
        fd.write(report.encode())

# Mapping of controller buttons to keyboard keys (example)
button_to_key = {
    # Assuming button codes from your controller
    288: chr(4),  # Button A -> Keyboard 'a'
    289: chr(5),  # Button B -> Keyboard 'b'
    # Add more mappings as needed
}

# Loop to handle button presses
for event in gamepad.read_loop():
    if event.type == ecodes.EV_KEY:
        data = categorize(event)
        if data.keystate == 1:  # Button pressed
            key = button_to_key.get(data.scancode)
            if key:
                write_report(NULL_CHAR*2 + key + NULL_CHAR*5)
                time.sleep(0.1)  # Delay to simulate key press
                write_report(NULL_CHAR*8)  # Release key
