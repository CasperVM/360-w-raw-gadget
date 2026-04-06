// Raw bindings to the Linux raw-gadget kernel interface.
// Structs and ioctl numbers match <linux/usb/raw_gadget.h>.
// All structs are #[repr(C)] to match kernel ABI.

use std::io;
use std::os::fd::RawFd;

// ---------------------------------------------------------------------------
// ioctl numbers
// Computed from the Linux _IO/_IOW/_IOR/_IOWR macros, magic byte 'U' = 0x55.
// Formula (x86_64 and ARM match):
//   _IO(type, nr)         = (type << 8) | nr
//   _IOW(type, nr, size)  = 0x40000000 | (size << 16) | (type << 8) | nr
//   _IOR(type, nr, size)  = 0x80000000 | (size << 16) | (type << 8) | nr
//   _IOWR(type, nr, size) = 0xC0000000 | (size << 16) | (type << 8) | nr
//
// Struct sizes used in encoding (fixed header only, no flex arrays):
//   usb_raw_init              = 257  (128 + 128 + 1)
//   usb_raw_event (header)    = 8    (u32 + u32)
//   usb_raw_ep_io (header)    = 8    (u16 + u16 + u32)
//   usb_endpoint_descriptor   = 9    (packed, including audio extension bytes)
//   __u32                     = 4
// ---------------------------------------------------------------------------

pub const USB_RAW_IOCTL_INIT:        u32 = 0x41015500; // _IOW('U', 0, usb_raw_init=257)
pub const USB_RAW_IOCTL_RUN:         u32 = 0x00005501; // _IO('U', 1)
pub const USB_RAW_IOCTL_EVENT_FETCH: u32 = 0x80085502; // _IOR('U', 2, usb_raw_event=8)
pub const USB_RAW_IOCTL_EP0_WRITE:   u32 = 0x40085503; // _IOW('U', 3, usb_raw_ep_io=8)
pub const USB_RAW_IOCTL_EP0_READ:    u32 = 0xC0085504; // _IOWR('U', 4, usb_raw_ep_io=8)
pub const USB_RAW_IOCTL_EP_ENABLE:   u32 = 0x40095505; // _IOW('U', 5, usb_endpoint_descriptor=9)
pub const USB_RAW_IOCTL_EP_DISABLE:  u32 = 0x40045506; // _IOW('U', 6, __u32=4)
pub const USB_RAW_IOCTL_EP_WRITE:    u32 = 0x40085507; // _IOW('U', 7, usb_raw_ep_io=8)
pub const USB_RAW_IOCTL_EP_READ:     u32 = 0xC0085508; // _IOWR('U', 8, usb_raw_ep_io=8)
pub const USB_RAW_IOCTL_CONFIGURE:   u32 = 0x00005509; // _IO('U', 9)
pub const USB_RAW_IOCTL_VBUS_DRAW:   u32 = 0x4004550A; // _IOW('U', 10, __u32=4)
pub const USB_RAW_IOCTL_EP0_STALL:   u32 = 0x0000550C; // _IO('U', 12)

// ---------------------------------------------------------------------------
// USB speed constants (enum usb_device_speed in <linux/usb/ch9.h>)
// ---------------------------------------------------------------------------
pub const USB_SPEED_HIGH: u8 = 3;

// ---------------------------------------------------------------------------
// USB event types (enum usb_raw_event_type in raw_gadget.h)
// ---------------------------------------------------------------------------
pub const USB_RAW_EVENT_CONNECT:    u32 = 1;
pub const USB_RAW_EVENT_CONTROL:    u32 = 2;
pub const USB_RAW_EVENT_SUSPEND:    u32 = 3;
pub const USB_RAW_EVENT_RESUME:     u32 = 4;
pub const USB_RAW_EVENT_RESET:      u32 = 5;
pub const USB_RAW_EVENT_DISCONNECT: u32 = 6;

// ---------------------------------------------------------------------------
// USB standard request constants (from <linux/usb/ch9.h>)
// ---------------------------------------------------------------------------
pub const USB_DIR_IN:  u8 = 0x80;
pub const USB_DIR_OUT: u8 = 0x00;

pub const USB_TYPE_STANDARD: u8 = 0x00;
pub const USB_TYPE_VENDOR:   u8 = 0x40;
pub const USB_TYPE_MASK:     u8 = 0x60;

pub const USB_REQ_GET_DESCRIPTOR:    u8 = 0x06;
pub const USB_REQ_SET_CONFIGURATION: u8 = 0x09;
pub const USB_REQ_GET_INTERFACE:     u8 = 0x0A;

pub const USB_DT_DEVICE:           u8 = 0x01;
pub const USB_DT_CONFIG:           u8 = 0x02;
pub const USB_DT_STRING:           u8 = 0x03;
pub const USB_DT_DEVICE_QUALIFIER: u8 = 0x06;

pub const USB_ENDPOINT_XFER_INT: u8 = 0x03;

// ---------------------------------------------------------------------------
// Structs
// ---------------------------------------------------------------------------

