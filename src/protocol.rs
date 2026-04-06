//! USB HID packet types for the Xbox 360 wireless receiver protocol.
//!
//! ## Input (host ← device)
//!
//! [`InputReport`] is the 20-byte HID report sent from the emulated receiver
//! to the host describing controller state. Build one from an [`InputState`]
//! with [`InputReport::from_input`] or pass a raw packet through with
//! [`InputReport::from_bytes`].
//!
//! ## Output (host → device)
//!
//! The host sends 8-byte output reports back to each interface for rumble and
//! LED control. These are parsed into [`RumbleReport`] and [`LedReport`], or
//! dispatched automatically via [`OutputReport::from_bytes`].

use crate::controller::InputState;

/// Length of an Xbox 360 HID input report in bytes.
pub const INPUT_REPORT_LEN: usize = 20;

/// A 20-byte Xbox 360 HID input report.
///
/// Byte layout:
///
/// | Byte(s) | Contents |
/// |---------|----------|
/// | 0 | `0x00` (report ID) |
/// | 1 | `0x14` (length = 20) |
/// | 2 | D-pad + Start + Back + L3 + R3 flags |
/// | 3 | LB + RB + Guide + A + B + X + Y flags |
/// | 4 | Left trigger (0–255) |
/// | 5 | Right trigger (0–255) |
/// | 6–7 | Left stick X (little-endian i16) |
/// | 8–9 | Left stick Y (little-endian i16) |
/// | 10–11 | Right stick X (little-endian i16) |
/// | 12–13 | Right stick Y (little-endian i16) |
/// | 14–19 | Reserved / zero |
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct InputReport(pub [u8; INPUT_REPORT_LEN]);

impl InputReport {
    /// All-neutral report (no buttons, sticks centred, triggers at zero).
    pub fn neutral() -> Self {
        Self([0x00, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00])
    }

    /// Serialise an [`InputState`] into a 20-byte HID report.
    pub fn from_input(state: InputState) -> Self {
        let mut report = Self::neutral();

        // byte[2]: dpad | start | back | l3 | r3
        report.0[2] =
            (state.dpad_up    as u8) << 0
            | (state.dpad_down  as u8) << 1
            | (state.dpad_left  as u8) << 2
            | (state.dpad_right as u8) << 3
            | (state.start      as u8) << 4
            | (state.back       as u8) << 5
            | (state.l3         as u8) << 6
            | (state.r3         as u8) << 7;

        // byte[3]: lb | rb | guide | (reserved) | a | b | x | y
        report.0[3] =
            (state.lb    as u8) << 0
            | (state.rb    as u8) << 1
            | (state.guide as u8) << 2
            | (state.a     as u8) << 4
            | (state.b     as u8) << 5
            | (state.x     as u8) << 6
            | (state.y     as u8) << 7;

        // triggers
        report.0[4] = state.left_trigger;
        report.0[5] = state.right_trigger;

        // joystick axes (little-endian i16)
        let lx = state.left_stick_x.to_le_bytes();
        let ly = state.left_stick_y.to_le_bytes();
        let rx = state.right_stick_x.to_le_bytes();
        let ry = state.right_stick_y.to_le_bytes();
        report.0[6..8].copy_from_slice(&lx);
        report.0[8..10].copy_from_slice(&ly);
        report.0[10..12].copy_from_slice(&rx);
        report.0[12..14].copy_from_slice(&ry);

        report
    }

    /// Build an [`InputReport`] directly from a raw byte slice.
    ///
    /// Returns `None` if `data` is shorter than [`INPUT_REPORT_LEN`].
    /// Used by the C FFI layer where the caller builds the full packet.
    pub fn from_bytes(data: &[u8]) -> Option<Self> {
        if data.len() < INPUT_REPORT_LEN {
            return None;
        }
        let mut arr = [0u8; INPUT_REPORT_LEN];
        arr.copy_from_slice(&data[..INPUT_REPORT_LEN]);
        Some(Self(arr))
    }

    /// Return a reference to the raw bytes.
    pub fn as_bytes(&self) -> &[u8; INPUT_REPORT_LEN] {
        &self.0
    }
}

/// Length of an Xbox 360 rumble output report in bytes.
pub const RUMBLE_REPORT_LEN: usize = 8;
/// Length of an Xbox 360 LED output report in bytes.
pub const LED_REPORT_LEN: usize = 3;

/// Rumble (force-feedback) command sent from the host to the receiver.
///
/// Motor values are 0 (off) – 255 (full strength).
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct RumbleReport {
    /// Left (low-frequency) motor strength, 0–255.
    pub left_motor: u8,
    /// Right (high-frequency) motor strength, 0–255.
    pub right_motor: u8,
}

impl RumbleReport {
    /// Parse a rumble report from a raw 8-byte output packet.
    ///
    /// Returns `None` if the slice is too short or the type bytes don't match.
    pub fn from_bytes(data: &[u8]) -> Option<Self> {
        if data.len() < RUMBLE_REPORT_LEN { return None; }
        if data[0] != 0x00 || data[1] != 0x08 { return None; }
        Some(Self { left_motor: data[3], right_motor: data[4] })
    }
}

