#!/bin/bash

# Define gadget path
GADGET_PATH="/sys/kernel/config/usb_gadget/myquadpads"

# Create the gadget directory structure
mkdir -p ${GADGET_PATH}
cd ${GADGET_PATH}

# Set basic attributes
echo 0x1d6b > idVendor  # Linux Foundation
echo 0x028e > idProduct # Generic Xbox Controller
echo 0x0100 > bcdDevice
echo 0x0200 > bcdUSB

# Create string directories
mkdir -p strings/0x409
echo "0001" > strings/0x409/serialnumber
echo "Example Manufacturer" > strings/0x409/manufacturer
echo "Quad Xbox-Style Gamepad Gadget" > strings/0x409/product

# Create configuration
mkdir -p configs/c.1
mkdir -p configs/c.1/strings/0x409
echo "Xbox-Style Gamepad Configuration" > configs/c.1/strings/0x409/configuration

# Controler funcs
for i in 0 1 2 3; do
    mkdir functions/hid.gp$i
    echo 0 > functions/hid.gp$i/protocol
    echo 0 > functions/hid.gp$i/subclass
    echo 64 > functions/hid.gp$i/report_length
    # Ensure the report descriptor is correctly formatted and byte aligned
    echo -ne \\x05\\x01\\x09\\x05\\xa1\\x01\\x05\\x09\\x19\\x01\\x29\\x0B\\x15\\x00\\x25\\x01\\x75\\x01\\x95\\x0B\\x81\\x02\\x05\\x01\\x09\\x30\\x09\\x31\\x09\\x32\\x09\\x35\\x15\\x81\\x25\\x7f\\x75\\x08\\x95\\x04\\x81\\x02\\xc0 > functions/hid.gp$i/report_desc
    ln -s functions/hid.gp$i configs/c.1/
done

# Enable the gadget
# First, deactivate any existing connection if present
if [ -e ${GADGET_PATH}/UDC ]; then
    echo "" > UDC
fi

# List available UDC drivers
UDC_DRIVER=$(ls /sys/class/udc | head -n 1)
if [ -z "$UDC_DRIVER" ]; then
    echo "No UDC driver found. Cannot continue."
    exit 1
fi

echo $UDC_DRIVER > UDC
