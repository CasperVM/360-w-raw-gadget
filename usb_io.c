#include "usb_io.h"

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
