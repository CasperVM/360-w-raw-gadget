#!/bin/bash

# Define gadget path
GADGET_PATH="/sys/kernel/config/usb_gadget/myxboxcontroller"

# Function to remove existing gadget
remove_gadget() {
    echo "" > ${GADGET_PATH}/UDC
    rm -r ${GADGET_PATH}/configs/c.1
    rm -r ${GADGET_PATH}/functions/hid.gp0
    rmdir ${GADGET_PATH}
}

# Function to create a new gadget
create_gadget() {
    mkdir -p ${GADGET_PATH}
    cd ${GADGET_PATH}
    echo 0x045e > idVendor
    echo 0x02ea > idProduct
    echo 0x0200 > bcdUSB
    echo 0x0514 > bcdDevice
    mkdir -p strings/0x409
    echo "1" > strings/0x409/serialnumber
    echo "Microsoft" > strings/0x409/manufacturer
    echo "Xbox One Controller" > strings/0x409/product
    mkdir -p configs/c.1
    mkdir -p configs/c.1/strings/0x409
    echo "Standard Configuration" > configs/c.1/strings/0x409/configuration
    mkdir functions/hid.gp0
    echo 0 > functions/hid.gp0/protocol
    echo 0 > functions/hid.gp0/subclass
    echo 64 > functions/hid.gp0/report_length
    echo -ne \\x05\\x01\\x09\\x04\\xa1\\x01\\x15\\x00\\x26\\xff\\x00\\x75\\x08\\x95\\x02\\x09\\x30\\x09\\x31\\x81\\x02\\x15\\x00\\x25\\x01\\x75\\x01\\x95\\x08\\x05\\x09\\x19\\x01\\x29\\x08\\x81\\x02\\xc0 > functions/hid.gp0/report_desc
    ln -s functions/hid.gp0 configs/c.1/
    echo $(ls /sys/class/udc) > UDC
}

# Check if gadget exists and remove it
if [ -d "${GADGET_PATH}" ]; then
    remove_gadget
fi

# Create new gadget configuration
create_gadget

