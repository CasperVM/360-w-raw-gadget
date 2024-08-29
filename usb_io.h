#include <assert.h>
#include <fcntl.h>
#include <string.h>

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include <sys/ioctl.h>

#include <linux/usb/ch9.h>
#include <linux/usb/raw_gadget.h>

// Max sizes
#define EP0_MAX_DATA 512
// EP1, control surface -> 32 bytes max. (in actuality, 20 bytes get sent)
#define EP1_MAX_PACKET_INT __constant_cpu_to_le16(0x0020)

struct usb_raw_control_ep0_event
{
    struct usb_raw_event inner;
    struct usb_ctrlrequest ctrl;
};
extern struct usb_raw_control_ep0_event usb_raw_control_ep0_event;

struct usb_raw_control_ep0_io
{
    struct usb_raw_ep_io inner;
    char data[EP0_MAX_DATA];
};
extern struct usb_raw_control_ep0_io usb_raw_control_ep0_io;

struct usb_raw_interrupt_ep1_io
{
    struct usb_raw_ep_io inner;
    char data[EP1_MAX_PACKET_INT];
};
extern struct usb_raw_interrupt_ep1_io usb_raw_interrupt_ep1_io;

// taken from latest raw_gadget.h -> needed to rename the enum to avoid conflict (yay)
// Depending on the kernel version, the enum might be different. That's why I'm copying it here.
enum usb_raw_event_type_
{
    USB_RAW_EVENT_INVALID_ = 0,

    /* This event is queued when the driver has bound to a UDC. */
    USB_RAW_EVENT_CONNECT_ = 1,

    /* This event is queued when a new control request arrived to ep0. */
    USB_RAW_EVENT_CONTROL_ = 2,

    /*
     * These events are queued when the gadget driver is suspended,
     * resumed, reset, or disconnected. Note that some UDCs (e.g. dwc2)
     * report a disconnect event instead of a reset.
     */
    USB_RAW_EVENT_SUSPEND_ = 3,
    USB_RAW_EVENT_RESUME_ = 4,
    USB_RAW_EVENT_RESET_ = 5,
    USB_RAW_EVENT_DISCONNECT_ = 6,

    /* The list might grow in the future. */
};

// IO functions
int usb_raw_open();
void usb_raw_init(int fd, enum usb_device_speed speed,
                  const char *driver, const char *device);
void usb_raw_run(int fd);
void usb_raw_event_fetch(int fd, struct usb_raw_event *event);
int usb_raw_ep0_read(int fd, struct usb_raw_ep_io *io);
int usb_raw_ep0_write(int fd, struct usb_raw_ep_io *io);
int usb_raw_ep_enable(int fd, struct usb_endpoint_descriptor *desc);
int usb_raw_ep_disable(int fd, int ep);
int usb_raw_ep_read(int fd, struct usb_raw_ep_io *io);
int usb_raw_ep_write(int fd, struct usb_raw_ep_io *io);
int usb_raw_ep_write_may_fail(int fd, struct usb_raw_ep_io *io);
void usb_raw_configure(int fd);
void usb_raw_vbus_draw(int fd, uint32_t power);
int usb_raw_eps_info(int fd, struct usb_raw_eps_info *info);
void usb_raw_ep0_stall(int fd);
void usb_raw_ep_set_halt(int fd, int ep);
bool assign_ep_address(struct usb_raw_ep_info *info,
                       struct usb_endpoint_descriptor *ep);
