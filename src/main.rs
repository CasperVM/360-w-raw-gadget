//! Standalone binary: `x360-w-gadget`
//!
//! Enumerates a USB Xbox 360 wireless receiver on the local raw-gadget device
//! and prints any rumble/LED commands the host sends back.
//!
//! # Usage
//!
//! ```text
//! sudo x360-w-gadget [num_interfaces] [driver] [device] [--demo] [--debug]
//! ```
//!
//! | Argument | Default | Description |
//! |---|---|---|
//! | `num_interfaces` | `4` | Controller slots to expose (1–4) |
//! | `driver` | `3f980000.usb` | UDC driver name |
//! | `device` | same as driver | UDC device name |
//! | `--demo` | off | Toggle A button on all slots every second |
//! | `--debug` | off | Print verbose USB traffic to stderr |
//!
//! Debug logging can also be enabled by setting the `X360_DEBUG` environment
//! variable.

use std::env;
use std::time::Duration;
use std::thread;

use x360_w_raw_gadget::{
    Button, ConfigDescriptorSet, InputState, OutputReport, WirelessReceiver,
    set_debug,
    transport_hw::RawGadgetTransport,
};

fn main() {
    let args: Vec<String> = env::args().collect();

    let demo_mode  = args.iter().any(|a| a == "--demo");
    let debug_mode = args.iter().any(|a| a == "--debug");

    if debug_mode {
        set_debug(true);
    }

    let positional: Vec<&str> = args[1..].iter()
        .filter(|a| !a.starts_with('-'))
        .map(|s| s.as_str())
        .collect();

    let num_interfaces: u8 = positional.get(0)
        .and_then(|s| s.parse().ok())
        .unwrap_or(4);

    let driver = positional.get(1).copied().unwrap_or("3f980000.usb");
    let device = positional.get(2).copied().unwrap_or(driver);

    let config = ConfigDescriptorSet::new(num_interfaces)
        .expect("invalid interface count (must be 1-4)");

    eprintln!("Opening raw-gadget ({} interface(s), driver={}, device={}{}{})",
        num_interfaces, driver, device,
        if demo_mode  { ", demo"  } else { "" },
        if debug_mode { ", debug" } else { "" });

    let transport = RawGadgetTransport::new(&config, driver, device)
        .expect("failed to open /dev/raw-gadget (run as root?)");

    let mut receiver = WirelessReceiver::new(config, Box::new(transport));

    eprintln!("Waiting for host to enumerate (plug in USB cable)...");

    let mut tick: u32 = 0;
    let mut a_pressed = false;

    loop {
        thread::sleep(Duration::from_millis(100));
        tick += 1;

        if demo_mode && tick % 10 == 0 {
            a_pressed = !a_pressed;
            let state = InputState::default().with_button(Button::A, a_pressed);
            for slot in 0..num_interfaces {
                if let Some(s) = receiver.slot_mut(slot) {
                    s.set_state(state.clone());
                }
            }
            eprintln!("[demo] all slots A button: {}", if a_pressed { "DOWN" } else { "UP" });
        }

        for slot in 0..num_interfaces {
            let _ = receiver.send_input(slot);
        }

        for slot in 0..num_interfaces {
            loop {
                match receiver.poll_output(slot) {
                    Ok(Some(OutputReport::Rumble(r))) => {
                        println!("[slot {}] rumble  left={:3}  right={:3}", slot, r.left_motor, r.right_motor);
                    }
                    Ok(Some(OutputReport::Led(l))) => {
                        println!("[slot {}] led     {:?}", slot, l.animation);
                    }
                    Ok(None) => break,
                    Err(e) => { eprintln!("[slot {}] poll error: {}", slot, e); break; }
                }
            }
        }
    }
}