/// LED ring animation command sent from the host to the receiver.
///
/// The animation ID values match the Xbox 360 wireless protocol exactly.
#[derive(Debug, Clone, PartialEq, Eq)]
#[repr(u8)]
pub enum LedAnimation {
    /// Turn all four LEDs off.
    AllOff      = 0x00,
    /// Blink all four LEDs.
    AllBlink    = 0x01,
    /// Flash player-1 LED, then leave it on.
    P1FlashOn   = 0x02,
    /// Flash player-2 LED, then leave it on.
    P2FlashOn   = 0x03,
    /// Flash player-3 LED, then leave it on.
    P3FlashOn   = 0x04,
    /// Flash player-4 LED, then leave it on.
    P4FlashOn   = 0x05,
    /// Player-1 LED steady on.
    P1On        = 0x06,
    /// Player-2 LED steady on.
    P2On        = 0x07,
    /// Player-3 LED steady on.
    P3On        = 0x08,
    /// Player-4 LED steady on.
    P4On        = 0x09,
    /// Rotate LEDs in sequence.
    Rotate      = 0x0A,
    /// Blink previously-set LED.
    BlinkPrev   = 0x0B,
    /// Slow-blink previously-set LED.
    SlowBlink   = 0x0C,
    /// Alternate between two previously-set LEDs.
    AltPairPrev = 0x0D,
}

impl LedAnimation {
    /// Parse an animation ID byte. Returns `None` for unknown values.
    pub fn from_byte(b: u8) -> Option<Self> {
        match b {
            0x00 => Some(Self::AllOff),
            0x01 => Some(Self::AllBlink),
            0x02 => Some(Self::P1FlashOn),
            0x03 => Some(Self::P2FlashOn),
            0x04 => Some(Self::P3FlashOn),
            0x05 => Some(Self::P4FlashOn),
            0x06 => Some(Self::P1On),
            0x07 => Some(Self::P2On),
            0x08 => Some(Self::P3On),
            0x09 => Some(Self::P4On),
            0x0A => Some(Self::Rotate),
            0x0B => Some(Self::BlinkPrev),
            0x0C => Some(Self::SlowBlink),
            0x0D => Some(Self::AltPairPrev),
            _    => None,
        }
    }
}

/// LED ring command sent from the host to the receiver.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct LedReport {
    /// The animation the host requested.
    pub animation: LedAnimation,
}

impl LedReport {
    /// Parse an LED report from a raw 3-byte output packet.
    ///
    /// Returns `None` if the slice is too short, the type bytes don't match,
    /// or the animation ID is unknown.
    pub fn from_bytes(data: &[u8]) -> Option<Self> {
        if data.len() < LED_REPORT_LEN { return None; }
        if data[0] != 0x01 || data[1] != 0x03 { return None; }
        LedAnimation::from_byte(data[2]).map(|animation| Self { animation })
    }
}

/// Either a [`RumbleReport`] or a [`LedReport`] received from the host.
///
/// Returned by [`WirelessReceiver::poll_output`](crate::WirelessReceiver::poll_output).
#[derive(Debug, Clone, PartialEq, Eq)]
pub enum OutputReport {
    Rumble(RumbleReport),
    Led(LedReport),
}

impl OutputReport {
    /// Dispatch a raw output packet to the correct report type.
    ///
    /// Returns `None` if the packet is empty, too short, or has an unknown
    /// type byte.
    pub fn from_bytes(data: &[u8]) -> Option<Self> {
        match data.first()? {
            0x00 => RumbleReport::from_bytes(data).map(Self::Rumble),
            0x01 => LedReport::from_bytes(data).map(Self::Led),
            _    => None,
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::controller::{Button, InputState};

    #[test]
    fn neutral_report_matches_current_c_example() {
        assert_eq!(InputReport::neutral().0[0..4], [0x00, 0x14, 0x00, 0x00]);
    }

    #[test]
    fn a_button_maps_to_known_bit() {
        let report = InputReport::from_input(InputState::default().with_button(Button::A, true));
        assert_eq!(report.0[3], 0x10);
    }

    #[test]
    fn multiple_buttons_combine() {
        let report = InputReport::from_input(
            InputState::default()
                .with_button(Button::A, true)
                .with_button(Button::B, true),
        );
        assert_eq!(report.0[3], 0x30);
    }

    #[test]
    fn rumble_parses_valid_packet() {
        let data = [0x00, 0x08, 0x00, 0xAA, 0x55, 0x00, 0x00, 0x00];
        let r = RumbleReport::from_bytes(&data).unwrap();
        assert_eq!(r.left_motor, 0xAA);
        assert_eq!(r.right_motor, 0x55);
    }

    #[test]
    fn rumble_rejects_wrong_type_byte() {
        let data = [0x01, 0x08, 0x00, 0xAA, 0x55, 0x00, 0x00, 0x00];
        assert!(RumbleReport::from_bytes(&data).is_none());
    }

    #[test]
    fn rumble_rejects_short_slice() {
        assert!(RumbleReport::from_bytes(&[0x00, 0x08, 0x00]).is_none());
    }

    #[test]
    fn led_parses_all_animation_ids() {
        for id in 0x00u8..=0x0D {
            let data = [0x01, 0x03, id];
            assert!(LedReport::from_bytes(&data).is_some(), "failed on id 0x{:02X}", id);
        }
    }

    #[test]
    fn led_rejects_unknown_animation_id() {
        let data = [0x01, 0x03, 0x0E];
        assert!(LedReport::from_bytes(&data).is_none());
    }

    #[test]
    fn output_report_dispatches_rumble() {
        let data = [0x00, 0x08, 0x00, 0x10, 0x20, 0x00, 0x00, 0x00];
        assert!(matches!(OutputReport::from_bytes(&data), Some(OutputReport::Rumble(_))));
    }

    #[test]
    fn output_report_dispatches_led() {
        let data = [0x01, 0x03, 0x02];
        assert!(matches!(OutputReport::from_bytes(&data), Some(OutputReport::Led(_))));
    }

    #[test]
    fn output_report_unknown_type_is_none() {
        let data = [0xFF, 0x03, 0x00];
        assert!(OutputReport::from_bytes(&data).is_none());
    }

    #[test]
    fn output_report_empty_is_none() {
        assert!(OutputReport::from_bytes(&[]).is_none());
    }
}
