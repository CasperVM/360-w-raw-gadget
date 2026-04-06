# 360-w-raw-gadget

[![crates.io](https://img.shields.io/crates/v/x360-w-raw-gadget)](https://crates.io/crates/x360-w-raw-gadget)
[![docs.rs](https://docs.rs/x360-w-raw-gadget/badge.svg)](https://docs.rs/x360-w-raw-gadget)

Emulates an Xbox 360 wireless receiver (up to 4 controllers) on Linux via the raw-gadget kernel interface. Written in Rust; exposes a C-callable shared library for Python/ctypes integration.

Tested on Raspberry Pi Zero 2W.

## Building

```bash
# Release build (produces binary + .so)
cargo build --release

# Run tests (no hardware needed)
cargo test --lib
```

Outputs:
- `target/release/x360-w-gadget`: standalone binary
- `target/release/libx360_w_raw_gadget.so`: shared library for Python/C

## Running the Binary

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

## Requirements

- Linux with raw-gadget kernel support (`sudo modprobe raw_gadget`)
- USB OTG-capable device (Raspberry Pi Zero / Zero 2W / Pi 4 in device mode)
- Rust toolchain (1.70+)

## License

MIT
