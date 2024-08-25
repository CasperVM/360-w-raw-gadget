#include "usb_descriptors.h"

struct usb_device_descriptor usb_device = {
    .bLength = USB_DT_DEVICE_SIZE,
    .bDescriptorType = USB_DT_DEVICE,
    .bcdUSB = __constant_cpu_to_le16(BCD_USB),
    .bDeviceClass = 0xFF,
    .bDeviceSubClass = 0xFF,
    .bDeviceProtocol = 0xFF,
    .bMaxPacketSize0 = EP0_MAX_PACKET_CONTROL,
    .idVendor = __constant_cpu_to_le16(USB_VENDOR),
    .idProduct = __constant_cpu_to_le16(USB_PRODUCT),
    .bcdDevice = 0,
    .iManufacturer = STRING_ID_MANUFACTURER,
    .iProduct = STRING_ID_PRODUCT,
    .iSerialNumber = STRING_ID_SERIAL,
    .bNumConfigurations = 1,
};

// Device descriptor for full-speed operation.
struct usb_qualifier_descriptor usb_qualifier = {
    .bLength = sizeof(struct usb_qualifier_descriptor),
    .bDescriptorType = USB_DT_DEVICE_QUALIFIER,
    .bcdUSB = __constant_cpu_to_le16(BCD_USB),
    .bDeviceClass = 0,
    .bDeviceSubClass = 0,
    .bDeviceProtocol = 0,
    .bMaxPacketSize0 = EP0_MAX_PACKET_CONTROL,
    .bNumConfigurations = 1,
    .bRESERVED = 0,
};

struct usb_config_descriptor usb_config = {
    .bLength = USB_DT_CONFIG_SIZE,
    .bDescriptorType = USB_DT_CONFIG,
    .wTotalLength = 0,      // computed later
    .bNumInterfaces = 0x04, // 0x04 <- 4 interfaces, todo. try first with 1
    .bConfigurationValue = 1,
    .iConfiguration = 0x00,
    .bmAttributes = 0xA0, // NOT SELF-POWERED, REMOTE WAKEUP
    .bMaxPower = 0xFA,    // 500 mA
};

// INTERFACES

// IF0; Control data
struct usb_interface_descriptor usb_if0 = {
    .bLength = USB_DT_INTERFACE_SIZE,
    .bDescriptorType = USB_DT_INTERFACE,
    .bInterfaceNumber = 0x00,
    .bAlternateSetting = 0x00,
    .bNumEndpoints = 0x02,   // 2 endpoints
    .bInterfaceClass = 0xFF, // Vendor specific
    .bInterfaceSubClass = 0x5D,
    .bInterfaceProtocol = 0x01,
    .iInterface = 0x00,
};

// Unknown descriptor for if0
struct if0_unknown_desc if0_ud = {
    .bLength = 0x11,
    .bDescriptorType = 0x21,
    .unknown1 = 0x00,
    .unknown2 = 0x01,
    .unknown3 = 0x01,
    .unknown4 = 0x25,
    .bEndpointAddress = 0x81,
    .bMaxDataSize = 0x14,
    .unknown5 = 0x00,
    .unknown6 = 0x00,
    .unknown7 = 0x00,
    .unknown8 = 0x00,
    .unknown9 = 0x13,
    .bEndpointAddress2 = 0x01,
    .bMaxDataSize2 = 0x08,
    .unknown10 = 0x00,
    .unknown11 = 0x00,
};

// IF0_EP1_IN; Interrupt data
#define IF0_EP1_ADDR 0x01 // Endpoint number
struct usb_endpoint_descriptor usb_if0_ep1_in = {
    .bLength = USB_DT_ENDPOINT_SIZE,
    .bDescriptorType = USB_DT_ENDPOINT,
    .bEndpointAddress = USB_DIR_IN | IF0_EP1_ADDR,    // Bitwise OR, so 0x81
    .bmAttributes = USB_ENDPOINT_XFER_INT,            // Interrupt endpoint.
    .wMaxPacketSize = __constant_cpu_to_le16(0x0020), // 32 bytes
    .bInterval = 0x04,
};
// IF0_EP1_OUT; Interrupt data
struct usb_endpoint_descriptor usb_if0_ep1_out = {
    .bLength = USB_DT_ENDPOINT_SIZE,
    .bDescriptorType = USB_DT_ENDPOINT,
    .bEndpointAddress = USB_DIR_OUT | IF0_EP1_ADDR,   // Bitwise OR, so 0x01
    .bmAttributes = USB_ENDPOINT_XFER_INT,            // Interrupt endpoint.
    .wMaxPacketSize = __constant_cpu_to_le16(0x0020), // 32 bytes
    .bInterval = 0x08,
};
// IF0 END //

// IF1; Headset (and expansion port?)
struct usb_interface_descriptor usb_if1 = {
    .bLength = USB_DT_INTERFACE_SIZE,
    .bDescriptorType = USB_DT_INTERFACE,
    .bInterfaceNumber = 0x01,
    .bAlternateSetting = 0x00,
    .bNumEndpoints = 0x04,   // 4 endpoints
    .bInterfaceClass = 0xFF, // Vendor specific
    .bInterfaceSubClass = 0x5D,
    .bInterfaceProtocol = 0x03,
    .iInterface = 0x00,
};

