//! # x360-w-raw-gadget
//!
//! Emulates an **Xbox 360 wireless receiver** (up to 4 controllers) on Linux
//! via the [`raw-gadget`](https://docs.kernel.org/usb/raw-gadget.html) kernel
//! interface.
//!
//! The crate is split into two layers:
//!
//! - **Portable core** — pure Rust, no kernel dependency, fully unit-testable
//!   on any Linux machine without USB hardware.
//! - **Hardware transport** (`transport_hw`, Linux only) — drives the actual
//!   `/dev/raw-gadget` device on a USB OTG capable board (e.g. Raspberry Pi
//!   Zero 2W).
//!
//! A **C-callable shared library** (`capi`, Linux only) is also built
//! (`libx360_w_raw_gadget.so`) for use from Python/ctypes or any other
//! language with a C FFI.
//!
//! ## Quick start — Rust
//!
//! ```rust,no_run
//! use x360_w_raw_gadget::{Button, ConfigDescriptorSet, InputState, WirelessReceiver};
//! # #[cfg(target_os = "linux")]
//! use x360_w_raw_gadget::RawGadgetTransport;
//!
//! # #[cfg(target_os = "linux")]
//! # fn main() -> Result<(), Box<dyn std::error::Error>> {
//! let config = ConfigDescriptorSet::new(1)?;
//! let transport = RawGadgetTransport::new(&config, "3f980000.usb", "3f980000.usb")?;
//! let mut receiver = WirelessReceiver::new(config, Box::new(transport));
//!
//! let state = InputState::default()
//!     .with_button(Button::A, true)
//!     .with_left_stick(-16000, 8000)
//!     .with_left_trigger(128);
//!
//! receiver.slot_mut(0).unwrap().set_state(state);
//! receiver.send_input(0)?;
//!
//! if let Some(r) = receiver.poll_rumble(0)? {
//!     println!("rumble left={} right={}", r.left_motor, r.right_motor);
//! }
//! if let Some(l) = receiver.poll_led(0)? {
//!     println!("led {:?}", l.animation);
//! }
//! # Ok(())
//! # }
//! # #[cfg(not(target_os = "linux"))]
//! # fn main() {}
//! ```
//!
//! ## Testing without hardware
//!
//! The core is fully testable using [`MockTransport`](session::MockTransport):
//!
//! ```rust
//! use x360_w_raw_gadget::{
//!     Button, ConfigDescriptorSet, InputState, WirelessReceiver,
//!     session::MockTransport,
//!     protocol::{OutputReport, RumbleReport},
//! };
//!
//! let config = ConfigDescriptorSet::new(1).unwrap();
//! let mut transport = MockTransport::new();
//! transport.queue_output(0, OutputReport::Rumble(RumbleReport { left_motor: 128, right_motor: 64 }));
//!
//! let mut receiver = WirelessReceiver::new(config, Box::new(transport));
//! receiver.slot_mut(0).unwrap().set_state(
//!     InputState::default().with_button(Button::A, true)
//! );
//! receiver.send_input(0).unwrap();
//!
//! let rumble = receiver.poll_rumble(0).unwrap();
//! assert!(rumble.is_some());
//! ```
//!
//! ## Modules
//!
//! | Module | Contents |
//! |--------|----------|
//! | [`controller`] | [`InputState`], [`Button`], [`ControllerSlot`] |
//! | [`protocol`] | [`InputReport`], [`RumbleReport`], [`LedReport`], [`LedAnimation`], [`OutputReport`] |
//! | [`descriptors`] | [`ConfigDescriptorSet`], [`ReceiverIdentity`] |
//! | [`session`] | [`WirelessReceiver`], [`Transport`], [`MockTransport`](session::MockTransport) |
//! | `transport_hw` *(Linux only)* | [`RawGadgetTransport`] |
//! | `capi` *(Linux only)* | C FFI symbols (`x360_open`, `x360_send`, …) |

use std::sync::atomic::{AtomicBool, Ordering};

pub mod controller;
pub mod descriptors;
pub mod ffi;
pub mod protocol;
pub mod session;
#[cfg(target_os = "linux")]
pub mod transport_hw;
#[cfg(target_os = "linux")]
pub mod capi;

pub use controller::{Button, ControllerSlot, InputState};
pub use descriptors::{ConfigDescriptorSet, DescriptorError, ReceiverIdentity, StringDescriptorMap};
pub use protocol::{InputReport, LedAnimation, LedReport, OutputReport, RumbleReport};
pub use session::{HostCommand, Transport, WirelessReceiver};
#[cfg(target_os = "linux")]
pub use transport_hw::RawGadgetTransport;

/// Global debug flag. When `true`, the hardware transport prints verbose logs
/// to stderr. Set via [`set_debug`], the `--debug` CLI flag, or the
/// `X360_DEBUG` environment variable.
pub static DEBUG: AtomicBool = AtomicBool::new(false);

/// Enable or disable verbose debug logging to stderr.
///
/// Equivalent to setting the `X360_DEBUG` environment variable or passing
/// `--debug` to the `x360-w-gadget` binary.
pub fn set_debug(enable: bool) {
    DEBUG.store(enable, Ordering::Relaxed);
}

/// Internal logging macro: prints only when [`DEBUG`] is set.
#[macro_export]
macro_rules! dbg_log {
    ($($arg:tt)*) => {
        if $crate::DEBUG.load(std::sync::atomic::Ordering::Relaxed) {
            eprintln!($($arg)*);
        }
    };
}
