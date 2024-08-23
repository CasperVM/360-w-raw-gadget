// SPDX-License-Identifier: Apache-2.0
//
// Originally written for https://github.com/google/syzkaller.
//
// Copyright 2019 syzkaller project authors. All rights reserved.
// Use of this source code is governed by Apache 2 LICENSE.

// Partial copy of usb raw gadget keyboard example.
// Will edit this further later.

#include <assert.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include <linux/types.h>
#include <linux/hid.h>
#include <linux/usb/ch9.h>

// Unlike the example, also just include the raw_gadget.h file, no need to redefine existing structs
#include <linux/usb/raw_gadget.h>

/*----------------------------------------------------------------------*/

#define BCD_USB 0x0200 // bcdUSB 2.0

#define USB_VENDOR 0x045e  // Microsoft
#define USB_PRODUCT 0x028e // Xbox 360 Controller

// String indexes for strings in the string descriptor
#define STRING_ID_MANUFACTURER 0x01
#define STRING_ID_PRODUCT 0x02
#define STRING_ID_SERIAL 0x03
// Interface 3 / Security method (unused on pc?)
#define STRING_ID_IF3 0x04

// Max sizes
#define EP0_MAX_DATA 256
// Device setting,
#define EP0_MAX_PACKET_CONTROL 64
// EP1, control surface -> 32 bytes max. (in actuality, 20 bytes get sent)
#define EP_MAX_PACKET_INT __constant_cpu_to_le16(0x0020)

// Structs
struct usb_raw_control_event
{
    struct usb_raw_event inner;
    struct usb_ctrlrequest ctrl;
};

struct usb_raw_control_io
{
    struct usb_raw_ep_io inner;
    char data[EP0_MAX_DATA];
};

struct usb_raw_int_io
{
    struct usb_raw_ep_io inner;
    char data[EP_MAX_PACKET_INT];
};

// Descriptors

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
struct if3_unknown_desc
{
    __u8 bLength;
    __u8 bDescriptorType;

    __u8 unknown1;
    __u8 unknown2;
    __u8 unknown3;
    __u8 unknown4;
} __attribute__((packed));

struct if3_unknown_desc if3_ud = {
    .bLength = 0x06,
    .bDescriptorType = 0x41,
    .unknown1 = 0x00,
    .unknown2 = 0x01,
    .unknown3 = 0x01,
    .unknown4 = 0x03,
};

// taken from latest raw_gadget.h -> needed to rename the enum to avoid conflict (yay)
enum usb_raw_event_type_copy
{
    USB_RAW_EVENT_INVALID_2 = 0,

    /* This event is queued when the driver has bound to a UDC. */
    USB_RAW_EVENT_CONNECT_2 = 1,

    /* This event is queued when a new control request arrived to ep0. */
    USB_RAW_EVENT_CONTROL_2 = 2,

    /*
     * These events are queued when the gadget driver is suspended,
     * resumed, reset, or disconnected. Note that some UDCs (e.g. dwc2)
     * report a disconnect event instead of a reset.
     */
    USB_RAW_EVENT_SUSPEND_2 = 3,
    USB_RAW_EVENT_RESUME_2 = 4,
    USB_RAW_EVENT_RESET_2 = 5,
    USB_RAW_EVENT_DISCONNECT_2 = 6,

    /* The list might grow in the future. */
};

