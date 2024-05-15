import functionfs
from functionfs import getDescriptor
from functionfs.gadget import (
    Gadget,
    GadgetSubprocessManager,
    ConfigFunctionFFSSubprocess,
    ConfigFunctionFFS
)
import functionfs.ch9
import functionfs.common
from xboxDev.xboxDev import XBOXGADGET
from xboxDev.xboxEndpoints import (
    IF0_EP1_IN,
    IF0_EP1_OUT,
    IF1_EP2_IN,
    IF1_EP2_OUT,
    IF1_EP3_IN,
    IF1_EP3_OUT,
    IF2_EP4_IN,
)


class IF0(functionfs.Function):
    """
    Xbox Controller Interface 0
    """
    
    
    def __init__(self, path, writer, onCanSend, onCannotSend):
        fs_list, hs_list, ss_list = functionfs.getInterfaceInAllSpeeds(
            interface=XBOXGADGET.interfaces[0].fs_dict,
            class_descriptor_list=[
                XBOXGADGET.interfaces[0].descriptors[0].getDescriptor()
            ],
            endpoint_list=[
                IF0_EP1_IN.fs_dict,
                IF0_EP1_OUT.fs_dict,
            ],
        )
        
        super().__init__(
            path,
            fs_list=fs_list,
            hs_list=hs_list,
            ss_list=ss_list,
            lang_dict=XBOXGADGET.lang_dict,
        )

    # def send_button_packet(self):
    #     # Construct a button packet, example: button A pressed
    #     # Button packets are typically specific to the device's USB protocol
    #     # 0x0014001000000000000000000000000000000000
    #     button_packet = [
    #         0x00, 0x14, 0x00, 0x10, 0x00,
    #         0x00, 0x00, 0x00, 0x00, 0x00,
    #         0x00, 0x00, 0x00, 0x00, 0x00,
    #         0x00, 0x00, 0x00, 0x00, 0x00,
    #     ]

    #     with open(self.ep_in, 'wb') as f:
    #         f.write(bytearray(button_packet))


class IF1(functionfs.Function):
    """
    Xbox Controller Interface 1
    Headset/Expansion Port Data
    """

    def __init__(self, path, writer, onCanSend, onCannotSend):
        fs_list, hs_list, ss_list = functionfs.getInterfaceInAllSpeeds(
            interface=XBOXGADGET.interfaces[1].fs_dict,
            class_descriptor_list=[
                XBOXGADGET.interfaces[1].descriptors[0].getDescriptor()
            ],
            endpoint_list=[
                IF1_EP2_IN.fs_dict,
                IF1_EP2_OUT.fs_dict,
                IF1_EP3_IN.fs_dict,
                IF1_EP3_OUT.fs_dict,
            ],
        )
        super().__init__(path, fs_list, hs_list, ss_list, lang_dict=XBOXGADGET.lang_dict)


class IF2(functionfs.Function):
    """
    Xbox Controller Interface 2
    Unknown Endpoint
    """

    def __init__(self, path, writer, onCanSend, onCannotSend):
        fs_list, hs_list, ss_list = functionfs.getInterfaceInAllSpeeds(
            interface=XBOXGADGET.interfaces[2].fs_dict,
            class_descriptor_list=[
                XBOXGADGET.interfaces[2].descriptors[0].getDescriptor()
            ],
            endpoint_list=[
                IF2_EP4_IN.fs_dict,
            ],
        )
        super().__init__(path, fs_list, hs_list, ss_list, lang_dict=XBOXGADGET.lang_dict)


class IF3(functionfs.Function):
    """
    Xbox Controller Interface 3
    Security Method
    """
    def __init__(self, path, writer, onCanSend, onCannotSend):
        fs_list, hs_list, ss_list = functionfs.getInterfaceInAllSpeeds(
            interface=XBOXGADGET.interfaces[3].fs_dict,
            class_descriptor_list=[
                XBOXGADGET.interfaces[3].descriptors[0].getDescriptor()
            ],
            endpoint_list=[
                # No endpoints
            ],
        )
        super().__init__(path, fs_list, hs_list, ss_list, lang_dict=XBOXGADGET.lang_dict)





def main():
    def if0(**kw):
        return ConfigFunctionFFS(getFunction=IF0, **kw)
    
    def if1(**kw):
        return ConfigFunctionFFS(getFunction=IF1, **kw)
    
    def if2(**kw):
        return ConfigFunctionFFS(getFunction=IF2, **kw)
    
    def if3(**kw):
        return ConfigFunctionFFS(getFunction=IF3, **kw)

    with Gadget(
            config_list=[XBOXGADGET.getConfigList([
                if0,
                if1,
                if2,
                if3,
            ])],
            lang_dict=XBOXGADGET.lang_dict,
            **XBOXGADGET.fs_dict
        ) as gadget:
        # Assume that the path is set, and each interface can initialize
        if0 = IF0(gadget.path, gadget.writer, gadget.onCanSend, gadget.onCannotSend)
        if1 = IF1(gadget.path, gadget.writer, gadget.onCanSend, gadget.onCannotSend)
        if2 = IF2(gadget.path, gadget.writer, gadget.onCanSend, gadget.onCannotSend)
        if3 = IF3(gadget.path, gadget.writer, gadget.onCanSend, gadget.onCannotSend)
        
        # FIXME: This setup doesn't work, using the wrong functionfs classes/functions?...
        gadget.waitForever()
    
if __name__ == "__main__":
    main()