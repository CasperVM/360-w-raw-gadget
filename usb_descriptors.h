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

// Endpoint Descriptors (for each interface)
extern struct usb_endpoint_descriptor usb_if0_ep1_in;
extern struct usb_endpoint_descriptor usb_if0_ep1_out;
extern struct usb_endpoint_descriptor usb_if1_ep2_in;
extern struct usb_endpoint_descriptor usb_if1_ep2_out;
extern struct usb_endpoint_descriptor usb_if1_ep3_in;
extern struct usb_endpoint_descriptor usb_if1_ep3_out;
extern struct usb_endpoint_descriptor usb_if2_ep4_in;

// Custom Descriptors
struct if0_unknown_desc
{
    __u8 bLength;
    __u8 bDescriptorType;
    __u8 unknown1;
    __u8 unknown2;
    __u8 unknown3;
    __u8 unknown4;
    __u8 bEndpointAddress;
    __u8 bMaxDataSize;
    __u8 unknown5;
    __u8 unknown6;
    __u8 unknown7;
    __u8 unknown8;
    __u8 unknown9;
    __u8 bEndpointAddress2;
    __u8 bMaxDataSize2;
    __u8 unknown10;
    __u8 unknown11;
} __attribute__((packed));
extern struct if0_unknown_desc if0_ud;

struct if1_unknown_desc
{
    __u8 bLength;
    __u8 bDescriptorType;
    __u8 unknown1;
    __u8 unknown2;
    __u8 unknown3;
    __u8 unknown4;
    __u8 bEndpointAddress;
    __u8 bMaxDataSize;
    __u8 unknown5;
    __u8 bEndpointAddress2;
    __u8 bMaxDataSize2;
    __u8 unknown6;
    __u8 bEndpointAddress3;
    __u8 bMaxDataSize3;
    __u8 unknown7;
    __u8 unknown8;
    __u8 unknown9;
    __u8 unknown10;
    __u8 unknown11;
    __u8 unknown12;
    __u8 bEndpointAddress4;
    __u8 bMaxDataSize4;
    __u8 unknown13;
    __u8 unknown14;
    __u8 unknown15;
    __u8 unknown16;
    __u8 unknown17;
} __attribute__((packed));
extern struct if1_unknown_desc if1_ud;

struct if2_unknown_desc
{
    __u8 bLength;
    __u8 bDescriptorType;
    __u8 unknown1;
    __u8 unknown2;
    __u8 unknown3;
    __u8 unknown4;
    __u8 bEndpointAddress;
    __u8 bMaxDataSize;
    __u8 unknown5;
} __attribute__((packed));
extern struct if2_unknown_desc if2_ud;

struct if3_unknown_desc
{
    __u8 bLength;
    __u8 bDescriptorType;
    __u8 unknown1;
    __u8 unknown2;
    __u8 unknown3;
    __u8 unknown4;
} __attribute__((packed));
extern struct if3_unknown_desc if3_ud;

// Relevant constant

// bcdUSB 2.0
#define BCD_USB 0x0200

#define USB_VENDOR 0x045e  // Microsoft
#define USB_PRODUCT 0x028e // Xbox 360 Controller

// String indexes for strings in the string descriptor
#define STRING_ID_MANUFACTURER 0x01
#define STRING_ID_PRODUCT 0x02
#define STRING_ID_SERIAL 0x03
// Interface 3 / Security method (unused on pc?)
#define STRING_ID_IF3 0x04

// Device setting
#define EP0_MAX_PACKET_CONTROL 64

#endif
