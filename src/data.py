import functionfs.common
from dataclasses import dataclass
from functionfs import getDescriptor as getDescriptorFS

"""
These are abstractions that are not directly comptatible with functionfs

fs_dict is maybe a bit misleading, but practically it's a dict of the field descriptions : int value.
These can be used in various functionfs functions to get other objs
"""


@dataclass
class GadgetDC:
    fs_dict: dict[
        str, int
    ]  # int here might be a specific ctype, but we can ignore that here.
    lang_dict: dict[int, dict[str, str]]
    maxPower: int
    bmAttributes: int
    interfaces: list["InterfaceDC"]

    def getConfigList(self, function_list):
        return {
            "function_list": function_list,
            "MaxPower": self.maxPower,
            "bmAttributes": self.bmAttributes,
        }


@dataclass
class InterfaceDC:
    fs_dict: dict[str, int]
    descriptors: list["DescriptorDC"]
    endpoints: list["EndpointDC"]


@dataclass
class EndpointDC:
    fs_dict: dict[str, int]


@dataclass
class DescriptorDC:
    klass: functionfs.common.USBDescriptorHeader  # subclass of USBDescriptorHeader
    fs_dict: dict[str, int]

    def getDescriptor(self) -> GadgetDC:
        return getDescriptorFS(self.klass, **self.fs_dict)
