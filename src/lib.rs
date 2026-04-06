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

// ---------------------------------------------------------------------------
// Global debug flag. Gated by X360_DEBUG env var or x360_set_debug() C call
// ---------------------------------------------------------------------------

pub static DEBUG: AtomicBool = AtomicBool::new(false);

pub fn set_debug(enable: bool) {
    DEBUG.store(enable, Ordering::Relaxed);
}

/// Internal logging macro: prints only when DEBUG is set.
#[macro_export]
macro_rules! dbg_log {
    ($($arg:tt)*) => {
        if $crate::DEBUG.load(std::sync::atomic::Ordering::Relaxed) {
            eprintln!($($arg)*);
        }
    };
}