/// Argument for USB_RAW_IOCTL_INIT.
/// Field order matches kernel: driver_name, device_name, speed.
/// UDC_NAME_LENGTH_MAX = 128.
#[repr(C)]
pub struct UsbRawInit {
    pub driver_name: [u8; 128],
    pub device_name: [u8; 128],
    pub speed: u8,
}

impl UsbRawInit {
    pub fn new(driver: &str, device: &str, speed: u8) -> Self {
        let mut s = Self {
            driver_name: [0u8; 128],
            device_name: [0u8; 128],
            speed,
        };
        let d = driver.as_bytes();
        s.driver_name[..d.len().min(127)].copy_from_slice(&d[..d.len().min(127)]);
        let v = device.as_bytes();
        s.device_name[..v.len().min(127)].copy_from_slice(&v[..v.len().min(127)]);
        s
    }
}

/// Fixed header of struct usb_raw_event (8 bytes).
/// The flex array `data[]` is handled by using UsbRawControlEvent for CONTROL events.
#[repr(C)]
pub struct UsbRawEvent {
    pub typ: u32,
    pub length: u32,
}

/// struct usb_ctrlrequest from <linux/usb/ch9.h> (8 bytes).
#[repr(C)]
pub struct UsbCtrlrequest {
    pub b_request_type: u8,
    pub b_request: u8,
    pub w_value: u16, // little-endian
    pub w_index: u16, // little-endian
    pub w_length: u16, // little-endian
}

/// EVENT_FETCH result for CONTROL events: event header + ctrlrequest payload.
#[repr(C)]
pub struct UsbRawControlEvent {
    pub event: UsbRawEvent,
    pub ctrl: UsbCtrlrequest,
}

impl UsbRawControlEvent {
    pub fn zeroed() -> Self {
        // SAFETY: all-zeros is valid for these plain-data structs
        unsafe { std::mem::zeroed() }
    }
}

/// Fixed header of struct usb_raw_ep_io (8 bytes).
/// The flex array `data[]` is appended by the sized wrappers below.
#[repr(C)]
pub struct UsbRawEpIo {
    pub ep: u16,
    pub flags: u16,
    pub length: u32,
}

/// EP0 I/O with a 16 KB buffer (for descriptor responses).
#[repr(C)]
pub struct UsbRawControlEpIo {
    pub inner: UsbRawEpIo,
    pub data: [u8; 16384],
}

impl UsbRawControlEpIo {
    pub fn zeroed() -> Self {
        unsafe { std::mem::zeroed() }
    }
}

/// Interrupt EP I/O with a 32-byte buffer (one controller packet).
#[repr(C)]
pub struct UsbRawInterruptEpIo {
    pub inner: UsbRawEpIo,
    pub data: [u8; 32],
}

impl UsbRawInterruptEpIo {
    pub fn zeroed() -> Self {
        unsafe { std::mem::zeroed() }
    }
}

/// struct usb_endpoint_descriptor (9 bytes, packed).
/// bRefresh and bSynchAddress are 0 for non-audio endpoints.
#[repr(C, packed)]
pub struct UsbEndpointDescriptor {
    pub b_length: u8,
    pub b_descriptor_type: u8,
    pub b_endpoint_address: u8,
    pub bm_attributes: u8,
    pub w_max_packet_size: u16, // little-endian
    pub b_interval: u8,
    pub b_refresh: u8,
    pub b_synch_address: u8,
}

impl UsbEndpointDescriptor {
    pub fn interrupt(address: u8, max_packet: u16, interval: u8) -> Self {
        Self {
            b_length: 7, // USB_DT_ENDPOINT_SIZE (not including audio extension bytes)
            b_descriptor_type: 0x05, // USB_DT_ENDPOINT
            b_endpoint_address: address,
            bm_attributes: USB_ENDPOINT_XFER_INT,
            w_max_packet_size: max_packet.to_le(),
            b_interval: interval,
            b_refresh: 0,
            b_synch_address: 0,
        }
    }
}

// ---------------------------------------------------------------------------
// Unsafe ioctl wrappers
// ---------------------------------------------------------------------------

pub unsafe fn raw_gadget_open() -> io::Result<RawFd> {
    let path = b"/dev/raw-gadget\0";
    let fd = libc::open(path.as_ptr() as *const libc::c_char, libc::O_RDWR);
    if fd < 0 {
        Err(io::Error::last_os_error())
    } else {
        Ok(fd)
    }
}

pub unsafe fn raw_gadget_open_nonblock() -> io::Result<RawFd> {
    let path = b"/dev/raw-gadget\0";
    // O_NONBLOCK is rejected by raw_gadget driver on open(); set it afterwards via fcntl.
    let fd = libc::open(path.as_ptr() as *const libc::c_char, libc::O_RDWR);
    if fd < 0 {
        return Err(io::Error::last_os_error());
    }
    let flags = libc::fcntl(fd, libc::F_GETFL);
    if flags < 0 || libc::fcntl(fd, libc::F_SETFL, flags | libc::O_NONBLOCK) < 0 {
        libc::close(fd);
        return Err(io::Error::last_os_error());
    }
    Ok(fd)
}