// Unknown descriptor for if1
struct if1_unknown_desc if1_ud = {
    .bLength = 0x1B,
    .bDescriptorType = 0x21,
    .unknown1 = 0x00,
    .unknown2 = 0x01,
    .unknown3 = 0x01,
    .unknown4 = 0x01,
    .bEndpointAddress = 0x82,
    .bMaxDataSize = 0x40,
    .unknown5 = 0x01,
    .bEndpointAddress2 = 0x02,
    .bMaxDataSize2 = 0x20,
    .unknown6 = 0x16,
    .bEndpointAddress3 = 0x83,
    .bMaxDataSize3 = 0x00,
    .unknown7 = 0x00,
    .unknown8 = 0x00,
    .unknown9 = 0x00,
    .unknown10 = 0x00,
    .unknown11 = 0x00,
    .unknown12 = 0x16,
    .bEndpointAddress4 = 0x03,
    .bMaxDataSize4 = 0x00,
    .unknown13 = 0x00,
    .unknown14 = 0x00,
    .unknown15 = 0x00,
    .unknown16 = 0x00,
    .unknown17 = 0x00,
};

// IF1_EP2_IN; Microphone data send
#define IF1_EP2_ADDR 0x02
struct usb_endpoint_descriptor usb_if1_ep2_in = {
    .bLength = USB_DT_ENDPOINT_SIZE,
    .bDescriptorType = USB_DT_ENDPOINT,
    .bEndpointAddress = USB_DIR_IN | IF1_EP2_ADDR,
    .bmAttributes = USB_ENDPOINT_XFER_INT,            // Interrupt endpoint
    .wMaxPacketSize = __constant_cpu_to_le16(0x0020), // 32 bytes
    .bInterval = 0x02,
};
// IF1_EP2_OUT; Headset data receive
struct usb_endpoint_descriptor usb_if1_ep2_out = {
    .bLength = USB_DT_ENDPOINT_SIZE,
    .bDescriptorType = USB_DT_ENDPOINT,
    .bEndpointAddress = USB_DIR_OUT | IF1_EP2_ADDR,
    .bmAttributes = USB_ENDPOINT_XFER_INT,            // Interrupt endpoint.
    .wMaxPacketSize = __constant_cpu_to_le16(0x0020), // 32 bytes
    .bInterval = 0x04,
};
// IF1_EP3_IN; Unknown data send
#define IF1_EP3_ADDR 0x03
struct usb_endpoint_descriptor usb_if1_ep3_in = {
    .bLength = USB_DT_ENDPOINT_SIZE,
    .bDescriptorType = USB_DT_ENDPOINT,
    .bEndpointAddress = USB_DIR_IN | IF1_EP3_ADDR,
    .bmAttributes = USB_ENDPOINT_XFER_INT,            // Interrupt endpoint
    .wMaxPacketSize = __constant_cpu_to_le16(0x0020), // 32 bytes
    .bInterval = 0x40,
};
// IF1_EP3_OUT; Unknown data receive
struct usb_endpoint_descriptor usb_if1_ep3_out = {
    .bLength = USB_DT_ENDPOINT_SIZE,
    .bDescriptorType = USB_DT_ENDPOINT,
    .bEndpointAddress = USB_DIR_OUT | IF1_EP3_ADDR,   // Bitwise OR, so 0x01
    .bmAttributes = USB_ENDPOINT_XFER_INT,            // Interrupt endpoint.
    .wMaxPacketSize = __constant_cpu_to_le16(0x0020), // 32 bytes
    .bInterval = 0x10,
};
// IF1 END //

// IF2; Unknown
struct usb_interface_descriptor usb_if2 = {
    .bLength = USB_DT_INTERFACE_SIZE,
    .bDescriptorType = USB_DT_INTERFACE,
    .bInterfaceNumber = 0x02,
    .bAlternateSetting = 0x00,
    .bNumEndpoints = 0x01,   // 1 endpoint
    .bInterfaceClass = 0xFF, // Vendor specific
    .bInterfaceSubClass = 0x5D,
    .bInterfaceProtocol = 0x02,
    .iInterface = 0x00,
};

// Unknown descriptor for if2
struct if2_unknown_desc if2_ud = {
    .bLength = 0x09,
    .bDescriptorType = 0x21,
    .unknown1 = 0x00,
    .unknown2 = 0x01,
    .unknown3 = 0x01,
    .unknown4 = 0x22,
    .bEndpointAddress = 0x84,
    .bMaxDataSize = 0x07,
    .unknown5 = 0x00,
};

// IF2_EP4_IN; Unknown data send
#define IF2_EP4_ADDR 0x04
struct usb_endpoint_descriptor usb_if2_ep4_in = {
    .bLength = USB_DT_ENDPOINT_SIZE,
    .bDescriptorType = USB_DT_ENDPOINT,
    .bEndpointAddress = USB_DIR_IN | IF2_EP4_ADDR,
    .bmAttributes = USB_ENDPOINT_XFER_INT,            // Interrupt endpoint
    .wMaxPacketSize = __constant_cpu_to_le16(0x0020), // 32 bytes
    .bInterval = 0x10,
};
// IF2 END //

// IF3; Security method (unused on pc?)
struct usb_interface_descriptor usb_if3 = {
    .bLength = USB_DT_INTERFACE_SIZE,
    .bDescriptorType = USB_DT_INTERFACE,
    .bInterfaceNumber = 0x03,
    .bAlternateSetting = 0x00,
    .bNumEndpoints = 0x00,   // no endpoints
    .bInterfaceClass = 0xFF, // Vendor specific
    .bInterfaceSubClass = 0xFD,
    .bInterfaceProtocol = 0x13,
    .iInterface = 0x04,
};

// Unknown descriptor for if3
struct if3_unknown_desc if3_ud = {
    .bLength = 0x06,
    .bDescriptorType = 0x41,
    .unknown1 = 0x00,
    .unknown2 = 0x01,
    .unknown3 = 0x01,
    .unknown4 = 0x03,
};
