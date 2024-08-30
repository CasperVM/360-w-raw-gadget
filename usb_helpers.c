#include "usb_helpers.h"
volatile int n_interfaces = 1;

struct if_full_struct get_if_x(int N)
{
    // Get the full interface for N
    // Interfaces start at 0x00
    // Endpoints start at 0x01

    // TODO: set max?/validation check
    struct usb_interface_descriptor usb_if_x = usb_if_xinput;
    usb_if_x.bInterfaceNumber = N;

    struct if_unknown_desc_control_surface if_ud_x = if_ud;
    if_ud_x.bEndpointAddressIn = USB_DIR_IN | (N + 1);
    if_ud_x.bEndpointAddressOut = USB_DIR_OUT | (N + 1);

    struct usb_endpoint_descriptor usb_ep_in_x = usb_ep_in;
    usb_ep_in_x.bEndpointAddress = USB_DIR_IN | (N + 1);

    struct usb_endpoint_descriptor usb_ep_out_x = usb_ep_out;
    usb_ep_out_x.bEndpointAddress = USB_DIR_OUT | (N + 1);

    struct if_full_struct if_full = {
        .interface = usb_if_x,
        .ud = if_ud_x,
        .ep_in = usb_ep_in_x,
        .ep_out = usb_ep_out_x};
    return if_full;
}

int build_config(char *data, int length, bool other_speed)
{
    // Build the configuration descriptor.
    // Contains the device descriptor, interface descriptor, custom desc and endpoint descriptor.
    struct usb_config_descriptor *config =
        (struct usb_config_descriptor *)data;
    int total_length = 0;

    // add config desc
    assert(length >= sizeof(usb_config));
    memcpy(data, &usb_config, sizeof(usb_config));
    data += sizeof(usb_config);
    length -= sizeof(usb_config);
    total_length += sizeof(usb_config);

    // Set N interfaces..
    int i;
    for (i = 0; i < n_interfaces; i++)
    {
        struct if_full_struct interface_x = get_if_x(i);
        // add interface
        assert(length >= sizeof(interface_x.interface));
        memcpy(data, &interface_x.interface, sizeof(interface_x.interface));
        data += sizeof(interface_x.interface);
        length -= sizeof(interface_x.interface);
        total_length += sizeof(interface_x.interface);
        // add unknown desc
        assert(length >= sizeof(interface_x.ud));
        memcpy(data, &interface_x.ud, sizeof(interface_x.ud));
        data += sizeof(interface_x.ud);
        length -= sizeof(interface_x.ud);
        total_length += sizeof(interface_x.ud);
        // ep in
        assert(length >= USB_DT_ENDPOINT_SIZE);
        memcpy(data, &interface_x.ep_in, USB_DT_ENDPOINT_SIZE);
        data += USB_DT_ENDPOINT_SIZE;
        length -= USB_DT_ENDPOINT_SIZE;
        total_length += USB_DT_ENDPOINT_SIZE;
        // ep out
        assert(length >= USB_DT_ENDPOINT_SIZE);
        memcpy(data, &interface_x.ep_out, USB_DT_ENDPOINT_SIZE);
        data += USB_DT_ENDPOINT_SIZE;
        length -= USB_DT_ENDPOINT_SIZE;
        total_length += USB_DT_ENDPOINT_SIZE;
    }

    // Adjust config
    config->bNumInterfaces = n_interfaces;
    config->wTotalLength = __cpu_to_le16(total_length);
    printf("config->wTotalLength: %d\n", total_length);

    if (other_speed)
        config->bDescriptorType = USB_DT_OTHER_SPEED_CONFIG;

    return total_length;
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
        struct if_full_struct current_if = get_if_x(i);
        assign_ep_address(&info.eps[i], &current_if.ep_in);
        // assign_ep_address(&info.eps[i], &current_if.ep_out);
        int in_addr = usb_endpoint_num(&current_if.ep_in);
        assert(in_addr != 0);
        printf("ep_int_in: addr = %u\n", in_addr);
        // int out_addr = usb_endpoint_num(&current_if.ep_out);
        // assert(out_addr != 0);
        // printf("ep_int_out: addr = %u\n", out_addr);
    }
}

void set_usb_string_desc(const char *str, struct usb_raw_control_ep0_io *io)
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
    default:
        printf("  req = unknown = 0x%x\n", ctrl->bRequest);
        break;
    }
}

void log_event(struct usb_raw_event *event)
{
    switch (event->type)
    {
    case USB_RAW_EVENT_CONNECT_:
        printf("event: connect, length: %u\n", event->length);
        break;
    case USB_RAW_EVENT_CONTROL_:
        printf("event: control, length: %u\n", event->length);
        log_control_request((struct usb_ctrlrequest *)&event->data[0]);
        break;
    case USB_RAW_EVENT_SUSPEND_:
        printf("event: suspend\n");
        break;
    case USB_RAW_EVENT_RESUME_:
        printf("event: resume\n");
        break;
    case USB_RAW_EVENT_RESET_:
        printf("event: reset\n");
        break;
    case USB_RAW_EVENT_DISCONNECT_:
        printf("event: disconnect\n");
        break;
    default:
        printf("event: %d (unknown), length: %u\n", event->type, event->length);
    }
}
