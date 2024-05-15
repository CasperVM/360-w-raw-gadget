"""
This file contains all INPUT packet definitions for the Xbox 360 controller.

https://www.partsnotincluded.com/understanding-the-xbox-360-wired-controllers-usb-data/

====================

Each update from the controller is a 20-byte packet sent from endpoint 1 IN (0x81) on interface 0. 
Byte 0 is the message type (0x00) and byte 1 is the length in bytes (0x14 = 20).
The packed control surface data is stored in 12 bytes in the center of the message (2 – 13). 
The final 6 bytes, 14 – 19, are unused (0x00).

Bytes 2 and 3 contain the packed states for the controller’s digital buttons in a bit array, 
where ‘1’ is pressed and ‘0’ is released. This includes all 8 surface buttons, the 2 joystick buttons, the center ‘Xbox’ button, and the directional pad.

From low to high bit, byte 2 maps to the directional pad (up, down, left, right), start, back, L3, and R3 buttons. 
The first three bits of byte 3 map to the left bumper (LB), right bumper (RB), and ‘Xbox’ button. Bits 4-7 map to the A, B, X, and Y buttons respectively. Bit 3 is unused (0).

Triggers

Bytes 4 and 5 contain 8-bit unsigned integers for the state of the left and right analog triggers (respectively), where a value of ‘0’ is released and a value of ‘255’ is fully depressed.
Joysticks

Bytes 6 – 13 contain the positions of the dual analog joysticks, as 16-bit signed integers. These are stored little endian (low byte first), X axis before Y, with north-east being positive.

The left joystick position is stored in bytes 6, 7, 8, and 9, and the right joystick position is stored in bytes 10, 11, 12, and 13.

Rumble and LED Data (Out)

Updates to the controller’s user feedback features are sent on endpoint 1 OUT (0x01), also on interface 0. These updates follow the same format as the sent data, where byte 0 is the ‘type’ identifier and byte 1 is the packet length (in bytes).
Rumble (0x00)

A type byte of ‘0x00’ indicates a rumble packet. The controller contains two rumble motors: a large weight in the left grip and a small weight in the right grip. The value for both of these motors is updated in a single packet.

The rumble values are 8-bit unsigned integers representing the motor speed, where ‘0’ is off and ‘255’ is max speed. The left motor’s rumble value is stored in index 3, while the right motor’s rumble value is stored in index 4.

This packet is typically 8 bytes long, with bytes 2, 5, 6, and 7 unused (0x00).
LEDs (0x01)

A type byte of ‘0x01’ indicates an LED packet. The controller has four green LEDs around its center ring that display the assigned player number or other status information. This packet selects which LED animation the controller displays. For more information about the LED animations and their timing, see this post.

The identifier for the LED animation is stored in packet index 2. This packet is typically 3 bytes long, with all bytes used.

====================

E.g. A-button packet:

0: 00  // Message type 0
1: 14  // Length 20
2 ( R3 L3, back, start, left dpad, right dpad, down dpad, up dpad (7-0) ): 00
3 ( X Y B A, UNUSED, Xbox, RB, LB (7-0), so bits are 00010000 ): 10
... rest of the packet is 0x00

Triggers: byte 4 and 5 (0-255) left and right
Joysticks: 6-13 (16-bit signed little endian) <- TODO check later
Rumble + LED OUT <- later
"""

A_BUTTON_TEST_PACKET = [
    0x00, 0x14, 0x00, 0x10, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00
]