pub unsafe fn raw_gadget_close(fd: RawFd) {
    libc::close(fd);
}

pub unsafe fn raw_gadget_init(fd: RawFd, init: &UsbRawInit) -> io::Result<()> {
    let rv = libc::ioctl(fd, USB_RAW_IOCTL_INIT as libc::c_ulong, init as *const _);
    if rv < 0 { Err(io::Error::last_os_error()) } else { Ok(()) }
}

pub unsafe fn raw_gadget_run(fd: RawFd) -> io::Result<()> {
    let rv = libc::ioctl(fd, USB_RAW_IOCTL_RUN as libc::c_ulong, 0usize);
    if rv < 0 { Err(io::Error::last_os_error()) } else { Ok(()) }
}

/// Fetch next event. Blocks until an event is available (or EAGAIN if O_NONBLOCK).
/// Caller must set event.event.length = sizeof(UsbCtrlrequest) before calling.
pub unsafe fn raw_gadget_event_fetch(fd: RawFd, event: &mut UsbRawControlEvent) -> io::Result<()> {
    let rv = libc::ioctl(fd, USB_RAW_IOCTL_EVENT_FETCH as libc::c_ulong, event as *mut _);
    if rv < 0 { Err(io::Error::last_os_error()) } else { Ok(()) }
}

/// Send descriptor data on EP0 (device-to-host).
pub unsafe fn raw_gadget_ep0_write(fd: RawFd, io: &UsbRawControlEpIo) -> io::Result<()> {
    let rv = libc::ioctl(fd, USB_RAW_IOCTL_EP0_WRITE as libc::c_ulong, io as *const _);
    if rv < 0 { Err(io::Error::last_os_error()) } else { Ok(()) }
}

/// Read data from EP0 (host-to-device, e.g. SET_CONFIGURATION data phase).
pub unsafe fn raw_gadget_ep0_read(fd: RawFd, io: &mut UsbRawControlEpIo) -> io::Result<()> {
    let rv = libc::ioctl(fd, USB_RAW_IOCTL_EP0_READ as libc::c_ulong, io as *mut _);
    if rv < 0 { Err(io::Error::last_os_error()) } else { Ok(()) }
}

pub unsafe fn raw_gadget_ep0_stall(fd: RawFd) -> io::Result<()> {
    let rv = libc::ioctl(fd, USB_RAW_IOCTL_EP0_STALL as libc::c_ulong, 0usize);
    if rv < 0 { Err(io::Error::last_os_error()) } else { Ok(()) }
}

/// Enable an endpoint. Returns the ep handle used for subsequent EP_WRITE/EP_READ.
pub unsafe fn raw_gadget_ep_enable(fd: RawFd, desc: &UsbEndpointDescriptor) -> io::Result<i32> {
    let rv = libc::ioctl(fd, USB_RAW_IOCTL_EP_ENABLE as libc::c_ulong, desc as *const _);
    if rv < 0 { Err(io::Error::last_os_error()) } else { Ok(rv) }
}

pub unsafe fn raw_gadget_ep_disable(fd: RawFd, ep: i32) -> io::Result<()> {
    let rv = libc::ioctl(fd, USB_RAW_IOCTL_EP_DISABLE as libc::c_ulong, ep as libc::c_ulong);
    if rv < 0 { Err(io::Error::last_os_error()) } else { Ok(()) }
}

pub unsafe fn raw_gadget_configure(fd: RawFd) -> io::Result<()> {
    let rv = libc::ioctl(fd, USB_RAW_IOCTL_CONFIGURE as libc::c_ulong, 0usize);
    if rv < 0 { Err(io::Error::last_os_error()) } else { Ok(()) }
}

pub unsafe fn raw_gadget_vbus_draw(fd: RawFd, power: u32) -> io::Result<()> {
    let rv = libc::ioctl(fd, USB_RAW_IOCTL_VBUS_DRAW as libc::c_ulong, power as libc::c_ulong);
    if rv < 0 { Err(io::Error::last_os_error()) } else { Ok(()) }
}

/// Write an interrupt IN report (controller → host).
pub unsafe fn raw_gadget_ep_write(fd: RawFd, io: &UsbRawInterruptEpIo) -> io::Result<()> {
    let rv = libc::ioctl(fd, USB_RAW_IOCTL_EP_WRITE as libc::c_ulong, io as *const _);
    if rv < 0 { Err(io::Error::last_os_error()) } else { Ok(()) }
}

/// Read an interrupt OUT report (host → controller, e.g. rumble).
/// Returns EAGAIN error if O_NONBLOCK and no data available.
pub unsafe fn raw_gadget_ep_read(fd: RawFd, io: &mut UsbRawInterruptEpIo) -> io::Result<i32> {
    let rv = libc::ioctl(fd, USB_RAW_IOCTL_EP_READ as libc::c_ulong, io as *mut _);
    if rv < 0 { Err(io::Error::last_os_error()) } else { Ok(rv) }
}
