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
extern struct usb_interface_descriptor usb_if2;
extern struct usb_interface_descriptor usb_if3;
extern struct usb_interface_descriptor usb_if4;
extern struct usb_interface_descriptor usb_if5;
extern struct usb_interface_descriptor usb_if6;
extern struct usb_interface_descriptor usb_if7;

// TODO; Endpoint Descriptors (for each interface)
extern struct usb_endpoint_descriptor usb_if0_ep1_in;
extern struct usb_endpoint_descriptor usb_if0_ep1_out;
extern struct usb_endpoint_descriptor usb_if1_ep2_in;
extern struct usb_endpoint_descriptor usb_if1_ep2_out;
extern struct usb_endpoint_descriptor usb_if1_ep3_in;
extern struct usb_endpoint_descriptor usb_if1_ep3_out;
extern struct usb_endpoint_descriptor usb_if2_ep4_in;

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
extern struct if0_unknown_desc if0_ud;

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
extern struct if1_unknown_desc if1_ud;

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

#endif
