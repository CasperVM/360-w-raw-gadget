//! C-callable FFI surface for use from Python, C, or any language with a C FFI.
//!
//! Built as `libx360_w_raw_gadget.so` when the crate is compiled with
//! `crate-type = ["cdylib"]` (the default). All symbols are prefixed `x360_`.
//!
//! ## Symbol summary
//!
//! | Symbol | Description |
//! |--------|-------------|
//! | [`x360_open`] | Open a receiver gadget, returns opaque handle |
//! | [`x360_close`] | Close a handle |
//! | [`x360_send`] | Send a raw 20-byte input report for a slot |
//! | [`x360_poll_rumble`] | Poll for a rumble command from the host |
//! | [`x360_poll_led`] | Poll for an LED animation command from the host |
//! | [`x360_set_debug`] | Enable/disable verbose debug logging |
//!
//! ## Python example
//!
//! ```python
//! import ctypes
//!
//! lib = ctypes.CDLL("target/release/libx360_w_raw_gadget.so")
//! lib.x360_open.restype    = ctypes.c_void_p
//! lib.x360_send.restype    = ctypes.c_int
//! lib.x360_poll_rumble.restype = ctypes.c_int
//! lib.x360_poll_led.restype    = ctypes.c_int
//!
//! h = lib.x360_open(1, b"3f980000.usb", b"3f980000.usb")
//!
//! # Send a 20-byte input report (A button pressed)
//! packet = (ctypes.c_uint8 * 20)(0x00, 0x14, 0x00, 0x10, *([0] * 16))
//! lib.x360_send(h, 0, packet, 20)
//!
//! # Poll for rumble
//! left, right = ctypes.c_uint8(), ctypes.c_uint8()
//! if lib.x360_poll_rumble(h, 0, ctypes.byref(left), ctypes.byref(right)) == 1:
//!     print(f"rumble left={left.value} right={right.value}")
//!
//! # Poll for LED
//! anim = lib.x360_poll_led(h, 0)
//! if anim >= 0:
//!     print(f"led animation={anim}")
//!
//! lib.x360_close(h)
//! ```
//!
//! ## Debug logging
//!
//! Set the `X360_DEBUG` environment variable or call `x360_set_debug(1)` to
//! enable verbose USB traffic logging to stderr.

use std::ffi::CStr;
use std::os::raw::{c_char, c_int};

use crate::descriptors::ConfigDescriptorSet;
use crate::session::WirelessReceiver;
use crate::transport_hw::RawGadgetTransport;

const DEFAULT_UDC: &str = "3f980000.usb";

// ---------------------------------------------------------------------------
// x360_open
// ---------------------------------------------------------------------------

/// Open a wireless receiver gadget.
///
/// `num_slots`: number of controller slots (1–4).
/// `driver` / `device`: UDC driver/device name (e.g. `"3f980000.usb"`).
///   Pass NULL to use the Pi Zero 2W default (`"3f980000.usb"`).
///
/// Returns an opaque handle on success, or NULL on failure.
#[no_mangle]
pub extern "C" fn x360_open(
    num_slots: c_int,
    driver: *const c_char,
    device: *const c_char,
) -> *mut WirelessReceiver {
    // Honour X360_DEBUG env var.
    if std::env::var_os("X360_DEBUG").is_some() {
        crate::set_debug(true);
    }

    let n = num_slots as u8;
    let config = match ConfigDescriptorSet::new(n) {
        Ok(c) => c,
        Err(_) => return std::ptr::null_mut(),
    };

    let driver_str = if driver.is_null() {
        DEFAULT_UDC
    } else {
        match unsafe { CStr::from_ptr(driver) }.to_str() {
            Ok(s) => s,
            Err(_) => return std::ptr::null_mut(),
        }
    };

    let device_str = if device.is_null() {
        DEFAULT_UDC
    } else {
        match unsafe { CStr::from_ptr(device) }.to_str() {
            Ok(s) => s,
            Err(_) => return std::ptr::null_mut(),
        }
    };

    let transport = match RawGadgetTransport::new(&config, driver_str, device_str) {
        Ok(t) => t,
        Err(_) => return std::ptr::null_mut(),
    };

    let receiver = WirelessReceiver::new(config, Box::new(transport));
    Box::into_raw(Box::new(receiver))
}

// ---------------------------------------------------------------------------
// x360_close
// ---------------------------------------------------------------------------

/// Destroy a receiver opened with `x360_open`.
#[no_mangle]
pub extern "C" fn x360_close(handle: *mut WirelessReceiver) {
    if !handle.is_null() {
        unsafe { drop(Box::from_raw(handle)) };
    }
}

// ---------------------------------------------------------------------------
// x360_send
// ---------------------------------------------------------------------------

/// Send a raw 20-byte input report for `slot` (0-based).
///
/// `data` must point to at least `len` bytes. `len` should be 20.
///
/// Returns 1 on success, 0 on error (null handle, bad length, I/O error).
#[no_mangle]
pub extern "C" fn x360_send(
    handle: *mut WirelessReceiver,
    slot: c_int,
    data: *const u8,
    len: usize,
) -> c_int {
    if handle.is_null() || data.is_null() || len == 0 {
        return 0;
    }
    let receiver = unsafe { &mut *handle };
    let bytes = unsafe { std::slice::from_raw_parts(data, len) };
    match receiver.send_raw_input(slot as u8, bytes) {
        Ok(()) => 1,
        Err(_) => 0,
    }
}

// ---------------------------------------------------------------------------
// x360_poll_rumble
// ---------------------------------------------------------------------------

/// Poll for a rumble command on `slot`.
///
/// If a rumble command is pending, writes motor values to `*left_out` and
/// `*right_out` and returns 1.
/// Returns 0 if nothing is pending.
/// Returns -1 on error (null handle or out pointers).
#[no_mangle]
pub extern "C" fn x360_poll_rumble(
    handle: *mut WirelessReceiver,
    slot: c_int,
    left_out: *mut u8,
    right_out: *mut u8,
) -> c_int {
    if handle.is_null() || left_out.is_null() || right_out.is_null() {
        return -1;
    }
    let receiver = unsafe { &mut *handle };
    match receiver.poll_rumble(slot as u8) {
        Ok(Some(r)) => {
            unsafe {
                *left_out  = r.left_motor;
                *right_out = r.right_motor;
            }
            1
        }
        Ok(None) => 0,
        Err(_)   => -1,
    }
}

// ---------------------------------------------------------------------------
// x360_poll_led
// ---------------------------------------------------------------------------

/// Poll for an LED command on `slot`.
///
/// Returns the animation ID (0–13) if a command is pending.
/// Returns -1 if nothing is pending.
/// Returns -2 on error (null handle).
#[no_mangle]
pub extern "C" fn x360_poll_led(
    handle: *mut WirelessReceiver,
    slot: c_int,
) -> c_int {
    if handle.is_null() {
        return -2;
    }
    let receiver = unsafe { &mut *handle };
    match receiver.poll_led(slot as u8) {
        Ok(Some(l)) => l.animation as c_int,
        Ok(None)    => -1,
        Err(_)      => -2,
    }
}

// ---------------------------------------------------------------------------
// x360_set_debug
// ---------------------------------------------------------------------------

/// Enable (`enable != 0`) or disable (`enable == 0`) debug logging.
///
/// When enabled, verbose internal logs are printed to stderr.
#[no_mangle]
pub extern "C" fn x360_set_debug(enable: c_int) {
    crate::set_debug(enable != 0);
}
