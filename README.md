# 360-raw-gadget
360 controller raw USB gadget attempt.

Initially tried using functionfs in both python and Rust, but this only worked on *nix hosts.
The reason for this is that functionfs/gadgetfs only allows for well-defined USB classes/well-formed descriptors.
After the interface descriptor, windows presumably expects a vendor-specific descriptor, which is not supported by functionfs.

Thus, we'll try to use raw-gadget + c at least for emulating the controller.

(First project using C, so don't judge too hard)

# Sources

useful links/sources:

- [360 usb data explainer](https://www.partsnotincluded.com/understanding-the-xbox-360-wired-controllers-usb-data/) 
 -[usb raw gadget repo](https://github.com/xairy/raw-gadget)


More sources (old):
- [Composite USB Gadgets on the Raspberry Pi Zero](http://www.isticktoit.net/?p=1383)
- [GPIO Joystick Driver](https://github.com/recalbox/mk_arcade_joystick_rpi)
- [pi Pico based controller code](https://github.com/printnplay/PicoCader)
- [Pi Pico based controller with circuitpython YT](https://www.youtube.com/watch?v=__QZQEOG6tA)
- [Pi forums controller question](https://forums.raspberrypi.com/viewtopic.php?t=207197)
- [USB Hid Descriptor Tutorial](https://eleccelerator.com/tutorial-about-usb-hid-report-descriptors/)
- [HID descriptor question](https://stackoverflow.com/questions/49139136/emulate-a-gaming-device-raspberry-pi-zero/49151408#49151408)
- [Pi Zero as USB controller](https://www.reddit.com/r/RetroPie/comments/4vi0it/pi_zero_as_usb_controller/)
- [Pi Zero as USB controller 2](https://www.reddit.com/r/raspberry_pi/comments/4vkffh/pi_zero_as_usb_nes_controller/)