int build_config(char *data, int length, bool other_speed)
{
    // Build the configuration descriptor.
    // Contains the device descriptor, interface descriptor, custom desc and endpoint descriptor.
    struct usb_config_descriptor *config =
        (struct usb_config_descriptor *)data;
    int total_length = 0;

    assert(length >= sizeof(usb_config));
    memcpy(data, &usb_config, sizeof(usb_config));
    data += sizeof(usb_config);
    length -= sizeof(usb_config);
    total_length += sizeof(usb_config);

    // IF0
    assert(length >= sizeof(usb_if0));
    memcpy(data, &usb_if0, sizeof(usb_if0));
    data += sizeof(usb_if0);
    length -= sizeof(usb_if0);
    total_length += sizeof(usb_if0);

    // IF 0 unknown descriptor
    assert(length >= sizeof(if0_ud));
    memcpy(data, &if0_ud, sizeof(if0_ud));
    data += sizeof(if0_ud);
    length -= sizeof(if0_ud);
    total_length += sizeof(if0_ud);

    // IF0 EP1 IN
    assert(length >= USB_DT_ENDPOINT_SIZE);
    memcpy(data, &usb_if0_ep1_in, USB_DT_ENDPOINT_SIZE);
    data += USB_DT_ENDPOINT_SIZE;
    length -= USB_DT_ENDPOINT_SIZE;
    total_length += USB_DT_ENDPOINT_SIZE;

    // IF0 EP1 OUT
    assert(length >= USB_DT_ENDPOINT_SIZE);
    memcpy(data, &usb_if0_ep1_out, USB_DT_ENDPOINT_SIZE);
    data += USB_DT_ENDPOINT_SIZE;
    length -= USB_DT_ENDPOINT_SIZE;
    total_length += USB_DT_ENDPOINT_SIZE;

    // IF1
    assert(length >= sizeof(usb_if1));
    memcpy(data, &usb_if1, sizeof(usb_if1));
    data += sizeof(usb_if1);
    length -= sizeof(usb_if1);
    total_length += sizeof(usb_if1);

    // IF 1 unknown descriptor
    assert(length >= sizeof(if1_ud));
    memcpy(data, &if1_ud, sizeof(if1_ud));
    data += sizeof(if1_ud);
    length -= sizeof(if1_ud);
    total_length += sizeof(if1_ud);

    // IF1 EP2 IN
    assert(length >= USB_DT_ENDPOINT_SIZE);
    memcpy(data, &usb_if1_ep2_in, USB_DT_ENDPOINT_SIZE);
    data += USB_DT_ENDPOINT_SIZE;
    length -= USB_DT_ENDPOINT_SIZE;
    total_length += USB_DT_ENDPOINT_SIZE;

    // IF1 EP2 OUT
    assert(length >= USB_DT_ENDPOINT_SIZE);
    memcpy(data, &usb_if1_ep2_out, USB_DT_ENDPOINT_SIZE);
    data += USB_DT_ENDPOINT_SIZE;
    length -= USB_DT_ENDPOINT_SIZE;
    total_length += USB_DT_ENDPOINT_SIZE;

    // IF1 EP3 IN
    assert(length >= USB_DT_ENDPOINT_SIZE);
    memcpy(data, &usb_if1_ep3_in, USB_DT_ENDPOINT_SIZE);
    data += USB_DT_ENDPOINT_SIZE;
    length -= USB_DT_ENDPOINT_SIZE;
    total_length += USB_DT_ENDPOINT_SIZE;

    // IF1 EP3 OUT
    assert(length >= USB_DT_ENDPOINT_SIZE);
    memcpy(data, &usb_if1_ep3_out, USB_DT_ENDPOINT_SIZE);
    data += USB_DT_ENDPOINT_SIZE;
    length -= USB_DT_ENDPOINT_SIZE;
    total_length += USB_DT_ENDPOINT_SIZE;

    // IF2
    assert(length >= sizeof(usb_if2));
    memcpy(data, &usb_if2, sizeof(usb_if2));
    data += sizeof(usb_if2);
    length -= sizeof(usb_if2);
    total_length += sizeof(usb_if2);

    // IF 2 unknown descriptor
    assert(length >= sizeof(if2_ud));
    memcpy(data, &if2_ud, sizeof(if2_ud));
    data += sizeof(if2_ud);
    length -= sizeof(if2_ud);
    total_length += sizeof(if2_ud);

    // IF2 EP4 IN
    assert(length >= USB_DT_ENDPOINT_SIZE);
    memcpy(data, &usb_if2_ep4_in, USB_DT_ENDPOINT_SIZE);
    data += USB_DT_ENDPOINT_SIZE;
    length -= USB_DT_ENDPOINT_SIZE;
    total_length += USB_DT_ENDPOINT_SIZE;

    // IF3
    assert(length >= sizeof(usb_if3));
    memcpy(data, &usb_if3, sizeof(usb_if3));
    data += sizeof(usb_if3);
    length -= sizeof(usb_if3);
    total_length += sizeof(usb_if3);

    config->wTotalLength = __cpu_to_le16(total_length);
    printf("config->wTotalLength: %d\n", total_length);

    if (other_speed)
        config->bDescriptorType = USB_DT_OTHER_SPEED_CONFIG;

    return total_length;
}

// IO functions

