# 360-w-raw-gadget

[![crates.io](https://img.shields.io/crates/v/x360-w-raw-gadget)](https://crates.io/crates/x360-w-raw-gadget)
[![docs.rs](https://docs.rs/x360-w-raw-gadget/badge.svg)](https://docs.rs/x360-w-raw-gadget)
[![CI](https://github.com/CasperVM/360-w-raw-gadget/actions/workflows/ci.yml/badge.svg)](https://github.com/CasperVM/360-w-raw-gadget/actions/workflows/ci.yml)
[![crates.io downloads](https://img.shields.io/crates/d/x360-w-raw-gadget)](https://crates.io/crates/x360-w-raw-gadget)
[![MSRV](https://img.shields.io/badge/MSRV-1.70-orange?logo=rust)](https://www.rust-lang.org/)
[![License: MIT](https://img.shields.io/badge/license-MIT-green)](LICENSE)
[![Platform](https://img.shields.io/badge/platform-Linux-lightgrey?logo=linux&logoColor=white)](https://www.kernel.org/)

Emulates an Xbox 360 wireless receiver (up to 4 controllers) on Linux via the raw-gadget kernel interface.

The current implementation is Rust-first and ships both:
- a standalone binary for directly running the gadget, and
- a C-callable shared library for Python/ctypes or other FFI consumers.

Tested on Raspberry Pi Zero 2W.

## Overview

This project exists to make a Linux USB gadget device pretend to be an Xbox 360 wireless receiver. In practice, that lets a small OTG-capable board sit between a host and some other input source and expose up to 4 virtual Xbox controller slots.

The original version of the project was built around raw-gadget plus C. The Rust codebase is now the primary implementation, but the older C sources and Makefile are still kept in the repository as reference/legacy build artifacts.

> Small disclaimer: this is not a perfect hardware-faithful emulation of the original Microsoft receiver. It intentionally uses a somewhat cursed but practical mix of descriptors so common host drivers will accept it and expose Xbox-style controllers. (yes maybe the naming could be better...)

## Why raw-gadget?

Earlier attempts using functionfs/gadgetfs ran into descriptor limitations: some hosts expect vendor-specific descriptors that those interfaces do not handle well for this use case. raw-gadget provides the lower-level control needed to present the receiver shape that host drivers are willing to bind to.

## Current functionality

Implemented today:
- up to 4 controller slots
- input report submission
- host-to-device rumble polling
- host-to-device LED polling
- Rust library API
- C-callable shared library API

## Requirements

- Linux with raw-gadget kernel support
- USB OTG-capable hardware (for example Raspberry Pi Zero / Zero 2W / Pi 4 in device mode)
- Rust toolchain 1.70+

Load the kernel module before running the gadget:

```bash
sudo modprobe raw_gadget
```

For Raspberry Pi-specific setup, the raw-gadget repo has a useful guide:
[`docs/setup_raspberry-pi.md`](https://github.com/xairy/raw-gadget/blob/master/docs/setup_raspberry-pi.md)

## Building

### Rust build (recommended)

```bash
# Release build (produces binary + .so)
cargo build --release

# Run tests (no hardware needed)
cargo test --lib
```

Outputs:
- `target/release/x360-w-gadget` — standalone binary
- `target/release/libx360_w_raw_gadget.so` — shared library for Python/C

### Legacy C build

The repository still includes the older C implementation and Makefile. If you specifically need that build path:

```bash
make
```

That produces:
- `example`
- `lib360wgadget.a`
- `lib360wgadget.so`

The Makefile hardware targets below apply to this legacy C build path.

## Running the Rust binary

```bash
sudo ./target/release/x360-w-gadget [num_interfaces] [driver] [device] [--demo] [--debug]
```

| Argument | Default | Description |
|---|---|---|
| `num_interfaces` | `4` | Number of controller slots (1–4) |
| `driver` | `3f980000.usb` | UDC driver name |
| `device` | same as driver | UDC device name |
| `--demo` | off | Toggle A button on all slots every second |
| `--debug` | off | Print verbose USB traffic to stderr |

Example:

```bash
sudo ./target/release/x360-w-gadget 1 3f980000.usb --debug
```

Debug logging can also be enabled by setting `X360_DEBUG` in the environment.

## Hardware targets reference

If you are using the legacy Makefile build, these hardware presets are available:

| Hardware | Build Target | Driver | Device |
| --- | --- | --- | --- |
| Raspberry Pi Zero (default target) | `make` | `20980000.usb` | `20980000.usb` (dwc2) |
| Raspberry Pi Zero 2 | `make rpi0_2` | `3f980000.usb` | `3f980000.usb` (dwc2) |
| Raspberry Pi 4 | `make rpi4` | `fe980000.usb` | `fe980000.usb` (dwc2) |
| Raspberry Pi 5 | `make rpi5` | `1000480000.usb` | `1000480000.usb` (dwc2) |
| USB Armory Mk II | `make usb-armory-mk2` | `2184000.usb` | `ci_hdrc.0` |
| Orange Pi PC | `make orange-pi-pc` | `musb-hdrc` | `musb-hdrc.4.auto` |
| Khadas VIM1 | `make khadas-vim1` | `c9100000.usb` | `c9100000.usb` |
| ThinkPad X1 Carbon Gen 6 | `make thinkpad-x1` | `dwc3-gadget` | `dwc3.1.auto` |
| NXP i.MX8MP | `make nxp-imx8mp` | `dwc3-gadget` | `38100000.usb` |
| BeagleBone Black | `make beaglebone-black` | `musb-hdrc` | `musb-hdrc.0` |
| BeagleBone AI | `make beaglebone-ai` | `dwc3-gadget` | `48380000.usb` |
| EC3380-AB | `make ec3380-ab` | `net2280` | `0000:04:00.0` |
| Odroid C2 | `make odroid-c2` | `dwc_otg_pcd` | `dwc2_a` |
| Generic hardware | `make generic` | `dummy_udc` | `dummy_udc.0` |

This table originally came from the [raw-gadget repository](https://github.com/xairy/raw-gadget?tab=readme-ov-file#usb-device-controllers).

## Rust API

```rust
use x360_w_raw_gadget::{
    Button, InputState, ConfigDescriptorSet, WirelessReceiver,
    transport_hw::RawGadgetTransport,
};

let config = ConfigDescriptorSet::new(1).unwrap();
let transport = RawGadgetTransport::new(&config, "3f980000.usb", "3f980000.usb").unwrap();
let mut receiver = WirelessReceiver::new(config, Box::new(transport));

// Build and send input state
let state = InputState::default()
    .with_button(Button::A, true)
    .with_left_stick(-16000, 8000)
    .with_left_trigger(128);
receiver.slot_mut(0).unwrap().set_state(state);
receiver.send_input(0).unwrap();

// Poll for rumble / LED from the host
if let Ok(Some(r)) = receiver.poll_rumble(0) {
    println!("rumble left={} right={}", r.left_motor, r.right_motor);
}
if let Ok(Some(l)) = receiver.poll_led(0) {
    println!("led animation={:?}", l.animation);
}
```

### InputState fields

| Field | Type | Notes |
|---|---|---|
| `a b x y lb rb l3 r3 start back guide` | `bool` | Digital buttons |
| `dpad_up dpad_down dpad_left dpad_right` | `bool` | D-pad |
| `left_trigger right_trigger` | `u8` | 0–255 |
| `left_stick_x left_stick_y` | `i16` | ±32767 |
| `right_stick_x right_stick_y` | `i16` | ±32767 |

## C / Python API

The shared library exports six symbols:

```c
// Open a receiver. Returns opaque handle or NULL on failure.
// Pass NULL for driver/device to use the Pi Zero 2W default.
void* x360_open(int num_slots, const char* driver, const char* device);

// Close a handle opened with x360_open.
void  x360_close(void* handle);

// Send a raw 20-byte input report for slot (0-based).
// Returns 1 on success, 0 on error.
int   x360_send(void* handle, int slot, const uint8_t* data, size_t len);

// Poll for a rumble command. Writes motor values to *left_out/*right_out.
// Returns 1 if pending, 0 if nothing, -1 on error.
int   x360_poll_rumble(void* handle, int slot, uint8_t* left_out, uint8_t* right_out);

// Poll for an LED command.
// Returns animation ID (0–13) if pending, -1 if nothing, -2 on error.
int   x360_poll_led(void* handle, int slot);

// Enable (enable != 0) or disable debug logging.
void  x360_set_debug(int enable);
```

### Python (ctypes) example

```python
import ctypes

lib = ctypes.CDLL("target/release/libx360_w_raw_gadget.so")
lib.x360_open.restype = ctypes.c_void_p
lib.x360_send.restype = ctypes.c_int
lib.x360_poll_rumble.restype = ctypes.c_int
lib.x360_poll_led.restype = ctypes.c_int

h = lib.x360_open(1, b"3f980000.usb", b"3f980000.usb")

# Send a 20-byte input report
packet = (ctypes.c_uint8 * 20)(0x00, 0x14, 0x00, 0x10, *([0] * 16))
lib.x360_send(h, 0, packet, 20)

# Poll for rumble
left = ctypes.c_uint8()
right = ctypes.c_uint8()
if lib.x360_poll_rumble(h, 0, ctypes.byref(left), ctypes.byref(right)) == 1:
    print(f"rumble left={left.value} right={right.value}")

# Poll for LED
anim = lib.x360_poll_led(h, 0)
if anim >= 0:
    print(f"led animation={anim}")

lib.x360_close(h)
```

## Limitations / notes

- This is Linux raw-gadget software, so device-mode capable USB hardware is required.
- macOS support has historically been less reliable for multiple controller slots; one-slot setups are more realistic there if drivers cooperate.
- The descriptor shape is intentionally pragmatic rather than a perfect receiver clone.

## Sources

Useful references that informed the project and are still worth keeping around:

- [(partsnotincluded.com) Understanding the Xbox 360 wired controller USB data](https://www.partsnotincluded.com/understanding-the-xbox-360-wired-controllers-usb-data/)
- [raw-gadget repository](https://github.com/xairy/raw-gadget)
- [Composite USB Gadgets on the Raspberry Pi Zero](http://www.isticktoit.net/?p=1383)
- [GPIO Joystick Driver](https://github.com/recalbox/mk_arcade_joystick_rpi)
- [PicoCader](https://github.com/printnplay/PicoCader)
- [Pi Pico based controller with CircuitPython (YouTube)](https://www.youtube.com/watch?v=__QZQEOG6tA)
- [Raspberry Pi forum thread about using a controller over USB](https://forums.raspberrypi.com/viewtopic.php?t=207197)
- [USB HID descriptor tutorial](https://eleccelerator.com/tutorial-about-usb-hid-report-descriptors/)
- [Stack Overflow: emulate a gaming device on Raspberry Pi Zero](https://stackoverflow.com/questions/49139136/emulate-a-gaming-device-raspberry-pi-zero/49151408#49151408)
- [Reddit: Pi Zero as USB controller](https://www.reddit.com/r/RetroPie/comments/4vi0it/pi_zero_as_usb_controller/)
- [Reddit: Pi Zero as USB controller (raspberry_pi)](https://www.reddit.com/r/raspberry_pi/comments/4vkffh/pi_zero_as_usb_nes_controller/)

## License

MIT
