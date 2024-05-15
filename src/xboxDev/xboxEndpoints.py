from data import EndpointDC
import functionfs
import functionfs.ch9
import functionfs.common

# ENDPOINTS

# IF0 (2 endpoints)
IF0_EP1_IN = EndpointDC(
    {
        "bEndpointAddress": 0x81,  # IN
        "bmAttributes": functionfs.ch9.USB_ENDPOINT_XFERTYPE_MASK,
        "wMaxPacketSize": 0x0020,
        "bInterval": 0x04,
    }
)

IF0_EP1_OUT = EndpointDC(
    {
        "bEndpointAddress": 0x01,  # OUT
        "bmAttributes": functionfs.ch9.USB_ENDPOINT_XFERTYPE_MASK,
        "wMaxPacketSize": 0x0020,
        "bInterval": 0x08,
    }
)
# IF1 (4 endpoints)
IF1_EP2_IN = EndpointDC(
    {
        "bEndpointAddress": 0x82,  # IN 2
        "bmAttributes": functionfs.ch9.USB_ENDPOINT_XFERTYPE_MASK,
        "wMaxPacketSize": 0x0020,
        "bInterval": 0x02,
    }
)

IF1_EP2_OUT = EndpointDC(
    {
        "bEndpointAddress": 0x02,  # OUT
        "bmAttributes": functionfs.ch9.USB_ENDPOINT_XFERTYPE_MASK,
        "wMaxPacketSize": 0x0020,
        "bInterval": 0x04,
    }
)

IF1_EP3_IN = EndpointDC(
    {
        "bEndpointAddress": 0x83,  # IN 3
        "bmAttributes": functionfs.ch9.USB_ENDPOINT_XFERTYPE_MASK,
        "wMaxPacketSize": 0x0020,
        "bInterval": 0x40,
    }
)

IF1_EP3_OUT = EndpointDC(
    {
        "bEndpointAddress": 0x03,  # OUT
        "bmAttributes": functionfs.ch9.USB_ENDPOINT_XFERTYPE_MASK,
        "wMaxPacketSize": 0x0020,
        "bInterval": 0x10,
    }
)

# IF2 (1 endpoints)
IF2_EP4_IN = EndpointDC(
    {
        "bEndpointAddress": 0x84,  # IN 4
        "bmAttributes": functionfs.ch9.USB_ENDPOINT_XFERTYPE_MASK,
        "wMaxPacketSize": 0x0020,
        "bInterval": 0x10,
    }
)

# IF3 (0 endpoints!)