int usb_raw_open()
{
    int fd = open("/dev/raw-gadget", O_RDWR);
    if (fd < 0)
    {
        perror("open()");
        exit(EXIT_FAILURE);
    }
    return fd;
}

void usb_raw_init(int fd, enum usb_device_speed speed,
                  const char *driver, const char *device)
{
    struct usb_raw_init arg;
    strcpy((char *)&arg.driver_name[0], driver);
    strcpy((char *)&arg.device_name[0], device);
    arg.speed = speed;
    int rv = ioctl(fd, USB_RAW_IOCTL_INIT, &arg);
    if (rv < 0)
    {
        perror("ioctl(USB_RAW_IOCTL_INIT)");
        exit(EXIT_FAILURE);
    }
}

void usb_raw_run(int fd)
{
    int rv = ioctl(fd, USB_RAW_IOCTL_RUN, 0);
    if (rv < 0)
    {
        perror("ioctl(USB_RAW_IOCTL_RUN)");
        exit(EXIT_FAILURE);
    }
}

void usb_raw_event_fetch(int fd, struct usb_raw_event *event)
{
    int rv = ioctl(fd, USB_RAW_IOCTL_EVENT_FETCH, event);
    if (rv < 0)
    {
        perror("ioctl(USB_RAW_IOCTL_EVENT_FETCH)");
        exit(EXIT_FAILURE);
    }
}

int usb_raw_ep0_read(int fd, struct usb_raw_ep_io *io)
{
    int rv = ioctl(fd, USB_RAW_IOCTL_EP0_READ, io);
    if (rv < 0)
    {
        perror("ioctl(USB_RAW_IOCTL_EP0_READ)");
        exit(EXIT_FAILURE);
    }
    return rv;
}

int usb_raw_ep0_write(int fd, struct usb_raw_ep_io *io)
{
    int rv = ioctl(fd, USB_RAW_IOCTL_EP0_WRITE, io);
    if (rv < 0)
    {
        perror("ioctl(USB_RAW_IOCTL_EP0_WRITE)");
        exit(EXIT_FAILURE);
    }
    return rv;
}

int usb_raw_ep_enable(int fd, struct usb_endpoint_descriptor *desc)
{
    int rv = ioctl(fd, USB_RAW_IOCTL_EP_ENABLE, desc);
    if (rv < 0)
    {
        perror("ioctl(USB_RAW_IOCTL_EP_ENABLE)");
        exit(EXIT_FAILURE);
    }
    return rv;
}

int usb_raw_ep_disable(int fd, int ep)
{
    int rv = ioctl(fd, USB_RAW_IOCTL_EP_DISABLE, ep);
    if (rv < 0)
    {
        perror("ioctl(USB_RAW_IOCTL_EP_DISABLE)");
        exit(EXIT_FAILURE);
    }
    return rv;
}

int usb_raw_ep_read(int fd, struct usb_raw_ep_io *io)
{
    int rv = ioctl(fd, USB_RAW_IOCTL_EP_READ, io);
    if (rv < 0)
    {
        perror("ioctl(USB_RAW_IOCTL_EP_READ)");
        exit(EXIT_FAILURE);
    }
    return rv;
}

int usb_raw_ep_write(int fd, struct usb_raw_ep_io *io)
{
    int rv = ioctl(fd, USB_RAW_IOCTL_EP_WRITE, io);
    if (rv < 0)
    {
        perror("ioctl(USB_RAW_IOCTL_EP_WRITE)");
        exit(EXIT_FAILURE);
    }
    return rv;
}

int usb_raw_ep_write_may_fail(int fd, struct usb_raw_ep_io *io)
{
    return ioctl(fd, USB_RAW_IOCTL_EP_WRITE, io);
}

void usb_raw_configure(int fd)
{
    int rv = ioctl(fd, USB_RAW_IOCTL_CONFIGURE, 0);
    if (rv < 0)
    {
        perror("ioctl(USB_RAW_IOCTL_CONFIGURED)");
        exit(EXIT_FAILURE);
    }
}

void usb_raw_vbus_draw(int fd, uint32_t power)
{
    int rv = ioctl(fd, USB_RAW_IOCTL_VBUS_DRAW, power);
    if (rv < 0)
    {
        perror("ioctl(USB_RAW_IOCTL_VBUS_DRAW)");
        exit(EXIT_FAILURE);
    }
}

