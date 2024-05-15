from data import DescriptorDC
import functionfs
import functionfs.ch9
import functionfs.common

# DESCRIPTORS


class UnknownDescriptorIF0(functionfs.common.USBDescriptorHeader):
    """
    Unknown mandatory descriptor for Interface 0
    Possibly a HID descriptor, but that might be misleading.
    // Unknown Descriptor (If0)
    0x11,        // bLength
    0x21,        // bDescriptorType
    0x00, 0x01, 0x01, 0x25,  // ???
    0x81,        // bEndpointAddress (IN, 1)
    0x14,        // bMaxDataSize
    0x00, 0x00, 0x00, 0x00, 0x13,  // ???
    0x01,        // bEndpointAddress (OUT, 1)
    0x08,        // bMaxDataSize
    0x00, 0x00,  // ???
    """
    _bDescriptorType = 0x21
    _fields_ = [
        ("bLength", functionfs.common.u8),
        ("bDescriptorType", functionfs.common.u8),
        # 4 unknown bytes (possibly wrappable as le32?)
        ("bUnknown1", functionfs.common.u8),
        ("bUnknown2", functionfs.common.u8),
        ("bUnknown3", functionfs.common.u8),
        ("bUnknown4", functionfs.common.u8),
        ("bEndpointAddressIN", functionfs.common.u8),
        ("bMaxDataSizeIN", functionfs.common.u8),
        # 5 unknown bytes (possibly wrappable as le32 + u8?)
        ("bUnknown5", functionfs.common.u8),
        ("bUnknown6", functionfs.common.u8),
        ("bUnknown7", functionfs.common.u8),
        ("bUnknown8", functionfs.common.u8),
        ("bUnknown9", functionfs.common.u8),
        ("bEndpointAddressOUT", functionfs.common.u8),
        ("bMaxDataSizeOUT", functionfs.common.u8),
        # 2 unknown bytes (possibly wrappable as le16?)
        ("bUnknown10", functionfs.common.u8),
        ("bUnknown11", functionfs.common.u8),
    ]


DESCRIPTOR_IF0 = DescriptorDC(
    UnknownDescriptorIF0,
    {
        "bLength": 0x11,
        "bDescriptorType": 0x21,  # TODO: Can remove this, maybe?
        "bUnknown1": 0x00,
        "bUnknown2": 0x01,
        "bUnknown3": 0x01,
        "bUnknown4": 0x25,
        "bEndpointAddressIN": 0x81,
        "bMaxDataSizeIN": 0x14,
        "bUnknown5": 0x00,
        "bUnknown6": 0x00,
        "bUnknown7": 0x00,
        "bUnknown8": 0x00,
        "bUnknown9": 0x13,
        "bEndpointAddressOUT": 0x01,
        "bMaxDataSizeOUT": 0x08,
        "bUnknown10": 0x00,
        "bUnknown11": 0x00,
    },
)


class UnknownDescriptorIF1(functionfs.common.USBDescriptorHeader):
    """
    Unknown mandatory descriptor for Interface 1

    // Unknown Descriptor (If1)
    0x1B,        // bLength
    0x21,        // bDescriptorType
    0x00, 0x01, 0x01, 0x01,  // ???
    0x82,        // bEndpointAddress (IN, 2)
    0x40,        // bMaxDataSize
    0x01,        // ???
    0x02,        // bEndpointAddress (OUT, 2)
    0x20,        // bMaxDataSize
    0x16,        // ???
    0x83,        // bEndpointAddress (IN, 3)
    0x00,        // bMaxDataSize
    0x00, 0x00, 0x00, 0x00, 0x00, 0x16,  // ???
    0x03,        // bEndpointAddress (OUT, 3)
    0x00,        // bMaxDataSize
    0x00, 0x00, 0x00, 0x00, 0x00,  // ???
    """

    _bDescriptorType = 0x21
    _fields_ = [
        ("bLength", functionfs.common.u8),
        ("bDescriptorType", functionfs.common.u8),
        # 4 unknown bytes (possibly wrappable as le32?)
        ("bUnknown1", functionfs.common.u8),
        ("bUnknown2", functionfs.common.u8),
        ("bUnknown3", functionfs.common.u8),
        ("bUnknown4", functionfs.common.u8),
        # Endpoint 2 IN
        ("bEndpointAddressIN", functionfs.common.u8),
        ("bMaxDataSizeIN", functionfs.common.u8),
        # 1 unknown byte
        ("bUnknown5", functionfs.common.u8),
        # Endpoint 2 OUT
        ("bEndpointAddressOUT", functionfs.common.u8),
        ("bMaxDataSizeOUT", functionfs.common.u8),
        # 1 unknown byte
        ("bUnknown6", functionfs.common.u8),
        # Endpoint 3 IN
        ("bEndpointAddressIN2", functionfs.common.u8),
        ("bMaxDataSizeIN2", functionfs.common.u8),
        # 6 unknown bytes (possibly wrappable as le32 + u16?)
        ("bUnknown7", functionfs.common.u8),
        ("bUnknown8", functionfs.common.u8),
        ("bUnknown9", functionfs.common.u8),
        ("bUnknown10", functionfs.common.u8),
        ("bUnknown11", functionfs.common.u8),
        ("bUnknown12", functionfs.common.u8),
        # Endpoint 3 OUT
        ("bEndpointAddressOUT2", functionfs.common.u8),
        ("bMaxDataSizeOUT2", functionfs.common.u8),
        # 5 unknown bytes (possibly wrappable as le32 + u8?)
        ("bUnknown13", functionfs.common.u8),
        ("bUnknown14", functionfs.common.u8),
        ("bUnknown15", functionfs.common.u8),
        ("bUnknown16", functionfs.common.u8),
        ("bUnknown17", functionfs.common.u8),
    ]


