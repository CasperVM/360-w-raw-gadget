#include "usb_helpers.h"

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
