use bytes::BytesMut;
use std::{
    io::ErrorKind,
    sync::{
        atomic::{AtomicBool, Ordering},
        Arc,
    },
    thread,
    time::Duration,
};

use usb_gadget::{
    default_udc,
    function::custom::{Custom, Endpoint, EndpointDirection, Event, Interface, OsExtProp, TransferType},
    Class, Config, Gadget, Id, OsDescriptor, Strings,
};

fn main() {
    // Setup logging
    env_logger::init();

    // Get the default UDC
    let udc = default_udc().expect("no UDC found");
    // Remove all gadgets
    usb_gadget::remove_all().expect("cannot remove all gadgets");


    // IF0: Control data

    // Custom descriptor for IF0
    // 0x11 0x21 0x00 0x01 0x01 0x25 0x81 0x14 0x00 0x00 0x00 0x00 0x13 0x01 0x08 0x00 0x00
    // Some kind of HID descriptor?
    let if0_ud: Vec<u8> = vec![0x11, 0x21, 0x00, 0x01, 0x01, 0x25, 0x81, 0x14, 0x00, 0x00, 0x00, 0x00, 0x13, 0x01, 0x08, 0x00, 0x00];
    
    
    // IN: Control data send
    let (mut if0_ep1_tx, if0_ep1_tx_direction) = EndpointDirection::device_to_host();
    // OUT: Control data receive (led + rumble)
    let (mut if0_ep1_rx, if0_ep1_rx_direction) = EndpointDirection::host_to_device();

    let if0 =   Interface::new(Class::vendor_specific(0x5D, 0x01), "©Microsoft Corporation")
    .with_endpoint(Endpoint::custom(if0_ep1_tx_direction, TransferType::Interrupt))
    .with_endpoint(Endpoint::custom(if0_ep1_rx_direction, TransferType::Interrupt))

    // IF1: Headset (and expansion port?)

    // Customer descriptor for IF1
    // 0x1b 0x21 0x00 0x01 0x01 0x01 0x82 0x40 0x01 0x02 0x20 0x16 0x83 0x00 0x00 0x00 0x00 0x00 0x00 0x16 0x03 0x00 0x00 0x00 0x00 0x00 0x00
    // another HID descriptor?
    let if1_ud: Vec<u8> = vec![0x1b, 0x21, 0x00, 0x01, 0x01, 0x01, 0x82, 0x40, 0x01, 0x02, 0x20, 0x16, 0x83, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x16, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00];

    // IN: Microphone data send
    let (mut if1_ep2_tx, if1_ep2_tx_direction) = EndpointDirection::device_to_host();
    // OUT: Headset data receive
    let (mut if1_ep2_rx, if1_ep2_rx_direction) = EndpointDirection::host_to_device();
    // IN: Unknown send (chatpad?)
    let (mut if1_ep3_tx, if1_ep3_tx_direction) = EndpointDirection::device_to_host();
    // OUT: Unknown receive ???
    let (mut if1_ep3_rx, if1_ep3_rx_direction) = EndpointDirection::host_to_device();

    let if1 =   Interface::new(Class::vendor_specific(0x5D, 0x03), "Controller")
    .with_endpoint(Endpoint::custom(if1_ep2_tx_direction, TransferType::Interrupt))
    .with_endpoint(Endpoint::custom(if1_ep2_rx_direction, TransferType::Interrupt))
    .with_endpoint(Endpoint::custom(if1_ep3_tx_direction, TransferType::Interrupt))
    .with_endpoint(Endpoint::custom(if1_ep3_rx_direction, TransferType::Interrupt))

    // IF2: Unknown

    // Custom descriptor for IF2
    // 0x09 0x21 0x00 0x01 0x01 0x22 0x84 0x07 0x00
    // Unkown
    let if2_ud: Vec<u8> = vec![0x09, 0x21, 0x00, 0x01, 0x01, 0x22, 0x84, 0x07, 0x00];

    // IN: Unknown send
    let (mut if2_ep4_tx, if2_ep4_tx_direction) = EndpointDirection::device_to_host();

    let if2 =   Interface::new(Class::vendor_specific(0x5D, 0x04), "08FEC93")
    .with_endpoint(Endpoint::custom(if2_ep4_tx_direction, TransferType::Interrupt))
    .with_os_ext_prop(OsExtProp::new("custom?", if2_ud));

    // IF3: Security method

    // Custom descriptor for IF3
    // 0x06 0x41 0x00 0x01 0x01 0x03
    // 0x41 instead of 0x21, different class?
    let if3_ud: Vec<u8> = vec![0x06, 0x41, 0x00, 0x01, 0x01, 0x03];
    let if3 =   Interface::new(Class::vendor_specific(0x5D, 0x05), "Xbox Security Method 3, Version 1.00, © 2005 Microsoft Corporation. All rights reserved.")
    .with_os_ext_prop(OsExtProp::new("custom?", if3_ud));

    let (mut custom, handle) = Custom::builder()
    .with_interface(if0)
    .with_interface(if1)
    .with_interface(if2)
    .with_interface(if3)
    .build();
    
    // Create a new gadget and bind it to the UDC
    let reg = Gadget::new(
        Class::new(0xff, 0xff, 0xff),
        Id::new(
            0x045E, // (Microsoft Corp.)
            0x028E, // (Xbox360 Controller)
        ),
        Strings::new("©Microsoft Corporation", "Controller", "08FEC93"),
    )
    .with_config(Config::new("config")
    .with_function(handle))
    .with_os_descriptor(OsDescriptor::microsoft())
    .bind(&udc)
    .expect("cannot bind to UDC");

    println!("Custom function at {}", custom.status().unwrap().path().unwrap().display());
    println!();

    // let if0_ep1_tx_control = if0_ep1_tx.control().unwrap();
    // println!("if0_ep1_tx real address: {}", if0_ep1_tx_control.real_address().unwrap());
    // println!("if0_ep1_tx descriptor: {:?}", if0_ep1_tx_control.descriptor().unwrap());

    // let if0_ep1_rx_control = if0_ep1_rx.control().unwrap();
    // println!("if0_ep1_rx real address: {}", if0_ep1_rx_control.real_address().unwrap());
    // println!("if0_ep1_rx descriptor: {:?}", if0_ep1_rx_control.descriptor().unwrap());

    // let if1_ep2_tx_control = if1_ep2_tx.control().unwrap();
    // println!("if1_ep2_tx real address: {}", if1_ep2_tx_control.real_address().unwrap());
    // println!("if1_ep2_tx descriptor: {:?}", if1_ep2_tx_control.descriptor().unwrap());

    // let if1_ep2_rx_control = if1_ep2_rx.control().unwrap();
    // println!("if1_ep2_rx real address: {}", if1_ep2_rx_control.real_address().unwrap());
    // println!("if1_ep2_rx descriptor: {:?}", if1_ep2_rx_control.descriptor().unwrap());

    // let if1_ep3_tx_control = if1_ep3_tx.control().unwrap();
    // println!("if1_ep3_tx real address: {}", if1_ep3_tx_control.real_address().unwrap());
    // println!("if1_ep3_tx descriptor: {:?}", if1_ep3_tx_control.descriptor().unwrap());

    // let if1_ep3_rx_control = if1_ep3_rx.control().unwrap();
    // println!("if1_ep3_rx real address: {}", if1_ep3_rx_control.real_address().unwrap());
    // println!("if1_ep3_rx descriptor: {:?}", if1_ep3_rx_control.descriptor().unwrap());

    // let if2_ep4_tx_control = if2_ep4_tx.control().unwrap();
    // println!("if2_ep4_tx real address: {}", if2_ep4_tx_control.real_address().unwrap());
    // println!("if2_ep4_tx descriptor: {:?}", if2_ep4_tx_control.descriptor().unwrap());

    let stop = Arc::new(AtomicBool::new(false));

    thread::scope(|s| {
        s.spawn(|| {
            let size = if0_ep1_rx.max_packet_size().unwrap();
            // let mut b: i32 = 0;
            while !stop.load(Ordering::Relaxed) {
                let data = if0_ep1_rx
                    .recv_timeout(BytesMut::with_capacity(size), Duration::from_secs(1))
                    .expect("recv failed");
                match data {
                    Some(data) => {
                        // println!("received {} bytes: {data:x?}", data.len());
                        // if !data.iter().all(|x| *x == b) {
                        //     panic!("wrong data received");
                        // }
                        // b = b.wrapping_add(1);
                        println!("received {} bytes", data.len());
                        print!("data: ");
                        for i in 0..data.len() {
                            print!("{:02x} ", data[i]);
                        }
                    }
                    None => {
                        // Do nothing
                        // println!("receive empty");
                    }
                }
            }
        });

        s.spawn(|| {
            // let size = if0_ep1_tx.max_packet_size().unwrap();
            let a_button_packet = vec![
                0x00, 0x14, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            ];
            let empty_packet = vec![0; 20];

            while !stop.load(Ordering::Relaxed) {
                match if0_ep1_tx.send_timeout(a_button_packet.clone().into(), Duration::from_secs(1)) {
                    Ok(()) => println!("A button packet sent!"),
                    Err(err) if err.kind() == ErrorKind::TimedOut => println!("send timeout"),
                    Err(err) => panic!("send failed: {err}"),
                }
                thread::sleep(Duration::from_secs(1));
                // Send an empty packet
                match if0_ep1_tx.send_timeout(empty_packet.clone().into(), Duration::from_secs(1)) {
                    Ok(()) => println!("Empty packet sent!"),
                    Err(err) if err.kind() == ErrorKind::TimedOut => println!("send timeout"),
                    Err(err) => panic!("send failed: {err}"),
                }
                thread::sleep(Duration::from_secs(1));
            }
        });

        s.spawn(|| {
            // let mut ctrl_data = Vec::new();

            while !stop.load(Ordering::Relaxed) {
                if let Some(event) = custom.event_timeout(Duration::from_secs(1)).expect("event failed") {
                    
                    match event {
                        // Event::SetupHostToDevice(req) => {
                        //     if req.ctrl_req().request == 255 {
                        //         println!("Stopping");
                        //         stop.store(true, Ordering::Relaxed);
                        //     }
                        //     ctrl_data = req.recv_all().unwrap();
                        //     println!("Control data: {ctrl_data:x?}");
                        // }
                        // Event::SetupDeviceToHost(req) => {
                        //     println!("Replying with data");
                        //     req.send(&ctrl_data).unwrap();
                        // }
                        Event::Enable => {
                            println!("Enable event");
                            // Send unkown enable data

                        },
                        _ => println!("Event: {event:?}")

                        ,
                    }
                } 
                // else {
                //     println!("no event");
                // }
            }
        });
    });

    // For now, just sleep for 10 seconds
    thread::sleep(Duration::from_millis(10000));

    println!("Unregistering gadget");
    reg.remove().unwrap();
}