DESCRIPTOR_IF1 = DescriptorDC(
    UnknownDescriptorIF1,
    {
        "bLength": 0x1B,
        "bDescriptorType": 0x21,  # TODO: Can remove this, maybe?
        "bUnknown1": 0x00,
        "bUnknown2": 0x01,
        "bUnknown3": 0x01,
        "bUnknown4": 0x01,
        "bEndpointAddressIN": 0x82,
        "bMaxDataSizeIN": 0x40,
        "bUnknown5": 0x01,
        "bEndpointAddressOUT": 0x02,
        "bMaxDataSizeOUT": 0x20,
        "bUnknown6": 0x16,
        "bEndpointAddressIN2": 0x83,
        "bMaxDataSizeIN2": 0x00,
        "bUnknown7": 0x00,
        "bUnknown8": 0x00,
        "bUnknown9": 0x00,
        "bUnknown10": 0x00,
        "bUnknown11": 0x00,
        "bUnknown12": 0x16,
        "bEndpointAddressOUT2": 0x03,
        "bMaxDataSizeOUT2": 0x00,
        "bUnknown13": 0x00,
        "bUnknown14": 0x00,
        "bUnknown15": 0x00,
        "bUnknown16": 0x00,
        "bUnknown17": 0x00,
    }
)


class UnknownDescriptorIF2(functionfs.common.USBDescriptorHeader):
    """
    Unknown mandatory descriptor for Interface 2

    bLength: 9
    bDescriptorType: 0x21 (UNKNOWN)
    ???: 0x00, 0x01, 0x01
    ???: 0x22
    bEndpointAddress: 0x84 IN Endpoint: 4
    bMaxDataSize: 7
    ???: 0x00
    Raw Hex: 0x09 0x21 0x00 0x01 0x01 0x22 0x84 0x07 0x00
    """

    _bDescriptorType = 0x21
    _fields_ = [
        ("bLength", functionfs.common.u8),
        ("bDescriptorType", functionfs.common.u8),
        # 4 unknown bytes (possibly wrappable as le32?)
        ("bUnknown1", functionfs.common.u8),
        ("bUnknown2", functionfs.common.u8),
        ("bUnknown3", functionfs.common.u8),
        ("bUnknown4", functionfs.common.u8),
        # Endpoint 4 IN
        ("bEndpointAddressIN", functionfs.common.u8),
        ("bMaxDataSizeIN", functionfs.common.u8),
        # 1 unknown byte
        ("bUnknown5", functionfs.common.u8),
    ]


DESCRIPTOR_IF2 = DescriptorDC(
    UnknownDescriptorIF2,
    {
        "bLength": 0x09,
        "bDescriptorType": 0x21,  # TODO: Can remove this, maybe?
        "bUnknown1": 0x00,
        "bUnknown2": 0x01,
        "bUnknown3": 0x01,
        "bUnknown4": 0x22,
        "bEndpointAddressIN": 0x84,
        "bMaxDataSizeIN": 0x07,
        "bUnknown5": 0x00,
    }
)


class UnknownDescriptorIF3(functionfs.common.USBDescriptorHeader):
    """
    Unknown mandatory descriptor for Interface 3

    bLength: 6
    bDescriptorType: 0x41 (UNKNOWN)
    ???: 0x00, 0x01, 0x01
    ???: 0x03
    Raw Hex: 0x06 0x41 0x00 0x01 0x01 0x03
    """

    _bDescriptorType = 0x41
    _fields_ = [
        ("bLength", functionfs.common.u8),
        ("bDescriptorType", functionfs.common.u8),
        # 4 unknown bytes (possibly wrappable as le32?)
        ("bUnknown1", functionfs.common.u8),
        ("bUnknown2", functionfs.common.u8),
        ("bUnknown3", functionfs.common.u8),
        ("bUnknown4", functionfs.common.u8),
    ]


DESCRIPTOR_IF3 = DescriptorDC(
    UnknownDescriptorIF3,
    {
        "bLength": 0x06,
        "bDescriptorType": 0x41,  # TODO: Can remove this, maybe?
        "bUnknown1": 0x00,
        "bUnknown2": 0x01,
        "bUnknown3": 0x01,
        "bUnknown4": 0x03,
    }
)