int usb_raw_eps_info(int fd, struct usb_raw_eps_info *info)
{
    int rv = ioctl(fd, USB_RAW_IOCTL_EPS_INFO, info);
    if (rv < 0)
    {
        perror("ioctl(USB_RAW_IOCTL_EPS_INFO)");
        exit(EXIT_FAILURE);
    }
    return rv;
}

void usb_raw_ep0_stall(int fd)
{
    int rv = ioctl(fd, USB_RAW_IOCTL_EP0_STALL, 0);
    if (rv < 0)
    {
        perror("ioctl(USB_RAW_IOCTL_EP0_STALL)");
        exit(EXIT_FAILURE);
    }
}

void usb_raw_ep_set_halt(int fd, int ep)
{
    int rv = ioctl(fd, USB_RAW_IOCTL_EP_SET_HALT, ep);
    if (rv < 0)
    {
        perror("ioctl(USB_RAW_IOCTL_EP_SET_HALT)");
        exit(EXIT_FAILURE);
    }
}

bool assign_ep_address(struct usb_raw_ep_info *info,
                       struct usb_endpoint_descriptor *ep)
{
    if (usb_endpoint_num(ep) != 0)
        return false; // Already assigned.
    if (usb_endpoint_dir_in(ep) && !info->caps.dir_in)
        return false;
    if (usb_endpoint_dir_out(ep) && !info->caps.dir_out)
        return false;
    if (usb_endpoint_maxp(ep) > info->limits.maxpacket_limit)
        return false;
    switch (usb_endpoint_type(ep))
    {
    case USB_ENDPOINT_XFER_BULK:
        if (!info->caps.type_bulk)
            return false;
        break;
    case USB_ENDPOINT_XFER_INT:
        if (!info->caps.type_int)
            return false;
        break;
    default:
        assert(false);
    }
    if (info->addr == USB_RAW_EP_ADDR_ANY)
    {
        static int addr = 1;
        ep->bEndpointAddress |= addr++;
    }
    else
        ep->bEndpointAddress |= info->addr;
    return true;
}

void process_eps_info(int fd)
{
    // Process non-control endpoint information
    struct usb_raw_eps_info info;
    memset(&info, 0, sizeof(info));

    int num = usb_raw_eps_info(fd, &info);
    for (int i = 0; i < num; i++)
    {
        printf("ep #%d:\n", i);
        printf("  name: %s\n", &info.eps[i].name[0]);
        printf("  addr: %u\n", info.eps[i].addr);
        printf("  type: %s %s %s\n",
               info.eps[i].caps.type_iso ? "iso" : "___",
               info.eps[i].caps.type_bulk ? "blk" : "___",
               info.eps[i].caps.type_int ? "int" : "___");
        printf("  dir : %s %s\n",
               info.eps[i].caps.dir_in ? "in " : "___",
               info.eps[i].caps.dir_out ? "out" : "___");
        printf("  maxpacket_limit: %u\n",
               info.eps[i].limits.maxpacket_limit);
        printf("  max_streams: %u\n", info.eps[i].limits.max_streams);
    }

    for (int i = 0; i < num; i++)
    {
        if (assign_ep_address(&info.eps[i], &usb_if0_ep1_in))
            continue;
    }

    int ep_int_in_addr = usb_endpoint_num(&usb_if0_ep1_in);
    assert(ep_int_in_addr != 0);
    printf("ep_int_in: addr = %u\n", ep_int_in_addr);
}

void set_usb_string_desc(const char *str, struct usb_raw_control_io *io)
{
    int str_len = strlen(str);
    int descriptor_len = 2; // Initial length for length byte and descriptor type byte

    io->data[1] = USB_DT_STRING;

    // Process each character
    for (int i = 0, pos = 2; i < str_len; i++)
    {
        unsigned char c = str[i];
        if (c == 0xC2 && i + 1 < str_len && (unsigned char)str[i + 1] == 0xA9)
        {
            // Encode '©' in UTF-16LE
            io->data[pos++] = 0xA9;
            io->data[pos++] = 0x00;
            i++; // Skip the next byte in the input string, as it's part of '©' UTF-8 encoding
            // Adjust the descriptor length, as we're using 2 bytes for this character not 4.
            // UTF-8 -> UTF-16LE: 4 bytes -> 2 bytes :-D
            descriptor_len -= 2;
        }
        else
        {
            // Convert ASCII to UTF-16LE
            io->data[pos++] = c;    // ASCII character
            io->data[pos++] = 0x00; // High byte (0x00 for basic ASCII chars)
        }
    }

    descriptor_len += str_len * 2;     // Adjust the descriptor length
    io->data[0] = descriptor_len;      // Set total length of the descriptor
    io->inner.length = descriptor_len; // Set the length field for transmission
}

