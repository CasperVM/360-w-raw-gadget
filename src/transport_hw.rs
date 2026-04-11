//! Hardware [`Transport`](crate::Transport) implementation using raw-gadget ioctls.
//!
//! [`RawGadgetTransport`] is the production backend. It opens
//! `/dev/raw-gadget`, performs USB enumeration, and manages three layers of
//! threads so that all blocking I/O stays off the caller's thread:
//!
//! | Thread | Responsibility |
//! |--------|----------------|
//! | **ep0** | Blocks on `EVENT_FETCH`; handles `GET_DESCRIPTOR`, `SET_CONFIGURATION`, vendor requests |
//! | **ep_out × N** | One per controller slot; blocks on `EP_READ` for rumble/LED output reports |
//! | **caller** | Calls `EP_WRITE` to send input reports; polls output via `mpsc` channel |
//!
//! The kernel's raw-gadget driver does not support `O_NONBLOCK` on ioctls, so
//! all blocking calls are isolated in the background threads above and
//! communicate with the main thread via channels and a condvar for the
//! ready-signal.
//!
//! # Requirements
//!
//! - Linux kernel with `raw_gadget` module loaded (`sudo modprobe raw_gadget`)
//! - USB OTG-capable hardware (Raspberry Pi Zero / Zero 2W / Pi 4 device mode)
//! - Process must have permission to open `/dev/raw-gadget` (typically `root`)

use std::io;
use std::mem::size_of;
use std::sync::{Arc, Condvar, Mutex};
use std::sync::mpsc;
use std::thread::{self, JoinHandle};
use std::time::Duration;

use std::os::fd::RawFd;

use crate::descriptors::ConfigDescriptorSet;
use crate::ffi::*;
use crate::protocol::{InputReport, LedReport, OutputReport, RumbleReport, INPUT_REPORT_LEN};
use crate::session::Transport;

// ---------------------------------------------------------------------------
// Shared state between ep0 thread and main transport
// ---------------------------------------------------------------------------

struct SharedState {
    ep_in: [Option<i32>; 4],  // ep handle per slot (from EP_ENABLE return value)
    ep_out: [Option<i32>; 4],
    ready: bool,               // true after SET_CONFIGURATION completes
}

impl SharedState {
    fn new() -> Self {
        Self {
            ep_in: [None; 4],
            ep_out: [None; 4],
            ready: false,
        }
    }

    fn clear_endpoints(&mut self) {
        self.ep_in = [None; 4];
        self.ep_out = [None; 4];
        self.ready = false;
    }
}

// ---------------------------------------------------------------------------
// RawGadgetTransport
// ---------------------------------------------------------------------------

/// Production [`Transport`](crate::Transport) that drives a real USB OTG port via raw-gadget.
///
/// Create with [`RawGadgetTransport::new`], then pass to [`WirelessReceiver::new`](crate::WirelessReceiver::new).
/// The transport self-manages enumeration in background threads; the caller
/// only needs to call [`send_input`](crate::WirelessReceiver::send_input) and
/// [`poll_rumble`](crate::WirelessReceiver::poll_rumble) /
/// [`poll_led`](crate::WirelessReceiver::poll_led) each tick.
///
/// Dropping the transport cleanly shuts down all background threads.
pub struct RawGadgetTransport {
    fd: RawFd,
    _num_interfaces: u8,
    state: Arc<(Mutex<SharedState>, Condvar)>,
    ep0_thread: Option<JoinHandle<()>>,
    output_rx: mpsc::Receiver<(u8, OutputReport)>,
    rumble_buf: [Option<RumbleReport>; 4],
    led_buf: [Option<LedReport>; 4],
}

impl RawGadgetTransport {
    /// Create a new transport, bind to the UDC, and start the ep0 thread.
    /// `driver` and `device` are the UDC driver/device names, e.g. "20980000.usb" for Pi Zero.
    pub fn new(
        config: &ConfigDescriptorSet,
        driver: &str,
        device: &str,
    ) -> io::Result<Self> {
        let fd = unsafe { raw_gadget_open_nonblock()? };

        let init = UsbRawInit::new(driver, device, USB_SPEED_HIGH);
        unsafe { raw_gadget_init(fd, &init)? };
        unsafe { raw_gadget_run(fd)? };

        let num_interfaces = config.num_interfaces;
        let state = Arc::new((Mutex::new(SharedState::new()), Condvar::new()));

        // Build descriptor bytes once, hand to ep0 thread
        let desc_bytes = build_descriptor_bytes(config);

        let (output_tx, output_rx) = mpsc::channel();

        let state_clone = Arc::clone(&state);
        let ep0_thread = thread::Builder::new()
            .name("ep0".into())
            .spawn(move || ep0_loop(fd, num_interfaces, desc_bytes, state_clone, output_tx))?;

        Ok(Self {
            fd,
            _num_interfaces: num_interfaces,
            state,
            ep0_thread: Some(ep0_thread),
            output_rx,
            rumble_buf: [None, None, None, None],
            led_buf:    [None, None, None, None],
        })
    }

