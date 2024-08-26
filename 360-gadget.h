#include <stddef.h>
#include <unistd.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include <linux/types.h>
#include <linux/usb/ch9.h>
#include <linux/usb/raw_gadget.h>

#include <pthread.h>
#include "usb_helpers.h"

// Driver and device names depend on the hardware
// Based on the raw-gadget list: https://github.com/xairy/raw-gadget?tab=readme-ov-file#usb-device-controllers

#ifndef USB_GADGET_CONFIG_H
#define USB_GADGET_CONFIG_H

// Raspberry Pi Zero / Zero 2 (default)
#define USB_RAW_GADGET_DRIVER_DEFAULT "20980000.usb"
#define USB_RAW_GADGET_DEVICE_DEFAULT "20980000.usb" // dwc2

// Raspberry Pi 4
#if defined(HW_RPI4)
#define USB_RAW_GADGET_DRIVER_DEFAULT "fe980000.usb"
#define USB_RAW_GADGET_DEVICE_DEFAULT "fe980000.usb" // dwc2

// Pi 5
#elif defined(HW_RPI5)
#define USB_RAW_GADGET_DRIVER_DEFAULT "1000480000.usb"
#define USB_RAW_GADGET_DEVICE_DEFAULT "1000480000.usb" // dwc2

// NanoPi NEO2 (ref: https://github.com/msawahara/me56ps2-emulator/blob/master/me56ps2.h)
#elif defined(HW_NANOPI_NEO2)
#define USB_RAW_GADGET_DRIVER_DEFAULT "musb-hdrc";
#define USB_RAW_GADGET_DEVICE_DEFAULT "musb-hdrc.2.auto";

// USB armory mk2
#elif defined(HW_USB_ARMORY_MK2)
#define USB_RAW_GADGET_DRIVER_DEFAULT "2184000.usb";
#define USB_RAW_GADGET_DEVICE_DEFAULT "ci_hdrc.0";

// Orange pi pc / pc 2
#elif defined(HW_ORANGE_PI_PC)
#define USB_RAW_GADGET_DRIVER_DEFAULT "musb-hdrc";
#define USB_RAW_GADGET_DEVICE_DEFAULT "musb-hdrc.4.auto";

// Khadas VIM1
#elif defined(HW_KHADAS_VIM1)
#define USB_RAW_GADGET_DRIVER_DEFAULT "c9100000.usb"
#define USB_RAW_GADGET_DEVICE_DEFAULT "c9100000.usb"

// ThinkPad X1 Carbon Gen 6
#elif defined(HW_THINKPAD_X1)
#define USB_RAW_GADGET_DRIVER_DEFAULT "dwc3-gadget"
#define USB_RAW_GADGET_DEVICE_DEFAULT "dwc3.1.auto"

// NXP i.MX8MP
#elif defined(HW_NXP_IMX8MP)
#define USB_RAW_GADGET_DRIVER_DEFAULT "dwc3-gadget"
#define USB_RAW_GADGET_DEVICE_DEFAULT "38100000.usb"

// BeagleBone Black
#elif defined(HW_BEAGLEBONE_BLACK)
#define USB_RAW_GADGET_DRIVER_DEFAULT "musb-hdrc"
#define USB_RAW_GADGET_DEVICE_DEFAULT "musb-hdrc.0"

// BeagleBone AI
#elif defined(HW_BEAGLEBONE_AI)
#define USB_RAW_GADGET_DRIVER_DEFAULT "dwc3-gadget"
#define USB_RAW_GADGET_DEVICE_DEFAULT "48380000.usb"

// EC3380-AB
#elif defined(HW_EC3380_AB)
#define USB_RAW_GADGET_DRIVER_DEFAULT "net2280"
#define USB_RAW_GADGET_DEVICE_DEFAULT "0000:04:00.0"

// Odroid C2
#elif defined(HW_ODROID_C2)
#define USB_RAW_GADGET_DRIVER_DEFAULT "dwc_otg_pcd"
#define USB_RAW_GADGET_DEVICE_DEFAULT "dwc2_a"

#elif defined(HW_GENERIC)
#define USB_RAW_GADGET_DRIVER_DEFAULT "dummy_udc"
#define USB_RAW_GADGET_DEVICE_DEFAULT "dummy_udc.0"
#endif

#endif // USB_GADGET_CONFIG_H

// Available functions
int init_360_gadget(bool await_endpoint_availability);
void close_360_gadget(int fd);
bool send_to_ep1(int fd, char *data, int len);
void gadget_example();
