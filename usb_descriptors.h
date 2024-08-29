#ifndef USB_DESCRIPTORS_H
#define USB_DESCRIPTORS_H

#include <linux/usb/ch9.h>
#include <linux/usb/raw_gadget.h>

// Device Descriptors
extern struct usb_device_descriptor usb_device;
extern struct usb_qualifier_descriptor usb_qualifier;
extern struct usb_config_descriptor usb_config;

// Interface Descriptors
extern struct usb_interface_descriptor usb_if0;
extern struct usb_interface_descriptor usb_if1;

// TODO; Endpoint Descriptors (for each interface)
extern struct usb_endpoint_descriptor usb_if0_ep1_in;
extern struct usb_endpoint_descriptor usb_if0_ep1_out;
extern struct usb_endpoint_descriptor usb_if1_ep2_in;
extern struct usb_endpoint_descriptor usb_if1_ep2_out;

// Custom Descriptors
struct if_unknown_desc_control_surface
/*
hex dump;
if1;
0000   14 22 00 01 13 81 1d 00 17 01 02 08 13 01 0c 00
0010   0c 01 02 08
if3;
0000   14 22 00 01 13 83 1d 00 17 01 02 08 13 03 0c 00
0010   0c 01 02 08
...etc

only seems to increment endpoint addresses?
*/ 
{
    __u8 bLength;
    __u8 bDescriptorType;
    __u8 unknown1;
    __u8 unknown2;
    __u8 unknown3;
    __u8 bEndpointAddressIn;
    __u8 unknown5;
    __u8 unknown6;
    __u8 unknown7;
    __u8 unknown8;
    __u8 unknown9;
    __u8 unknown10;
    __u8 unknown11;
    __u8 bEndpointAddressOut;
    __u8 unknown13;
    __u8 unknown14;
    __u8 unknown15;
    __u8 unknown16;
    __u8 unknown17;
    __u8 unknown18;
} __attribute__((packed));
extern struct if_unknown_desc_control_surface if0_ud;

struct if_unknown_desc_audio_surface
/*
0000   0c 22 00 01 01 82 00 40 01 02 20 00
0000   0c 22 00 01 01 84 00 40 01 04 20 00

Same thing here.
*/
{
    __u8 bLength;
    __u8 bDescriptorType;
    __u8 unknown1;
    __u8 unknown2;
    __u8 unknown3;
    __u8 bEndpointAddressIn;
    __u8 unknown5;
    __u8 unknown6;
    __u8 unknown7;
    __u8 bEndpointAddressOut;
    __u8 unknown9;
    __u8 unknown10;
} __attribute__((packed));
extern struct if_unknown_desc_audio_surface if1_ud;

// Relevant constant

// bcdUSB 2.0
#define BCD_USB 0x0200

#define USB_VENDOR 0x045e  // Microsoft
#define USB_PRODUCT 0x0719 // Xbox 360 Wireless Adapter

// String indexes for strings in the string descriptor
#define STRING_ID_MANUFACTURER 0x01
#define STRING_ID_PRODUCT 0x02
#define STRING_ID_SERIAL 0x03

// Device setting
#define EP0_MAX_PACKET_CONTROL 64

// IF0; Control data (1)
#define IF0_INTERFACE_NUMBER 0x00
#define IF0_EP_ADDR 0x01
#define IF0_UD_EP_IN USB_DIR_IN | IF0_EP_ADDR
#define IF0_UD_EP_OUT USB_DIR_OUT | IF0_EP_ADDR

// IF1; Audio data(1)
#define IF1_INTERFACE_NUMBER 0x01
#define IF1_EP_ADDR 0x02
#define IF1_UD_EP_IN USB_DIR_IN | IF1_EP_ADDR
#define IF1_UD_EP_OUT USB_DIR_OUT | IF1_EP_ADDR

// IF2; Control data(2)
#define IF2_INTERFACE_NUMBER 0x02
#define IF2_EP_ADDR 0x03
#define IF2_UD_EP_IN USB_DIR_IN | IF2_EP_ADDR
#define IF2_UD_EP_OUT USB_DIR_OUT | IF2_EP_ADDR

// IF3; Audio data(2)
#define IF3_INTERFACE_NUMBER 0x03
#define IF3_EP_ADDR 0x04
#define IF3_UD_EP_IN USB_DIR_IN | IF3_EP_ADDR
#define IF3_UD_EP_OUT USB_DIR_OUT | IF3_EP_ADDR

// IF4; Control data(3)
#define IF4_INTERFACE_NUMBER 0x04
#define IF4_EP_ADDR 0x05
#define IF4_UD_EP_IN USB_DIR_IN | IF4_EP_ADDR
#define IF4_UD_EP_OUT USB_DIR_OUT | IF4_EP_ADDR

// IF5; Audio data(3)
#define IF5_INTERFACE_NUMBER 0x05
#define IF5_EP_ADDR 0x06
#define IF5_UD_EP_IN USB_DIR_IN | IF5_EP_ADDR
#define IF5_UD_EP_OUT USB_DIR_OUT | IF5_EP_ADDR

// IF6; Control data(4)
#define IF6_INTERFACE_NUMBER 0x06
#define IF6_EP_ADDR 0x07
#define IF6_UD_EP_IN USB_DIR_IN | IF6_EP_ADDR
#define IF6_UD_EP_OUT USB_DIR_OUT | IF6_EP_ADDR

// IF7; Audio data(4)
#define IF7_INTERFACE_NUMBER 0x07
#define IF7_EP_ADDR 0x08
#define IF7_UD_EP_IN USB_DIR_IN | IF7_EP_ADDR
#define IF7_UD_EP_OUT USB_DIR_OUT | IF7_EP_ADDR

//

#endif