    /// Block until SET_CONFIGURATION has completed (endpoints are active).
    pub fn wait_ready(&self) {
        let (lock, cvar) = &*self.state;
        let mut state = lock.lock().unwrap();
        while !state.ready {
            state = cvar.wait(state).unwrap();
        }
    }

    /// Drain everything the ep_out threads have sent into the split per-slot buffers.
    /// Newer packets overwrite older ones of the same type on the same slot.
    fn drain_output_channel(&mut self) {
        loop {
            match self.output_rx.try_recv() {
                Ok((slot, report)) if (slot as usize) < self.rumble_buf.len() => {
                    match report {
                        OutputReport::Rumble(r) => self.rumble_buf[slot as usize] = Some(r),
                        OutputReport::Led(l)    => self.led_buf[slot as usize]    = Some(l),
                    }
                }
                Ok(_) => {}
                Err(mpsc::TryRecvError::Empty) => break,
                Err(mpsc::TryRecvError::Disconnected) => break,
            }
        }
    }

    fn ep_in_handle(&self, slot: u8) -> Option<i32> {
        let (lock, _) = &*self.state;
        lock.lock().unwrap().ep_in[slot as usize]
    }

}

impl Drop for RawGadgetTransport {
    fn drop(&mut self) {
        // Closing the fd causes the ep0 thread's EVENT_FETCH to return EBADF,
        // which causes the thread to exit its loop cleanly.
        unsafe { raw_gadget_close(self.fd) };
        if let Some(t) = self.ep0_thread.take() {
            let _ = t.join();
        }
    }
}

impl Transport for RawGadgetTransport {
    fn write_descriptor(&mut self, _data: &[u8]) -> Result<(), Box<dyn std::error::Error>> {
        // no-op: descriptors are handled entirely by the ep0 thread
        Ok(())
    }

    fn write_input_report(
        &mut self,
        interface: u8,
        report: &InputReport,
    ) -> Result<(), Box<dyn std::error::Error>> {
        let ep = match self.ep_in_handle(interface) {
            Some(h) => h,
            None => return Ok(()), // not ready yet, drop silently
        };

        let mut io = UsbRawInterruptEpIo::zeroed();
        io.inner.ep = ep as u16;
        io.inner.flags = 0;
        io.inner.length = INPUT_REPORT_LEN as u32;
        io.data[..INPUT_REPORT_LEN].copy_from_slice(report.as_bytes());

        unsafe { raw_gadget_ep_write(self.fd, &io)? };
        Ok(())
    }

    fn poll_rumble(
        &mut self,
        interface: u8,
    ) -> Result<Option<RumbleReport>, Box<dyn std::error::Error>> {
        self.drain_output_channel();
        Ok(self.rumble_buf[interface as usize].take())
    }

    fn poll_led(
        &mut self,
        interface: u8,
    ) -> Result<Option<LedReport>, Box<dyn std::error::Error>> {
        self.drain_output_channel();
        Ok(self.led_buf[interface as usize].take())
    }
}

// ---------------------------------------------------------------------------
// ep0 thread
// ---------------------------------------------------------------------------

fn ep0_loop(
    fd: RawFd,
    num_interfaces: u8,
    desc_bytes: DescriptorBytes,
    state: Arc<(Mutex<SharedState>, Condvar)>,
    output_tx: mpsc::Sender<(u8, OutputReport)>,
) {
    loop {
        let mut event = UsbRawControlEvent::zeroed();
        // Tell the kernel how much space we have for the ctrl payload
        event.event.length = size_of::<UsbCtrlrequest>() as u32;

        if let Err(e) = unsafe { raw_gadget_event_fetch(fd, &mut event) } {
            let errno = e.raw_os_error().unwrap_or(0);
            if errno == libc::EAGAIN {
                thread::sleep(Duration::from_millis(1));
                continue;
            }
            // EBADF / ENODEV = fd closed, exit cleanly
            break;
        }

        match event.event.typ {
            USB_RAW_EVENT_CONNECT => {
                // UDC bound, nothing to do yet
            }
            USB_RAW_EVENT_RESET | USB_RAW_EVENT_DISCONNECT => {
                let (lock, _) = &*state;
                lock.lock().unwrap().clear_endpoints();
            }
            USB_RAW_EVENT_CONTROL => {
                crate::dbg_log!("[ep0] CONTROL bRequestType={:#04x} bRequest={:#04x} wValue={:#06x} wLength={}",
                    event.ctrl.b_request_type, event.ctrl.b_request,
                    u16::from_le(event.ctrl.w_value), u16::from_le(event.ctrl.w_length));
                handle_control(fd, num_interfaces, &event.ctrl, &desc_bytes, &state, &output_tx);
            }
            _ => {}
        }
    }
}

