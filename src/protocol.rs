use crate::controller::InputState;

pub const INPUT_REPORT_LEN: usize = 20;

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct InputReport(pub [u8; INPUT_REPORT_LEN]);

impl InputReport {
    pub fn neutral() -> Self {
        Self([0x00, 0x14, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00])
    }

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

    /// Build an InputReport directly from a raw 20-byte packet (e.g. from Python ctypes caller).
    pub fn from_bytes(data: &[u8]) -> Option<Self> {
        if data.len() < INPUT_REPORT_LEN {
            return None;
        }
        let mut arr = [0u8; INPUT_REPORT_LEN];
        arr.copy_from_slice(&data[..INPUT_REPORT_LEN]);
        Some(Self(arr))
    }

    pub fn as_bytes(&self) -> &[u8; INPUT_REPORT_LEN] {
        &self.0
    }
}

pub const RUMBLE_REPORT_LEN: usize = 8;
pub const LED_REPORT_LEN: usize = 3;

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct RumbleReport {
    pub left_motor: u8,
    pub right_motor: u8,
}

impl RumbleReport {
    pub fn from_bytes(data: &[u8]) -> Option<Self> {
        if data.len() < RUMBLE_REPORT_LEN { return None; }
        if data[0] != 0x00 || data[1] != 0x08 { return None; }
        Some(Self { left_motor: data[3], right_motor: data[4] })
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
#[repr(u8)]
pub enum LedAnimation {
    AllOff      = 0x00,
    AllBlink    = 0x01,
    P1FlashOn   = 0x02,
    P2FlashOn   = 0x03,
    P3FlashOn   = 0x04,
    P4FlashOn   = 0x05,
    P1On        = 0x06,
    P2On        = 0x07,
    P3On        = 0x08,
    P4On        = 0x09,
    Rotate      = 0x0A,
    BlinkPrev   = 0x0B,
    SlowBlink   = 0x0C,
    AltPairPrev = 0x0D,
}

impl LedAnimation {
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

#[derive(Debug, Clone, PartialEq, Eq)]
pub struct LedReport {
    pub animation: LedAnimation,
}

impl LedReport {
    pub fn from_bytes(data: &[u8]) -> Option<Self> {
        if data.len() < LED_REPORT_LEN { return None; }
        if data[0] != 0x01 || data[1] != 0x03 { return None; }
        LedAnimation::from_byte(data[2]).map(|animation| Self { animation })
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub enum OutputReport {
    Rumble(RumbleReport),
    Led(LedReport),
}

impl OutputReport {
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
