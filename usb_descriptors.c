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
    .bNumInterfaces = 0x08, // 8 interfaces, 2 for each input? (4 controllers)
    .bConfigurationValue = 1,
    .iConfiguration = 0x00,
    .bmAttributes = 0xA0, // NOT SELF-POWERED, REMOTE WAKEUP
    .bMaxPower = 0xFA,    // 500 mA
};

// INTERFACES

// IF0; Control data (1)
struct usb_interface_descriptor usb_if0 = {
    .bLength = USB_DT_INTERFACE_SIZE,
    .bDescriptorType = USB_DT_INTERFACE,
    .bInterfaceNumber = 0x00,
    .bAlternateSetting = 0x00,
    .bNumEndpoints = 0x02,   // 2 endpoints
    .bInterfaceClass = 0xFF, // Vendor specific
    .bInterfaceSubClass = 0x5D,
    .bInterfaceProtocol = 0x81,
    .iInterface = 0x00,
};

// Unknown descriptor for if0
struct if_unknown_desc_control_surface if0_ud = {
    .bLength = 0x14,
    .bDescriptorType = 0x22,
    .unknown1 = 0x00,
    .unknown2 = 0x01,
    .unknown3 = 0x13,
    .bEndpointAddressIn = 0x81,
    .unknown5 = 0x1d,
    .unknown6 = 0x00,
    .unknown7 = 0x17,
    .unknown8 = 0x01,
    .unknown9 = 0x02,
    .unknown10 = 0x08,
    .unknown11 = 0x13,
    .bEndpointAddressOut = 0x01,
    .unknown13 = 0x0c,
    .unknown14 = 0x00,
    .unknown15 = 0x0c,
    .unknown16 = 0x01,
    .unknown17 = 0x02,
    .unknown18 = 0x08
};

// IF0_EP1_IN; Interrupt data
#define IF0_EP_ADDR 0x01 // Endpoint number
struct usb_endpoint_descriptor usb_if0_ep1_in = {
    .bLength = USB_DT_ENDPOINT_SIZE,
    .bDescriptorType = USB_DT_ENDPOINT,
    .bEndpointAddress = USB_DIR_IN | IF0_EP_ADDR,    // Bitwise OR, so 0x81
    .bmAttributes = USB_ENDPOINT_XFER_INT,            // Interrupt endpoint.
    .wMaxPacketSize = __constant_cpu_to_le16(0x0020), // 32 bytes
    .bInterval = 0x01,
};
// IF0_EP1_OUT; Interrupt data
struct usb_endpoint_descriptor usb_if0_ep1_out = {
    .bLength = USB_DT_ENDPOINT_SIZE,
    .bDescriptorType = USB_DT_ENDPOINT,
    .bEndpointAddress = USB_DIR_OUT | IF0_EP_ADDR,   // Bitwise OR, so 0x01
    .bmAttributes = USB_ENDPOINT_XFER_INT,            // Interrupt endpoint.
    .wMaxPacketSize = __constant_cpu_to_le16(0x0020), // 32 bytes
    .bInterval = 0x08,
};
// IF0 END //

// IF1; (Headset and expansion port?, unsure.)
struct usb_interface_descriptor usb_if1 = {
    .bLength = USB_DT_INTERFACE_SIZE,
    .bDescriptorType = USB_DT_INTERFACE,
    .bInterfaceNumber = 0x01,
    .bAlternateSetting = 0x00,
    .bNumEndpoints = 0x02,   // 2 endpoints
    .bInterfaceClass = 0xFF, // Vendor specific
    .bInterfaceSubClass = 0x5D,
    .bInterfaceProtocol = 0x82,
    .iInterface = 0x00,
};

// Unknown descriptor for if1
struct if_unknown_desc_audio_surface if1_ud = {
    .bLength = 0x0c,
    .bDescriptorType = 0x22,
    .unknown1 = 0x00,
    .unknown2 = 0x01,
    .unknown3 = 0x01,
    .bEndpointAddressIn = 0x82,
    .unknown5 = 0x00, 
    .unknown6 = 0x40,
    .unknown7 = 0x01,
    .bEndpointAddressOut = 0x02,
    .unknown9 = 0x20,
    .unknown10 = 0x00
};

// IF1_EP2_IN; Microphone data send
#define IF1_EP_ADDR 0x02
struct usb_endpoint_descriptor usb_if1_ep2_in = {
    .bLength = USB_DT_ENDPOINT_SIZE,
    .bDescriptorType = USB_DT_ENDPOINT,
    .bEndpointAddress = USB_DIR_IN | IF1_EP_ADDR,
    .bmAttributes = USB_ENDPOINT_XFER_INT,            // Interrupt endpoint
    .wMaxPacketSize = __constant_cpu_to_le16(0x0020), // 32 bytes
    .bInterval = 0x02,
};
// IF1_EP2_OUT; Headset data receive
struct usb_endpoint_descriptor usb_if1_ep2_out = {
    .bLength = USB_DT_ENDPOINT_SIZE,
    .bDescriptorType = USB_DT_ENDPOINT,
    .bEndpointAddress = USB_DIR_OUT | IF1_EP_ADDR,
    .bmAttributes = USB_ENDPOINT_XFER_INT,            // Interrupt endpoint.
    .wMaxPacketSize = __constant_cpu_to_le16(0x0020), // 32 bytes
    .bInterval = 0x04,
};
// IF1 END //

// IF2; Control data(2)
#define IF2_INTERFACE_NUMBER 0x02
#define IF2_EP_ADDR = 0x03
#define IF2_UD_EP_IN = USB_DIR_IN | IF2_EP_ADDR
#define IF2_UD_EP_OUT = USB_DIR_OUT | IF2_EP_ADDR

// IF3; Audio data(2)
#define IF3_INTERFACE_NUMBER 0x03
#define IF3_EP_ADDR = 0x04
#define IF3_UD_EP_IN = USB_DIR_IN | IF3_EP_ADDR
#define IF3_UD_EP_OUT = USB_DIR_OUT | IF3_EP_ADDR

// IF4; Control data(3)
#define IF4_INTERFACE_NUMBER 0x04
#define IF4_EP_ADDR = 0x05
#define IF4_UD_EP_IN = USB_DIR_IN | IF4_EP_ADDR
#define IF4_UD_EP_OUT = USB_DIR_OUT | IF4_EP_ADDR

// IF5; Audio data(3)
#define IF5_INTERFACE_NUMBER 0x05
#define IF5_EP_ADDR = 0x06
#define IF5_UD_EP_IN = USB_DIR_IN | IF5_EP_ADDR
#define IF5_UD_EP_OUT = USB_DIR_OUT | IF5_EP_ADDR

// IF6; Control data(4)
#define IF6_INTERFACE_NUMBER 0x06
#define IF6_EP_ADDR = 0x07
#define IF6_UD_EP_IN = USB_DIR_IN | IF6_EP_ADDR
#define IF6_UD_EP_OUT = USB_DIR_OUT | IF6_EP_ADDR

// IF7; Audio data(4)
#define IF7_INTERFACE_NUMBER 0x07
#define IF7_EP_ADDR = 0x08
#define IF7_UD_EP_IN = USB_DIR_IN | IF7_EP_ADDR
#define IF7_UD_EP_OUT = USB_DIR_OUT | IF7_EP_ADDR