fn handle_control(
    fd: RawFd,
    num_interfaces: u8,
    ctrl: &UsbCtrlrequest,
    desc: &DescriptorBytes,
    state: &Arc<(Mutex<SharedState>, Condvar)>,
    output_tx: &mpsc::Sender<(u8, OutputReport)>,
) {
    let req_type = ctrl.b_request_type;
    let req = ctrl.b_request;
    let value = u16::from_le(ctrl.w_value);
    let length = u16::from_le(ctrl.w_length) as usize;

    let is_in = (req_type & USB_DIR_IN) != 0;
    let type_bits = req_type & USB_TYPE_MASK;

    if type_bits == USB_TYPE_VENDOR {
        if is_in {
            // Xbox vendor IN: send zero-filled response of requested length.
            // xpad polls this repeatedly (presence detection); respond with zeros.
            let send_len = length.min(16384);
            let mut io = UsbRawControlEpIo::zeroed();
            io.inner.length = send_len as u32;
            let _ = unsafe { raw_gadget_ep0_write(fd, &io) };
        } else {
            // Xbox vendor OUT: ack with zero-length read
            let mut io = UsbRawControlEpIo::zeroed();
            io.inner.length = 0;
            let _ = unsafe { raw_gadget_ep0_read(fd, &mut io) };
        }
        return;
    }

    if type_bits != USB_TYPE_STANDARD {
        let _ = unsafe { raw_gadget_ep0_stall(fd) };
        return;
    }

    match req {
        USB_REQ_GET_DESCRIPTOR => {
            let desc_type = (value >> 8) as u8;
            let desc_index = (value & 0xFF) as u8;

            let data: Option<&[u8]> = match desc_type {
                USB_DT_DEVICE => Some(&desc.device),
                USB_DT_DEVICE_QUALIFIER => Some(&desc.qualifier),
                USB_DT_CONFIG => Some(&desc.config),
                USB_DT_STRING => match desc_index {
                    0 => Some(&desc.string_lang),
                    1 => desc.string_manufacturer.as_deref(),
                    2 => desc.string_product.as_deref(),
                    3 => desc.string_serial.as_deref(),
                    _ => None,
                },
                _ => None,
            };

            match data {
                Some(bytes) => {
                    let send_len = bytes.len().min(length);
                    let mut io = UsbRawControlEpIo::zeroed();
                    io.inner.length = send_len as u32;
                    io.data[..send_len].copy_from_slice(&bytes[..send_len]);
                    let _ = unsafe { raw_gadget_ep0_write(fd, &io) };
                }
                None => {
                    let _ = unsafe { raw_gadget_ep0_stall(fd) };
                }
            }
        }

        USB_REQ_SET_CONFIGURATION => {
            let (lock, cvar) = &**state;
            let mut st = lock.lock().unwrap();

            for i in 0..num_interfaces as usize {
                let ep_in_addr = USB_DIR_IN | (i as u8 + 1);
                let ep_out_addr = i as u8 + 1;

                let ep_in_desc = UsbEndpointDescriptor::interrupt(ep_in_addr, 0x0020, 0x04);
                let ep_out_desc = UsbEndpointDescriptor::interrupt(ep_out_addr, 0x0020, 0x08);

                match unsafe { raw_gadget_ep_enable(fd, &ep_in_desc) } {
                    Ok(h) => {
                        crate::dbg_log!("[ep0] ep_enable IN[{}] -> handle {}", i, h);
                        st.ep_in[i] = Some(h);
                    }
                    Err(e) => eprintln!("[ep0] ep_enable IN[{}] failed: {}", i, e),
                }
                match unsafe { raw_gadget_ep_enable(fd, &ep_out_desc) } {
                    Ok(h) => {
                        crate::dbg_log!("[ep0] ep_enable OUT[{}] -> handle {}", i, h);
                        st.ep_out[i] = Some(h);
                        // Spawn a thread to block-read this OUT endpoint and forward reports.
                        let tx = output_tx.clone();
                        let slot = i as u8;
                        let ep_handle = h;
                        thread::Builder::new()
                            .name(format!("ep_out_{}", i))
                            .spawn(move || ep_out_loop(fd, slot, ep_handle, tx))
                            .ok();
                    }
                    Err(e) => eprintln!("[ep0] ep_enable OUT[{}] failed: {}", i, e),
                }
            }

            let _ = unsafe { raw_gadget_vbus_draw(fd, 0xFA) };
            let _ = unsafe { raw_gadget_configure(fd) };

            // OUT request: ack with ep0_read (zero-length), not ep0_write.
            let mut io = UsbRawControlEpIo::zeroed();
            io.inner.length = 0;
            let r = unsafe { raw_gadget_ep0_read(fd, &mut io) };
            crate::dbg_log!("[ep0] SET_CONFIGURATION ack: {:?}", r);

            st.ready = true;
            cvar.notify_all();
        }

        USB_REQ_GET_INTERFACE => {
            let mut io = UsbRawControlEpIo::zeroed();
            io.inner.length = 1;
            io.data[0] = 0; // alternate setting 0
            let _ = unsafe { raw_gadget_ep0_write(fd, &io) };
        }

        _ => {
            let _ = unsafe { raw_gadget_ep0_stall(fd) };
        }
    }
}

