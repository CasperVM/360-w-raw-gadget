# 360-w-raw-gadget
360 controller wireless receiver raw USB gadget. (WIP)

This project aims to allow emulating multiple controllers over USB. This works by using the UDC hardware, usually found on small SOC's like a raspberry pi zero.

Initially tried this using functionfs in both python and Rust, but this only worked on *nix hosts.
The reason for this is that functionfs/gadgetfs only allows for well-defined USB classes/well-formed descriptors.
After the interface descriptor, e.g. Windows expects a vendor-specific descriptor, which is not supported by functionfs.

Thus, we're using raw-gadget + C here.

(First project using C, so don't judge too hard)

Originally, I made this for 1 controller at first, this would be a bit more close to the hardware.
You can find that [here.](https://github.com/Berghopper/360-raw-gadget)

**Small disclaimer**: This doesn't actually properly emulate the receiver, it's a bit of a cursed mix of various descriptors of both the receiver + the xbox controller. It's just to trick various drivers to pick up multiple input xbox controllers.

## Functionality

For now only the input endpoints are implemented, which allows for controller input to be sent to the host.

Rumble/led feedback is on the todo list, but will be left out for now.

## Building for Specific Hardware

This project supports a range of hardware configurations, as listed below. You can build the project for your specific device using the appropriate make target.

| Hardware                           | Build Target            | Driver           | Device                  |
| ---------------------------------- | ----------------------- | ---------------- | ----------------------- |
| Raspberry Pi Zero (default target) | `make`                  | `20980000.usb`   | `20980000.usb` (dwc2)   |
| Raspberry Pi Zero 2                | `make rpi0_2`           | `3f980000.usb`   | `3f980000.usb` (dwc2)   |
| Raspberry Pi 4                     | `make rpi4`             | `fe980000.usb`   | `fe980000.usb` (dwc2)   |
| Raspberry Pi 5                     | `make rpi5`             | `1000480000.usb` | `1000480000.usb` (dwc2) |
| USB Armory Mk II                   | `make usb-armory-mk2`   | `2184000.usb`    | `ci_hdrc.0`             |
| Orange Pi PC                       | `make orange-pi-pc`     | `musb-hdrc`      | `musb-hdrc.4.auto`      |
| Khadas VIM1                        | `make khadas-vim1`      | `c9100000.usb`   | `c9100000.usb`          |
| ThinkPad X1 Carbon Gen 6           | `make thinkpad-x1`      | `dwc3-gadget`    | `dwc3.1.auto`           |
| NXP i.MX8MP                        | `make nxp-imx8mp`       | `dwc3-gadget`    | `38100000.usb`          |
| BeagleBone Black                   | `make beaglebone-black` | `musb-hdrc`      | `musb-hdrc.0`           |
| BeagleBone AI                      | `make beaglebone-ai`    | `dwc3-gadget`    | `48380000.usb`          |
| EC3380-AB                          | `make ec3380-ab`        | `net2280`        | `0000:04:00.0`          |
| Odroid C2                          | `make odroid-c2`        | `dwc_otg_pcd`    | `dwc2_a`                |
| Generic hardware                   | `make generic`          | `dummy_udc`      | `dummy_udc.0`           |

(Table is taken from the [raw-gadget](https://github.com/xairy/raw-gadget?tab=readme-ov-file#usb-device-controllers) repository)

To build for your device, use the corresponding target, for example:

```bash
make rpi4
```

For the Raspberry Pi Zero / Zero 2, you can use the default target:

```bash
make
```

## Running

To run the gadget, you need to load the kernel module for raw-gadget and then run the gadget binary.

For the raw-gadget module, you can follow the instructions in the raw-gadget repo, e.g. for raspberry pi's: [setup_raspberry-pi.md](https://github.com/xairy/raw-gadget/blob/master/docs/setup_raspberry-pi.md)

After loading the module, you can run the gadget example binary:

```bash
sudo ./example
```

This will run the example function, which activates the A button every second for 4 controllers at the same time.

For more input control, take a look at the `partsnotincluded` link in the sources section.

## Static lib

If you want to use this in other projects, you can edit the makefile `CFLAGS`. More info the makefile.
Afterwards you can link the functions found in `360-w-gadget.h` via the `lib360wgadget.a` static lib.

## Issues;

- Multiple controllers don't seem to work out of the box on MacOS. One could attempt installing proper drivers; [https://github.com/360Controller/360Controller](https://github.com/360Controller/360Controller)
- If input endpoints, for whatever reason aren't recognized properly (like in MacOs) they will hang.
- No rumble/led feedback yet.

## TODO:

- [ ] Implement output endpoints for rumble
- [ ] Audio endpoints support?

## Sources

useful links/sources:

- [(partsnotincluded.COM) 360 usb data explainer](https://www.partsnotincluded.com/understanding-the-xbox-360-wired-controllers-usb-data/)
- [usb raw gadget repo](https://github.com/xairy/raw-gadget)


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