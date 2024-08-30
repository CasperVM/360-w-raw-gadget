#include "360-w-gadget.h"

// Endpoint control block
bool ep_int_enabled = false;

// Handle ep0 control requests
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
                    set_usb_string_desc("Xbox 360 Wireless Receiver for Windows", io);
                    break;
                case STRING_ID_SERIAL:
                    // Serial number string descriptor
                    set_usb_string_desc("08FEC93", io); // TODO: Never gets asked from host?
                    break;
                    return true;
                }
            default:
                // Unknown descriptor response, ignore.
                return true;
            }
            break;
        case USB_REQ_SET_CONFIGURATION:
            // Set configuration request
            printf("SET CONFIGURATION\n");
            printf("event->ctrl.wValue: %d\n", event->ctrl.wValue);

            int i;
            for (i = 0; i < n_interfaces; i++)
            {
                struct if_full_struct interface_x = get_if_x(i);
                // ep in
                int ep_int_in = usb_raw_ep_enable(fd, &interface_x.ep_in);
                printf("ep0: ep_int_in enabled: %d\n", ep_int_in);
                // // ep out
                // int ep_int_out = usb_raw_ep_enable(fd, &interface_x.ep_out);
                // printf("ep0: ep_int_out enabled: %d\n", ep_int_out);
            }
            ep_int_enabled = true;
            // Power settings
            usb_raw_vbus_draw(fd, usb_config.bMaxPower);
            usb_raw_configure(fd);
            io->inner.length = 0;
            return true;
        case USB_REQ_GET_INTERFACE:
            io->data[0] = usb_if_xinput.bAlternateSetting;
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
    default:
        printf("fail: no/unknown response\n");
        exit(EXIT_FAILURE);
    }
}

pthread_t ep0_thread;
void *ep0_loop(void *arg)
{
    // Loop to handle the control endpoint
    // This is put in a separate thread as sometimes the control endpoint is IO blocked/halts entire program
    int fd = (int)(long)arg;
    while (true)
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
            continue;
        }

        // If the event == reset, stop the thread running the interrupt endpoint
        if (event.inner.type == USB_RAW_EVENT_RESET_)
        {
            // if (ep_int_in_thread_spawned)
            if (ep_int_enabled)
            {
                printf("ep0: disabling ep1 IN\n");
                // On reset, disable all endpoints
                int i;
                for (i = 0; i < n_interfaces; i++)
                {
                    struct if_full_struct interface_x = get_if_x(i);
                    // ep in
                    int ep_int_in = usb_endpoint_num(&interface_x.ep_in);
                    usb_raw_ep_disable(fd, ep_int_in);
                    printf("ep0: ep_int_in enabled: %d\n", ep_int_in);
                    // // ep out
                    // int ep_int_out = usb_endpoint_num(&interface_x.ep_out);
                    // usb_raw_ep_disable(fd, ep_int_out);
                    // printf("ep0: ep_int_out enabled: %d\n", ep_int_out);
                }
                ep_int_enabled = false;
            }
            continue;
        }

        if (event.inner.type != USB_RAW_EVENT_CONTROL_)
            // No new control event at ep0, continue to next event
            continue;

        struct usb_raw_control_ep0_io io;
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

// PUBLIC FUNCTIONS
bool gadget_initialized = false;
int init_360_gadget(bool await_endpoint_availability)
{
    // Initialize the USB device
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
    // Start the ep0 loop
    // Spawn ep0
    int rv = pthread_create(&ep0_thread, 0, ep0_loop, (void *)(long)fd);
    if (rv != 0)
    {
        perror("pthread_create(ep0)");
        exit(EXIT_FAILURE);
    }

    gadget_initialized = true;

    if (await_endpoint_availability)
    {
        // Wait for the interrupt endpoint to be available
        while (!ep_int_enabled)
        {
            usleep(1000);
        }
    }

    return fd;
}

void close_360_gadget(int fd)
{
    if (gadget_initialized)
    {
        // Cancel the ep0 thread
        int rv = pthread_cancel(ep0_thread);
        if (rv != 0)
        {
            perror("pthread_cancel(ep0)");
            exit(EXIT_FAILURE);
        }
        // Close the file descriptor
        close(fd);
    }
}

bool send_to_ep(int fd, int n, char *data, int len)
{
    if (!ep_int_enabled)
    {
        printf("ep_int_in not enabled / available\n");
        return false;
    }
    struct if_full_struct interface_x = get_if_x(n);
    
    // FIXME: `usb_endpoint_num` does NOT return the correct address?
    // int ep_int_in = usb_endpoint_num(&interface_x.ep_in);

    // Lets for now infer the endpoint address (0, 2, 4...)
    // That at least seems to be how these are assigned;
    // I'm to lazy to retrieve it properly from the `usb_raw_ep_enable`

    int ep_int_in = n*2;

    struct usb_raw_interrupt_ep_io io;
    io.inner.ep = ep_int_in;
    io.inner.flags = 0;
    io.inner.length = len;
    printf("ep_int_in: sending %d bytes for interface %d with addr %d \n", len, n, ep_int_in);
    memcpy(&io.inner.data[0], data, len);
    int rv = usb_raw_ep_write_may_fail(fd, (struct usb_raw_ep_io *)&io);
    if (rv < 0)
    {
        perror("usb_raw_ep_write_may_fail()");
        return false;
    }
    return true;
}

bool set_n_interfaces(int n)
{
    // 4 Seems to be the hard limit with this device, otherwise USB_RAW_IOCTL_EP_WRITE will stall.
    // Possibly driver ignoring input? / Maybe the custom vendor descriptor would need changing?
    if (n < 1 || n > 4) {
        return false;
    }
        
    n_interfaces = n;
    return true;
}

void gadget_example()
{
    // Set the amount of interfaces
    set_n_interfaces(4);
    int fd = init_360_gadget(true);

    // Event loop
    int time_passed = 0;
    char *a_packet = "\x00\x14\x00\x10\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00";
    char *blank_packet = "\x00\x14\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00";
    bool a_pressed = false;
    while (true)
    {

        // USB usually has a 1ms polling rate, so sleep for a little less than that
        usleep(900);
        time_passed += 1;

        if (ep_int_enabled && time_passed >= 1000)
        {
            if (a_pressed)
            {
                printf("A DOWN\n");
                // Send a blank packet to the interrupt endpoint
                int i;
                for (i = 0; i < n_interfaces; i++)
                {
                    send_to_ep(fd, i, blank_packet, 20);
                }

                a_pressed = false;
            }
            else
            {
                printf("NO INPUT\n");
                // Send A button press to the interrupt endpoint
                int i;
                for (i = 0; i < n_interfaces; i++)
                {
                    send_to_ep(fd, i, a_packet, 20);
                }

                a_pressed = true;
            }
            time_passed = 0;
        }
    }

    close_360_gadget(fd);
}