// ---------------------------------------------------------------------------
// Per-slot OUT endpoint reader thread
// ---------------------------------------------------------------------------

fn ep_out_loop(fd: RawFd, slot: u8, ep_handle: i32, tx: mpsc::Sender<(u8, OutputReport)>) {
    crate::dbg_log!("[ep_out {}] thread started (handle={})", slot, ep_handle);
    loop {
        let mut io = UsbRawInterruptEpIo::zeroed();
        io.inner.ep = ep_handle as u16;
        io.inner.flags = 0;
        io.inner.length = 32;

        match unsafe { raw_gadget_ep_read(fd, &mut io) } {
            Ok(n) if n > 0 => {
                let n = n as usize;
                let bytes = &io.data[..n];
                crate::dbg_log!("[ep_out {}] {} bytes: {:02x?}", slot, n, bytes);
                match OutputReport::from_bytes(bytes) {
                    Some(report) => {
                        if tx.send((slot, report)).is_err() {
                            eprintln!("[ep_out {}] channel closed, exiting", slot);
                            break;
                        }
                    }
                    None => {
                        crate::dbg_log!("[ep_out {}] unrecognized packet (first byte={:#04x})", slot, bytes[0]);
                    }
                }
            }
            Ok(0) => {
                crate::dbg_log!("[ep_out {}] zero-length read", slot);
            }
            Ok(n) => {
                crate::dbg_log!("[ep_out {}] unexpected return value: {}", slot, n);
            }
            Err(e) => {
                let errno = e.raw_os_error().unwrap_or(0);
                if errno == libc::EINTR {
                    continue;
                }
                eprintln!("[ep_out {}] read error: {} (errno {}), exiting", slot, e, errno);
                break;
            }
        }
    }
    crate::dbg_log!("[ep_out {}] thread exiting", slot);
}

// ---------------------------------------------------------------------------
// USB descriptor byte builder
// ---------------------------------------------------------------------------

struct DescriptorBytes {
    device: Vec<u8>,
    qualifier: Vec<u8>,
    config: Vec<u8>,
    string_lang: Vec<u8>,
    string_manufacturer: Option<Vec<u8>>,
    string_product: Option<Vec<u8>>,
    string_serial: Option<Vec<u8>>,
}

