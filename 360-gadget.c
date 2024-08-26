#include "360-gadget.h"

/*----------------------------------------------------------------------*/
// Endpoint thread.

// #include <pthread.h>
// pthread_t ep_int_in_thread;
// bool ep_int_in_thread_spawned = false;

// void *ep_int_in_loop(void *arg)
// {
//     // INPUT
//     // Loop to keep sending data to the interrupt endpoint
//     int fd = (int)(long)arg;

//     struct usb_raw_interrupt_ep1_io io;
//     io.inner.ep = ep_int_in;
//     io.inner.flags = 0;
//     io.inner.length = 20;

//     while (true)
//     {
//         // A button press on the controller;
//         // 0x00, 0x14, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
//         // Blank input -> 20x 0x00
//         // Send button press 'A' (0x1b)
//         memcpy(&io.inner.data[0],
//                "\x00\x14\x00\x10\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00", 20);
//         int rv = usb_raw_ep_write_may_fail(fd,
//                                            (struct usb_raw_ep_io *)&io);
//         if (rv < 0 && errno == ESHUTDOWN)
//         {
//             // If the device was reset, exit the loop
//             printf("ep_int_in: device was likely reset, exiting\n");
//             break;
//         }
//         else if (rv < 0)
//         {
//             // Device busy?
//             // TODO: should handle this better/try again
//             perror("usb_raw_ep_write_may_fail()");
//             exit(EXIT_FAILURE);
//         }
//         printf("ep_int_in: key down: %d\n", rv);
//         sleep(1);
//         // Reset inputs.
//         memcpy(&io.inner.data[0],
//                "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00", 20);
//         // TODO: Same as above, should handle this better
//         rv = usb_raw_ep_write_may_fail(fd, (struct usb_raw_ep_io *)&io);
//         if (rv < 0 && errno == ESHUTDOWN)
//         {
//             printf("ep_int_in: device was likely reset, exiting\n");
//             break;
//         }
//         else if (rv < 0)
//         {
//             perror("usb_raw_ep_write_may_fail()");
//             exit(EXIT_FAILURE);
//         }
//         printf("ep_int_in: key up: %d\n", rv);
//         sleep(1);
//     }

//     return NULL;
// }

/*----------------------------------------------------------------------*/

// Endpoint addr (- = first available?)
int ep_int_in = -1;
bool ep_int_enabled = false;
bool send_to_ep1(int fd, char *data, int len)
{
    if (!ep_int_enabled)
    {
        printf("ep_int_in not enabled / available\n");
        return false;
    }
    struct usb_raw_interrupt_ep1_io io;
    io.inner.ep = ep_int_in;
    io.inner.flags = 0;
    io.inner.length = len;
    memcpy(&io.inner.data[0], data, len);
    int rv = usb_raw_ep_write(fd, (struct usb_raw_ep_io *)&io);
    if (rv < 0)
    {
        perror("usb_raw_ep_write()");
        return false;
    }
    return true;
}