// LOG

void log_control_request(struct usb_ctrlrequest *ctrl)
{
    printf("  bRequestType: 0x%x (%s), bRequest: 0x%x, wValue: 0x%x,"
           " wIndex: 0x%x, wLength: %d\n",
           ctrl->bRequestType,
           (ctrl->bRequestType & USB_DIR_IN) ? "IN" : "OUT",
           ctrl->bRequest, ctrl->wValue, ctrl->wIndex, ctrl->wLength);

    switch (ctrl->bRequestType & USB_TYPE_MASK)
    {
    case USB_TYPE_STANDARD:
        printf("  type = USB_TYPE_STANDARD\n");
        break;
    case USB_TYPE_CLASS:
        printf("  type = USB_TYPE_CLASS\n");
        break;
    case USB_TYPE_VENDOR:
        printf("  type = USB_TYPE_VENDOR\n");
        break;
    default:
        printf("  type = unknown = %d\n", (int)ctrl->bRequestType);
        break;
    }

    switch (ctrl->bRequestType & USB_TYPE_MASK)
    {
    case USB_TYPE_STANDARD:
        switch (ctrl->bRequest)
        {
        case USB_REQ_GET_DESCRIPTOR:
            printf("  req = USB_REQ_GET_DESCRIPTOR\n");
            switch (ctrl->wValue >> 8)
            {
            case USB_DT_DEVICE:
                printf("  desc = USB_DT_DEVICE\n");
                break;
            case USB_DT_CONFIG:
                printf("  desc = USB_DT_CONFIG\n");
                break;
            case USB_DT_STRING:
                printf("  desc = USB_DT_STRING\n");
                break;
            case USB_DT_INTERFACE:
                printf("  desc = USB_DT_INTERFACE\n");
                break;
            case USB_DT_ENDPOINT:
                printf("  desc = USB_DT_ENDPOINT\n");
                break;
            case USB_DT_DEVICE_QUALIFIER:
                printf("  desc = USB_DT_DEVICE_QUALIFIER\n");
                break;
            case USB_DT_OTHER_SPEED_CONFIG:
                printf("  desc = USB_DT_OTHER_SPEED_CONFIG\n");
                break;
            case USB_DT_INTERFACE_POWER:
                printf("  desc = USB_DT_INTERFACE_POWER\n");
                break;
            case USB_DT_OTG:
                printf("  desc = USB_DT_OTG\n");
                break;
            case USB_DT_DEBUG:
                printf("  desc = USB_DT_DEBUG\n");
                break;
            case USB_DT_INTERFACE_ASSOCIATION:
                printf("  desc = USB_DT_INTERFACE_ASSOCIATION\n");
                break;
            case USB_DT_SECURITY:
                printf("  desc = USB_DT_SECURITY\n");
                break;
            case USB_DT_KEY:
                printf("  desc = USB_DT_KEY\n");
                break;
            case USB_DT_ENCRYPTION_TYPE:
                printf("  desc = USB_DT_ENCRYPTION_TYPE\n");
                break;
            case USB_DT_BOS:
                printf("  desc = USB_DT_BOS\n");
                break;
            case USB_DT_DEVICE_CAPABILITY:
                printf("  desc = USB_DT_DEVICE_CAPABILITY\n");
                break;
            case USB_DT_WIRELESS_ENDPOINT_COMP:
                printf("  desc = USB_DT_WIRELESS_ENDPOINT_COMP\n");
                break;
            case USB_DT_PIPE_USAGE:
                printf("  desc = USB_DT_PIPE_USAGE\n");
                break;
            case USB_DT_SS_ENDPOINT_COMP:
                printf("  desc = USB_DT_SS_ENDPOINT_COMP\n");
                break;
            case HID_DT_HID:
                printf("  descriptor = HID_DT_HID\n");
                return;
            case HID_DT_REPORT:
                printf("  descriptor = HID_DT_REPORT\n");
                return;
            case HID_DT_PHYSICAL:
                printf("  descriptor = HID_DT_PHYSICAL\n");
                return;
            default:
                printf("  desc = unknown = 0x%x\n",
                       ctrl->wValue >> 8);
                break;
            }
            break;
        case USB_REQ_SET_CONFIGURATION:
            printf("  req = USB_REQ_SET_CONFIGURATION\n");
            break;
        case USB_REQ_GET_CONFIGURATION:
            printf("  req = USB_REQ_GET_CONFIGURATION\n");
            break;
        case USB_REQ_SET_INTERFACE:
            printf("  req = USB_REQ_SET_INTERFACE\n");
            break;
        case USB_REQ_GET_INTERFACE:
            printf("  req = USB_REQ_GET_INTERFACE\n");
            break;
        case USB_REQ_GET_STATUS:
            printf("  req = USB_REQ_GET_STATUS\n");
            break;
        case USB_REQ_CLEAR_FEATURE:
            printf("  req = USB_REQ_CLEAR_FEATURE\n");
            break;
        case USB_REQ_SET_FEATURE:
            printf("  req = USB_REQ_SET_FEATURE\n");
            break;
        default:
            printf("  req = unknown = 0x%x\n", ctrl->bRequest);
            break;
        }
        break;
    case USB_TYPE_CLASS:
        switch (ctrl->bRequest)
        {
        case HID_REQ_GET_REPORT:
            printf("  req = HID_REQ_GET_REPORT\n");
            break;
        case HID_REQ_GET_IDLE:
            printf("  req = HID_REQ_GET_IDLE\n");
            break;
        case HID_REQ_GET_PROTOCOL:
            printf("  req = HID_REQ_GET_PROTOCOL\n");
            break;
        case HID_REQ_SET_REPORT:
            printf("  req = HID_REQ_SET_REPORT\n");
            break;
        case HID_REQ_SET_IDLE:
            printf("  req = HID_REQ_SET_IDLE\n");
            break;
        case HID_REQ_SET_PROTOCOL:
            printf("  req = HID_REQ_SET_PROTOCOL\n");
            break;
        default:
            printf("  req = unknown = 0x%x\n", ctrl->bRequest);
            break;
        }
        break;
    default:
        printf("  req = unknown = 0x%x\n", ctrl->bRequest);
        break;
    }
}