fn build_descriptor_bytes(config: &ConfigDescriptorSet) -> DescriptorBytes {
    let n = config.num_interfaces as usize;

    // --- Device descriptor (18 bytes) ---
    let vid = config.identity.vendor_id.to_le_bytes();
    let pid = config.identity.product_id.to_le_bytes();
    #[rustfmt::skip]
    let device = vec![
        0x12,       // bLength
        0x01,       // bDescriptorType: DEVICE
        0x00, 0x02, // bcdUSB: 2.0 (LE)
        0xFF,       // bDeviceClass: vendor-specific
        0xFF,       // bDeviceSubClass
        0xFF,       // bDeviceProtocol
        0x40,       // bMaxPacketSize0: 64
        vid[0], vid[1], // idVendor (LE)
        pid[0], pid[1], // idProduct (LE)
        0x00, 0x00, // bcdDevice: 0
        0x01,       // iManufacturer
        0x02,       // iProduct
        0x03,       // iSerialNumber
        0x01,       // bNumConfigurations
    ];

    // --- Device qualifier (10 bytes) ---
    #[rustfmt::skip]
    let qualifier = vec![
        0x0A,       // bLength
        0x06,       // bDescriptorType: DEVICE_QUALIFIER
        0x00, 0x02, // bcdUSB: 2.0 (LE)
        0x00,       // bDeviceClass
        0x00,       // bDeviceSubClass
        0x00,       // bDeviceProtocol
        0x40,       // bMaxPacketSize0: 64
        0x01,       // bNumConfigurations
        0x00,       // bRESERVED
    ];

    // --- Configuration descriptor (9 + n*43 bytes) ---
    let total_len = (9 + n * 43) as u16;
    let tl = total_len.to_le_bytes();
    let mut cfg = vec![
        0x09,       // bLength
        0x02,       // bDescriptorType: CONFIG
        tl[0], tl[1], // wTotalLength (LE)
        n as u8,    // bNumInterfaces
        0x01,       // bConfigurationValue
        0x00,       // iConfiguration
        0xA0,       // bmAttributes: bus-powered, remote wakeup
        0xFA,       // bMaxPower: 500 mA
    ];

    for i in 0..n {
        let ep_in_addr = 0x80u8 | (i as u8 + 1);
        let ep_out_addr = i as u8 + 1;

        // Interface descriptor (9 bytes)
        cfg.extend_from_slice(&[
            0x09, 0x04,   // bLength, bDescriptorType: INTERFACE
            i as u8,      // bInterfaceNumber
            0x00,         // bAlternateSetting
            0x02,         // bNumEndpoints
            0xFF,         // bInterfaceClass: vendor-specific
            0x5D,         // bInterfaceSubClass
            0x01,         // bInterfaceProtocol
            0x00,         // iInterface
        ]);

        // Custom Xbox vendor descriptor (20 bytes): if_unknown_desc_control_surface
        cfg.extend_from_slice(&[
            0x14, 0x22,   // bLength=20, bDescriptorType=0x22
            0x00, 0x01, 0x13,
            ep_in_addr,   // bEndpointAddressIn
            0x1D, 0x00, 0x17, 0x01, 0x02, 0x08, 0x13,
            ep_out_addr,  // bEndpointAddressOut
            0x0C, 0x00, 0x0C, 0x01, 0x02, 0x08,
        ]);

        // EP IN descriptor (7 bytes)
        let mp = 0x0020u16.to_le_bytes();
        cfg.extend_from_slice(&[
            0x07, 0x05,   // bLength=7, bDescriptorType: ENDPOINT
            ep_in_addr,   // bEndpointAddress
            0x03,         // bmAttributes: INT
            mp[0], mp[1], // wMaxPacketSize: 32 (LE)
            0x04,         // bInterval
        ]);

        // EP OUT descriptor (7 bytes)
        cfg.extend_from_slice(&[
            0x07, 0x05,
            ep_out_addr,
            0x03,
            mp[0], mp[1],
            0x08,         // bInterval (OUT has longer interval)
        ]);
    }

    // --- String descriptors ---
    // String 0: language descriptor (English US = 0x0409)
    let string_lang = vec![0x04, 0x03, 0x09, 0x04];

    DescriptorBytes {
        device,
        qualifier,
        config: cfg,
        string_lang,
        string_manufacturer: config.strings.get(&1).map(|s| encode_string_descriptor(s)),
        string_product: config.strings.get(&2).map(|s| encode_string_descriptor(s)),
        string_serial: config.strings.get(&3).map(|s| encode_string_descriptor(s)),
    }
}

/// Encode a Rust string as a USB string descriptor (UTF-16LE with 2-byte header).
fn encode_string_descriptor(s: &str) -> Vec<u8> {
    let utf16: Vec<u16> = s.encode_utf16().collect();
    let byte_len = 2 + utf16.len() * 2;
    let mut out = Vec::with_capacity(byte_len);
    out.push(byte_len as u8);
    out.push(0x03); // bDescriptorType: STRING
    for ch in utf16 {
        let b = ch.to_le_bytes();
        out.push(b[0]);
        out.push(b[1]);
    }
    out
}
