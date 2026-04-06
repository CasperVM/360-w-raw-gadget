use std::collections::VecDeque;
use std::error::Error;
use crate::controller::ControllerSlot;
use crate::descriptors::ConfigDescriptorSet;
use crate::protocol::{InputReport, LedReport, OutputReport, RumbleReport};

#[derive(Debug, Clone)]
pub enum HostCommand {
    GetDescriptor { descriptor_type: u8 },
    SetConfiguration { config: u8 },
    RequestInput,
}

pub trait Transport {
    fn write_descriptor(&mut self, data: &[u8]) -> Result<(), Box<dyn Error>>;
    fn write_input_report(&mut self, interface: u8, report: &InputReport) -> Result<(), Box<dyn Error>>;
    fn read_control(&mut self) -> Result<Option<HostCommand>, Box<dyn Error>>;

    /// Poll for a pending rumble command from the host on this interface's OUT endpoint.
    /// Returns Ok(None) if nothing is available (non-blocking).
    fn poll_rumble(&mut self, interface: u8) -> Result<Option<RumbleReport>, Box<dyn Error>>;

    /// Poll for a pending LED command from the host on this interface's OUT endpoint.
    /// Returns Ok(None) if nothing is available (non-blocking).
    fn poll_led(&mut self, interface: u8) -> Result<Option<LedReport>, Box<dyn Error>>;

    /// Convenience: returns whichever of rumble or LED is pending (rumble first).
    /// Call in a loop until None to drain both types in one tick.
    fn poll_output(&mut self, interface: u8) -> Result<Option<OutputReport>, Box<dyn Error>> {
        if let Some(r) = self.poll_rumble(interface)? {
            return Ok(Some(OutputReport::Rumble(r)));
        }
        if let Some(l) = self.poll_led(interface)? {
            return Ok(Some(OutputReport::Led(l)));
        }
        Ok(None)
    }
}

pub struct MockTransport {
    last_report: Option<InputReport>,
    descriptors_sent: Vec<Vec<u8>>,
    pending_rumble: [Option<RumbleReport>; 4],
    pending_led: [Option<LedReport>; 4],
    // Legacy queue for queue_output() compatibility
    _legacy_queue: VecDeque<(u8, OutputReport)>,
}

impl MockTransport {
    pub fn new() -> Self {
        Self {
            last_report: None,
            descriptors_sent: Vec::new(),
            pending_rumble: [None, None, None, None],
            pending_led: [None, None, None, None],
            _legacy_queue: VecDeque::new(),
        }
    }

    pub fn last_report(&self) -> Option<&InputReport> {
        self.last_report.as_ref()
    }

    pub fn descriptors_sent(&self) -> &[Vec<u8>] {
        &self.descriptors_sent
    }

    pub fn queue_output(&mut self, interface: u8, report: OutputReport) {
        let i = interface as usize;
        match report {
            OutputReport::Rumble(r) => self.pending_rumble[i] = Some(r),
            OutputReport::Led(l)    => self.pending_led[i]    = Some(l),
        }
    }
}

impl Default for MockTransport {
    fn default() -> Self {
        Self::new()
    }
}

impl Transport for MockTransport {
    fn write_descriptor(&mut self, data: &[u8]) -> Result<(), Box<dyn Error>> {
        self.descriptors_sent.push(data.to_vec());
        Ok(())
    }

    fn write_input_report(&mut self, _interface: u8, report: &InputReport) -> Result<(), Box<dyn Error>> {
        self.last_report = Some(report.clone());
        Ok(())
    }

    fn read_control(&mut self) -> Result<Option<HostCommand>, Box<dyn Error>> {
        Ok(None)
    }

    fn poll_rumble(&mut self, interface: u8) -> Result<Option<RumbleReport>, Box<dyn Error>> {
        Ok(self.pending_rumble[interface as usize].take())
    }

    fn poll_led(&mut self, interface: u8) -> Result<Option<LedReport>, Box<dyn Error>> {
        Ok(self.pending_led[interface as usize].take())
    }
}

pub struct WirelessReceiver {
    config: ConfigDescriptorSet,
    slots: Vec<ControllerSlot>,
    transport: Box<dyn Transport>,
}

impl WirelessReceiver {
    pub fn new(
        config: ConfigDescriptorSet,
        transport: Box<dyn Transport>,
    ) -> Self {
        let num_slots = config.num_interfaces as usize;
        let slots = (0..num_slots)
            .map(|i| ControllerSlot::new(i as u8))
            .collect();

        Self {
            config,
            slots,
            transport,
        }
    }

    pub fn num_slots(&self) -> u8 {
        self.config.num_interfaces
    }

    pub fn slot(&self, index: u8) -> Option<&ControllerSlot> {
        self.slots.get(index as usize)
    }

    pub fn slot_mut(&mut self, index: u8) -> Option<&mut ControllerSlot> {
        self.slots.get_mut(index as usize)
    }

    pub fn send_input(&mut self, slot_index: u8) -> Result<(), Box<dyn Error>> {
        if let Some(slot) = self.slot(slot_index) {
            let report = InputReport::from_input(slot.state());
            self.transport.write_input_report(slot_index, &report)?;
        }
        Ok(())
    }

    /// Send a raw 20-byte input packet for this slot, bypassing InputState.
    /// Used by the C FFI layer where Python builds the full packet directly.
    pub fn send_raw_input(&mut self, slot_index: u8, data: &[u8]) -> Result<(), Box<dyn Error>> {
        if let Some(report) = InputReport::from_bytes(data) {
            self.transport.write_input_report(slot_index, &report)?;
        }
        Ok(())
    }