void log_event(struct usb_raw_event *event)
{
    switch (event->type)
    {
    case USB_RAW_EVENT_CONNECT_2:
        printf("event: connect, length: %u\n", event->length);
        break;
    case USB_RAW_EVENT_CONTROL_2:
        printf("event: control, length: %u\n", event->length);
        log_control_request((struct usb_ctrlrequest *)&event->data[0]);
        break;
    case USB_RAW_EVENT_SUSPEND_2:
        printf("event: suspend\n");
        break;
    case USB_RAW_EVENT_RESUME_2:
        printf("event: resume\n");
        break;
    case USB_RAW_EVENT_RESET_2:
        printf("event: reset\n");
        break;
    case USB_RAW_EVENT_DISCONNECT_2:
        printf("event: disconnect\n");
        break;
    default:
        printf("event: %d (unknown), length: %u\n", event->type, event->length);
    }
}

// Endpoint thread.
int ep_int_in = -1;
pthread_t ep_int_in_thread;
bool ep_int_in_thread_spawned = false;

void *ep_int_in_loop(void *arg)
{
    // INPUT
    // Loop to keep sending data to the interrupt endpoint
    int fd = (int)(long)arg;

    struct usb_raw_int_io io;
    io.inner.ep = ep_int_in;
    io.inner.flags = 0;
    io.inner.length = 20;

    while (true)
    {
        // A button press on the controller;
        // 0x00, 0x14, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
        // Blank input -> 20x 0x00
        // Send button press 'A' (0x1b)
        memcpy(&io.inner.data[0],
               "\x00\x14\x00\x10\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00", 20);
        int rv = usb_raw_ep_write_may_fail(fd,
                                           (struct usb_raw_ep_io *)&io);
        if (rv < 0 && errno == ESHUTDOWN)
        {
            // If the device was reset, exit the loop
            printf("ep_int_in: device was likely reset, exiting\n");
            break;
        }
        else if (rv < 0)
        {
            // Device busy?
            // TODO: should handle this better/try again
            perror("usb_raw_ep_write_may_fail()");
            exit(EXIT_FAILURE);
        }
        printf("ep_int_in: key down: %d\n", rv);
        sleep(1);
        // Reset inputs.
        memcpy(&io.inner.data[0],
               "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00", 20);
        // TODO: Same as above, should handle this better
        rv = usb_raw_ep_write_may_fail(fd, (struct usb_raw_ep_io *)&io);
        if (rv < 0 && errno == ESHUTDOWN)
        {
            printf("ep_int_in: device was likely reset, exiting\n");
            break;
        }
        else if (rv < 0)
        {
            perror("usb_raw_ep_write_may_fail()");
            exit(EXIT_FAILURE);
        }
        printf("ep_int_in: key up: %d\n", rv);
        sleep(1);
    }

    return NULL;
}

