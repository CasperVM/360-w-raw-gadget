#ifndef USB_DESCRIPTORS_H
#define USB_DESCRIPTORS_H

#include <linux/usb/ch9.h>
#include <linux/usb/raw_gadget.h>

// Device Descriptors
extern struct usb_device_descriptor usb_device;
extern struct usb_qualifier_descriptor usb_qualifier;
extern struct usb_config_descriptor usb_config;

// Interface Descriptors
extern struct usb_interface_descriptor usb_if_xinput;

// Endpoint Descriptors
extern struct usb_endpoint_descriptor usb_ep_in;
extern struct usb_endpoint_descriptor usb_ep_out;

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
extern struct if_unknown_desc_control_surface if_ud;

struct if_full_struct
{
    struct usb_interface_descriptor interface;
    struct if_unknown_desc_control_surface ud;
    struct usb_endpoint_descriptor ep_in;
    struct usb_endpoint_descriptor ep_out;
};
// extern struct if_full_struct if_full;

// bcdUSB 2.0
#define BCD_USB 0x0200

#define USB_VENDOR 0x045e  // Microsoft
// #define USB_PRODUCT 0x0719 // Xbox 360 Wireless Adapter
// Both windows and linux allow multiple inputs (controllers) if we act like a 360 controller?
// Weird, but sure...
#define USB_PRODUCT 0x028e // 360 controller
// MacOS can only support 1 input this way? :/

// String indexes for strings in the string descriptor
#define STRING_ID_MANUFACTURER 0x01
#define STRING_ID_PRODUCT 0x02
#define STRING_ID_SERIAL 0x03

// Device setting
#define EP0_MAX_PACKET_CONTROL 64

#endif
