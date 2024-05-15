from data import GadgetDC

from xboxDev.xboxInterfaces import IF0, IF1, IF2, IF3

XBOXGADGET = GadgetDC(
    {
        "bcdUSB": 0x0200,
        "bDeviceClass": 0xFF,  # Vendor specific
        "bDeviceSubClass": 0xFF,
        "bDeviceProtocol": 0xFF,
        "idVendor": 0x045E,  # Microsoft Corp.
        "idProduct": 0x028E,  # Xbox Controller
        "bcdDevice": 0x0114,
        "name": "xbox",
    },
    {
        0x409: {
            "manufacturer": "©Microsoft Corporation",
            "product": "Controller",
            "serialnumber": "08FEC93",
            "0x04": "Xbox Security Method 3, Version 1.00, © 2005 Microsoft Corporation. All rights reserved.",  # TODO pretty sure this is omitted by the functionfs library...?
        },
    },
    250,  # 500mA
    0b10100000,  # (NOT SELF-POWERED, REMOTE-WAKEUP)
    [IF0, IF1, IF2, IF3],
)