// Run.
bool ep0_request(int fd, struct usb_raw_control_event *event,
                 struct usb_raw_control_io *io)
{
    switch (event->ctrl.bRequestType & USB_TYPE_MASK)
    {
    case USB_TYPE_STANDARD:
        switch (event->ctrl.bRequest)
        {
        case USB_REQ_GET_DESCRIPTOR:
            switch (event->ctrl.wValue >> 8)
            {
            case USB_DT_DEVICE:
                memcpy(&io->data[0], &usb_device,
                       sizeof(usb_device));
                io->inner.length = sizeof(usb_device);
                return true;
            case USB_DT_DEVICE_QUALIFIER:
                memcpy(&io->data[0], &usb_qualifier,
                       sizeof(usb_qualifier));
                io->inner.length = sizeof(usb_qualifier);
                return true;
            case USB_DT_CONFIG:
                io->inner.length =
                    build_config(&io->data[0],
                                 sizeof(io->data), false);
                return true;
            case USB_DT_OTHER_SPEED_CONFIG:
                io->inner.length =
                    build_config(&io->data[0],
                                 sizeof(io->data), true);
                return true;
            case USB_DT_STRING:
                printf("STRING DESCRIPTOR REQ; %X\n", event->ctrl.wValue & 0xff);
                switch (event->ctrl.wValue & 0xff)
                {
                case 0:
                    // Language ID request (String descriptor 0)
                    io->data[0] = 4; // bLength (4 bytes)
                    io->data[1] = USB_DT_STRING;
                    // No lang -> 0x0000
                    io->data[2] = 0x00;
                    io->data[3] = 0x00;
                    io->inner.length = 4;
                    break;
                case STRING_ID_MANUFACTURER:
                    // Manufacturer string descriptor
                    set_usb_string_desc("©Microsoft Corporation", io);
                    break;
                case STRING_ID_PRODUCT:
                    // Product string descriptor
                    set_usb_string_desc("Controller", io);
                    break;
                case STRING_ID_SERIAL:
                    // Serial number string descriptor
                    set_usb_string_desc("08FEC93", io);
                    break;
                case STRING_ID_IF3:
                    // Interface 3 string descriptor
                    set_usb_string_desc("Xbox Security Method 3, Version 1.00, © 2005 Microsoft Corporation. All rights reserved.", io);
                    break;
                }
                return true;
            case HID_DT_REPORT:
                // TOOD: unsure what to do here for now
                printf("HID_DT_REPORT\n");
                return true;
            default:
                printf("fail: no response 1\n");
                exit(EXIT_FAILURE);
            }
            break;
        case USB_REQ_SET_CONFIGURATION:
            // Loop disabled for now.. -> different inputs for xbox as opposed to keyboard.
            ep_int_in = usb_raw_ep_enable(fd, &usb_if0_ep1_in);
            printf("ep0: ep_int_in enabled: %d\n", ep_int_in);
            int rv = pthread_create(&ep_int_in_thread, 0,
                                    ep_int_in_loop, (void *)(long)fd);
            if (rv != 0)
            {
                perror("pthread_create(ep_int_in)");
                exit(EXIT_FAILURE);
            }
            ep_int_in_thread_spawned = true;
            printf("ep0: spawned ep_int_in thread\n");
            usb_raw_vbus_draw(fd, usb_config.bMaxPower);
            usb_raw_configure(fd);
            io->inner.length = 0;
            return true;
        case USB_REQ_GET_INTERFACE:
            io->data[0] = usb_if0.bAlternateSetting;
            io->inner.length = 1;
            return true;
        default:
            printf("fail: no response 2\n");
            exit(EXIT_FAILURE);
        }
        break;
    case USB_TYPE_CLASS:
        switch (event->ctrl.bRequest)
        {
        case HID_REQ_SET_REPORT:
            // This is an OUT request, so don't initialize data.
            io->inner.length = 1;
            return true;
        case HID_REQ_SET_IDLE:
            io->inner.length = 0;
            return true;
        case HID_REQ_SET_PROTOCOL:
            io->inner.length = 0;
            return true;
        default:
            printf("fail: no response 3\n");
            exit(EXIT_FAILURE);
        }
        break;
    case USB_TYPE_VENDOR:
        // Seen;
        // 0x1 -> Can be safely ignored? -> seems like this might be needed on some 3rd party controllers;
        // https://github.com/torvalds/linux/blob/3d5f968a177d468cd13568ef901c5be84d83d32b/drivers/input/joystick/xpad.c#L1755

        // Just ignore for now?

        // switch (event->ctrl.bRequest)
        // {
        // default:
        //     printf("fail: no response 4\n");
        //     exit(EXIT_FAILURE);
        // }
        return true;
        break;
    default:
        printf("fail: no response 5\n");
        exit(EXIT_FAILURE);
    }
}