    pub fn config(&self) -> &ConfigDescriptorSet {
        &self.config
    }

    /// Poll for a rumble command the host sent to this slot. Returns Ok(None) if nothing pending.
    pub fn poll_rumble(&mut self, slot_index: u8) -> Result<Option<RumbleReport>, Box<dyn Error>> {
        self.transport.poll_rumble(slot_index)
    }

    /// Poll for an LED command the host sent to this slot. Returns Ok(None) if nothing pending.
    pub fn poll_led(&mut self, slot_index: u8) -> Result<Option<LedReport>, Box<dyn Error>> {
        self.transport.poll_led(slot_index)
    }

    /// Poll for either a rumble or LED command (rumble first).
    /// Call in a loop until None to drain both types in one tick.
    pub fn poll_output(&mut self, slot_index: u8) -> Result<Option<OutputReport>, Box<dyn Error>> {
        self.transport.poll_output(slot_index)
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::controller::{Button, InputState};

    #[test]
    fn wireless_receiver_creates_slots() {
        let config = ConfigDescriptorSet::new(2).unwrap();
        let transport = Box::new(MockTransport::new());
        let receiver = WirelessReceiver::new(config, transport);

        assert_eq!(receiver.num_slots(), 2);
        assert!(receiver.slot(0).is_some());
        assert!(receiver.slot(1).is_some());
        assert!(receiver.slot(2).is_none());
    }

    #[test]
    fn can_update_slot_state() {
        let config = ConfigDescriptorSet::new(1).unwrap();
        let transport = Box::new(MockTransport::new());
        let mut receiver = WirelessReceiver::new(config, transport);

        let slot = receiver.slot_mut(0).unwrap();
        slot.set_state(InputState::default().with_button(Button::A, true));

        assert_eq!(receiver.slot(0).unwrap().state().a, true);
    }

    #[test]
    fn send_input_sends_report() {
        let config = ConfigDescriptorSet::new(1).unwrap();
        let transport = Box::new(MockTransport::new());
        let mut receiver = WirelessReceiver::new(config, transport);

        receiver.slot_mut(0).unwrap().set_state(InputState::default().with_button(Button::B, true));
        receiver.send_input(0).unwrap();
    }

    #[test]
    fn poll_rumble_returns_queued_rumble() {
        let config = ConfigDescriptorSet::new(2).unwrap();
        let mut transport = MockTransport::new();
        transport.queue_output(0, OutputReport::Rumble(RumbleReport { left_motor: 0xFF, right_motor: 0x80 }));
        let mut receiver = WirelessReceiver::new(config, Box::new(transport));

        let r = receiver.poll_rumble(0).unwrap();
        assert!(matches!(r, Some(RumbleReport { left_motor: 0xFF, right_motor: 0x80 })));
        assert!(receiver.poll_rumble(0).unwrap().is_none());
    }

    #[test]
    fn poll_led_returns_queued_led() {
        use crate::protocol::{LedAnimation, LedReport};
        let config = ConfigDescriptorSet::new(2).unwrap();
        let mut transport = MockTransport::new();
        transport.queue_output(0, OutputReport::Led(LedReport { animation: LedAnimation::P1On }));
        let mut receiver = WirelessReceiver::new(config, Box::new(transport));

        let l = receiver.poll_led(0).unwrap();
        assert!(matches!(l, Some(LedReport { animation: LedAnimation::P1On })));
        assert!(receiver.poll_led(0).unwrap().is_none());
    }

    #[test]
    fn rumble_and_led_do_not_clobber_each_other() {
        use crate::protocol::{LedAnimation, LedReport};
        let config = ConfigDescriptorSet::new(1).unwrap();
        let mut transport = MockTransport::new();
        transport.queue_output(0, OutputReport::Rumble(RumbleReport { left_motor: 100, right_motor: 200 }));
        transport.queue_output(0, OutputReport::Led(LedReport { animation: LedAnimation::Rotate }));
        let mut receiver = WirelessReceiver::new(config, Box::new(transport));

        // Both should be independently retrievable
        let r = receiver.poll_rumble(0).unwrap();
        let l = receiver.poll_led(0).unwrap();
        assert!(matches!(r, Some(RumbleReport { left_motor: 100, .. })));
        assert!(matches!(l, Some(LedReport { animation: LedAnimation::Rotate })));
    }

    #[test]
    fn poll_output_returns_queued_rumble() {
        let config = ConfigDescriptorSet::new(2).unwrap();
        let mut transport = MockTransport::new();
        transport.queue_output(0, OutputReport::Rumble(RumbleReport { left_motor: 0xFF, right_motor: 0x80 }));
        let mut receiver = WirelessReceiver::new(config, Box::new(transport));

        let report = receiver.poll_output(0).unwrap();
        assert!(matches!(report, Some(OutputReport::Rumble(r)) if r.left_motor == 0xFF && r.right_motor == 0x80));
        assert!(receiver.poll_output(0).unwrap().is_none());
    }

    #[test]
    fn poll_output_filters_by_interface() {
        use crate::protocol::{LedAnimation, LedReport, OutputReport};
        let config = ConfigDescriptorSet::new(2).unwrap();
        let mut transport = MockTransport::new();
        transport.queue_output(1, OutputReport::Led(LedReport { animation: LedAnimation::P2FlashOn }));
        let mut receiver = WirelessReceiver::new(config, Box::new(transport));

        assert!(receiver.poll_output(0).unwrap().is_none());
        assert!(receiver.poll_output(1).unwrap().is_some());
    }
}
