from data import InterfaceDC
import functionfs
import functionfs.ch9
import functionfs.common
from xboxDev.xboxDescriptors import (
    DESCRIPTOR_IF0,
    DESCRIPTOR_IF1,
    DESCRIPTOR_IF2,
    DESCRIPTOR_IF3,
)
from xboxDev.xboxEndpoints import (
    IF0_EP1_IN,
    IF0_EP1_OUT,
    IF1_EP2_IN,
    IF1_EP2_OUT,
    IF1_EP3_IN,
    IF1_EP3_OUT,
    IF2_EP4_IN,
)

# INTERFACES

IF0: InterfaceDC = InterfaceDC(
    {
        "bInterfaceNumber": 0,
        "bAlternateSetting": 0,
        "bNumEndpoints": 2,
        "bInterfaceClass": functionfs.ch9.USB_CLASS_VENDOR_SPEC,
        "bInterfaceSubClass": 0x5D,
        "bInterfaceProtocol": 0x01,
        "iInterface": 0,
    },
    [DESCRIPTOR_IF0],
    [IF0_EP1_IN, IF0_EP1_OUT],
)

IF1 = InterfaceDC(
    {
        "bInterfaceNumber": 1,
        "bAlternateSetting": 0,
        "bNumEndpoints": 2,
        "bInterfaceClass": functionfs.ch9.USB_CLASS_VENDOR_SPEC,
        "bInterfaceSubClass": 0x5D,
        "bInterfaceProtocol": 0x03,
        "iInterface": 0,
    },
    [DESCRIPTOR_IF1],
    [IF1_EP2_IN, IF1_EP2_OUT, IF1_EP3_IN, IF1_EP3_OUT],
)

IF2 = InterfaceDC(
    {
        "bInterfaceNumber": 2,
        "bAlternateSetting": 0,
        "bNumEndpoints": 1,
        "bInterfaceClass": functionfs.ch9.USB_CLASS_VENDOR_SPEC,
        "bInterfaceSubClass": 0x5D,
        "bInterfaceProtocol": 0x02,
        "iInterface": 0,
    },
    [DESCRIPTOR_IF2],
    [IF2_EP4_IN],
)

IF3 = InterfaceDC(
    {
        "bInterfaceNumber": 3,
        "bAlternateSetting": 0,
        "bNumEndpoints": 0,
        "bInterfaceClass": functionfs.ch9.USB_CLASS_VENDOR_SPEC,
        "bInterfaceSubClass": 0xFD,
        "bInterfaceProtocol": 0x13,
        "iInterface": 4,
    },
    [DESCRIPTOR_IF3],
    [],
)