void ep0_loop(int fd)
{
    while (true)
    {
        struct usb_raw_control_event event;
        event.inner.type = 0;
        event.inner.length = sizeof(event.ctrl);

        // Fetch the first event after init and log it.
        usb_raw_event_fetch(fd, (struct usb_raw_event *)&event);
        log_event((struct usb_raw_event *)&event);

        // Most likely, first event will be the connect event
        if (event.inner.type == USB_RAW_EVENT_CONNECT_2)
        {
            process_eps_info(fd);
            continue;
        }

        // If the event == reset, stop the thread running the interrupt endpoint
        if (event.inner.type == USB_RAW_EVENT_RESET_2)
        {
            if (ep_int_in_thread_spawned)
            {
                printf("ep0: stopping ep_int_in thread\n");
                // Even though normally, on a device reset,
                // the endpoint threads should exit due to
                // ESHUTDOWN, let's also attempt to cancel
                // them just in case.
                pthread_cancel(ep_int_in_thread);
                int rv = pthread_join(ep_int_in_thread, NULL);
                if (rv != 0)
                {
                    perror("pthread_join(ep_int_in)");
                    exit(EXIT_FAILURE);
                }
                usb_raw_ep_disable(fd, ep_int_in);
                ep_int_in_thread_spawned = false;
                printf("ep0: stopped ep_int_in thread\n");
            }
            continue;
        }

        if (event.inner.type != USB_RAW_EVENT_CONTROL_2)
            // No new control event at ep0, continue to next event
            continue;

        struct usb_raw_control_io io;
        io.inner.ep = 0;
        io.inner.flags = 0;
        io.inner.length = 0;

        bool reply = ep0_request(fd, &event, &io);
        if (!reply)
        {
            printf("ep0: stalling\n");
            usb_raw_ep0_stall(fd);
            continue;
        }

        if (event.ctrl.wLength < io.inner.length)
            io.inner.length = event.ctrl.wLength;

        if (event.ctrl.bRequestType & USB_DIR_IN)
        {
            int rv = usb_raw_ep0_write(fd, (struct usb_raw_ep_io *)&io);
            printf("ep0: transferred %d bytes (in)\n", rv);
        }
        else
        {
            int rv = usb_raw_ep0_read(fd, (struct usb_raw_ep_io *)&io);
            printf("ep0: transferred %d bytes (out)\n", rv);
        }
    }
}

int main(int argc, char **argv)
{
    /*Overall USB flow;
    - Open the raw-gadget device
    - Get the device descriptor
    - Give the device an address (UDC will handle this)
    - get the configuration descriptor
    - get the string descriptor
    - set interface to work with
    - set configuration to work with
    */

    // Default device and driver
    const char *device = "dummy_udc.0";
    const char *driver = "dummy_udc";
    if (argc >= 2)
        device = argv[1];
    if (argc >= 3)
        driver = argv[2];

    int fd = usb_raw_open();
    // Initialize the USB device by setting the speed we want to use
    usb_raw_init(fd, USB_SPEED_HIGH, driver, device);
    // Send op '0' to initialize the device
    usb_raw_run(fd);

    // Run the loop to handle the USB events
    ep0_loop(fd);

    close(fd);

    return 0;
}
