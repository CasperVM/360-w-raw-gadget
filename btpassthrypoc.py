import evdev
from evdev import InputDevice, categorize, ecodes
import time
import os


# Function to find the gamepad
def find_gamepad():
    devices = [evdev.InputDevice(path) for path in evdev.list_devices()]
    for device in devices:
        if (
            "gamepad" in device.name.lower()
        ):  # Adjust the condition based on your gamepad name
            return device
    return None


# Function to write to HID gadget
def write_report(fd, report):
    fd.write(report)
    fd.flush()


# Open the HID gadget file
gadget_path = "/dev/hidg0"
if not os.path.exists(gadget_path):
    raise Exception("Gadget path does not exist. Check your gadget setup.")
fd = open(gadget_path, "wb+", buffering=0)

# Find the gamepad device
gamepad = find_gamepad()
if gamepad is None:
    raise Exception(
        "Gamepad not found. Ensure it is connected and the name is correctly identified in the script."
    )
print(f"Using gamepad: {gamepad.name}")

# Main loop to read input and write to gadget
try:
    for event in gamepad.read_loop():
        if event.type == ecodes.EV_KEY or event.type == ecodes.EV_ABS:
            report = bytearray(
                8
            )  # Example report size, adjust as per your HID descriptor
            if event.type == ecodes.EV_KEY:
                button_map = {
                    304: 0,
                    305: 1,
                    307: 2,
                    308: 3,
                }  # Mapping example, adjust to your setup
                button_index = button_map.get(event.code, None)
                if button_index is not None:
                    if event.value == 1:  # Button press
                        report[button_index] = 1
                    else:  # Button release
                        report[button_index] = 0
            elif event.type == ecodes.EV_ABS:
                axis_map = {0: 4, 1: 5}  # Axis example, map to your HID report
                axis_index = axis_map.get(event.code, None)
                if axis_index is not None:
                    # Scale axis value to fit USB HID values, example assumes -127 to 127
                    scaled_value = int((event.value / 32767.0) * 127)
                    report[axis_index] = scaled_value & 0xFF

            write_report(fd, report)
finally:
    fd.close()
    gamepad.close()