// Run.
bool ep0_request(int fd, struct usb_raw_control_ep0_event *event,
                 struct usb_raw_control_ep0_io *io)
{
    // Handle the control request
    switch (event->ctrl.bRequestType & USB_TYPE_MASK)
    {
    case USB_TYPE_STANDARD:
        switch (event->ctrl.bRequest)
        {
        case USB_REQ_GET_DESCRIPTOR:
            switch (event->ctrl.wValue >> 8)
            {
            case USB_DT_DEVICE:
                // Device descriptor request
                memcpy(&io->data[0], &usb_device,
                       sizeof(usb_device));
                io->inner.length = sizeof(usb_device);
                return true;
            case USB_DT_DEVICE_QUALIFIER:
                // Device full speed descriptor request
                memcpy(&io->data[0], &usb_qualifier,
                       sizeof(usb_qualifier));
                io->inner.length = sizeof(usb_qualifier);
                return true;
            case USB_DT_CONFIG:
                // Configuration descriptor request
                io->inner.length =
                    build_config(&io->data[0],
                                 sizeof(io->data), false);
                return true;
            case USB_DT_OTHER_SPEED_CONFIG:
                // Configuration full speed descriptor request
                io->inner.length =
                    build_config(&io->data[0],
                                 sizeof(io->data), true);
                return true;
            case USB_DT_STRING:
                // String descriptor request
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
            default:
                printf("fail: no/unknown descriptor response\n");
                exit(EXIT_FAILURE);
            }
            break;
        case USB_REQ_SET_CONFIGURATION:
            // Set configuration request (endpoint 1?)
            printf("SET CONFIGURATION\n");
            printf("event->ctrl.wValue: %d\n", event->ctrl.wValue);
            ep_int_in = usb_raw_ep_enable(fd, &usb_if0_ep1_in);
            printf("ep0: ep_int_in enabled: %d\n", ep_int_in);
            ep_int_enabled = true;
            usb_raw_vbus_draw(fd, usb_config.bMaxPower);
            usb_raw_configure(fd);
            io->inner.length = 0;
            return true;
        case USB_REQ_GET_INTERFACE:
            io->data[0] = usb_if0.bAlternateSetting;
            io->inner.length = 1;
            return true;
        default:
            printf("fail: no/unknown configuration response\n");
            exit(EXIT_FAILURE);
        }
        break;
    case USB_TYPE_VENDOR:
        // Seen;
        // 0x1 -> Can be safely ignored? -> seems like this might be needed on some 3rd party controllers;
        // https://github.com/torvalds/linux/blob/3d5f968a177d468cd13568ef901c5be84d83d32b/drivers/input/joystick/xpad.c#L1755
        // Ignored for now.
        return true;
        break;
    default:
        printf("fail: no/unknown response\n");
        exit(EXIT_FAILURE);
    }
}

void ep0_single_loop(int fd)
{
    // ep0 logic, should be run in an event loop
    struct usb_raw_control_ep0_event event;
    event.inner.type = 0;
    event.inner.length = sizeof(event.ctrl);

    // Fetch event and log it.
    usb_raw_event_fetch(fd, (struct usb_raw_event *)&event);
    log_event((struct usb_raw_event *)&event);

    // Most likely, first event will be the connect event
    if (event.inner.type == USB_RAW_EVENT_CONNECT_)
    {
        process_eps_info(fd);
        return;
    }

    // If the event == reset, stop the thread running the interrupt endpoint
    if (event.inner.type == USB_RAW_EVENT_RESET_)
    {
        if (ep_int_enabled)
        {
            printf("ep0: disabling ep1 IN\n");
            // On reset, disable the interrupt endpoint
            usb_raw_ep_disable(fd, ep_int_in);
            ep_int_enabled = false;
            printf("ep0: disabled ep1 IN\n");
        }
        return;
    }

    if (event.inner.type != USB_RAW_EVENT_CONTROL_)
        // No new control event at ep0, continue to next event
        return;

    struct usb_raw_control_ep0_io io;
    io.inner.ep = 0;
    io.inner.flags = 0;
    io.inner.length = 0;

    bool reply = ep0_request(fd, &event, &io);
    if (!reply)
    {
        printf("ep0: stalling\n");
        usb_raw_ep0_stall(fd);
        return;
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

void ep0_loop_example(int fd)
{
    while (true)
    {
        ep0_single_loop(fd);
        // USB usually has a 1ms polling rate, so sleep for a little less than that
        usleep(900);

        if (ep_int_enabled)
        {
            // Send A button press to the interrupt endpoint
            send_to_ep1(fd, "\x00\x14\x00\x10\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00", 20);
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
    const char *device = USB_RAW_GADGET_DEVICE_DEFAULT;
    const char *driver = USB_RAW_GADGET_DRIVER_DEFAULT;

    printf("Using device: %s\n", device);
    printf("Using driver: %s\n", driver);

    int fd = usb_raw_open();
    // Initialize the USB device by setting the speed we want to use
    usb_raw_init(fd, USB_SPEED_HIGH, driver, device);
    // Send OP '0' to initialize the device
    usb_raw_run(fd);

    // Run the loop to handle the USB events
    ep0_loop_example(fd);

    close(fd);

    return 0;
}
