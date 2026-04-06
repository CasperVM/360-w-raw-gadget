//! Per-controller input state.
//!
//! [`InputState`] holds the full surface of an Xbox 360 controller — all
//! buttons, both triggers, and both analog sticks. [`ControllerSlot`] wraps a
//! state with its slot index (0–3) inside the receiver.

/// Every digital surface on an Xbox 360 controller.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum Button {
    A, B, X, Y,
    Lb, Rb,
    L3, R3,
    Start, Back, Guide,
    DpadUp, DpadDown, DpadLeft, DpadRight,
}

/// Full input state for one Xbox 360 controller slot.
///
/// All fields are `pub` for direct construction. The fluent builder methods
/// (`with_button`, `with_left_stick`, etc.) are provided for convenience.
///
/// # Example
///
/// ```rust
/// use x360_w_raw_gadget::{Button, InputState};
///
/// let state = InputState::default()
///     .with_button(Button::A, true)
///     .with_left_stick(-16000, 8000)
///     .with_left_trigger(200);
/// ```
#[derive(Debug, Clone, Copy, PartialEq, Eq, Default)]
pub struct InputState {
    pub a: bool,
    pub b: bool,
    pub x: bool,
    pub y: bool,
    pub lb: bool,
    pub rb: bool,
    pub l3: bool,
    pub r3: bool,
    pub start: bool,
    pub back: bool,
    pub guide: bool,
    pub dpad_up: bool,
    pub dpad_down: bool,
    pub dpad_left: bool,
    pub dpad_right: bool,
    /// Left trigger, 0–255.
    pub left_trigger: u8,
    /// Right trigger, 0–255.
    pub right_trigger: u8,
    /// Left stick X axis, −32767–32767.
    pub left_stick_x: i16,
    /// Left stick Y axis, −32767–32767. Positive = up.
    pub left_stick_y: i16,
    /// Right stick X axis, −32767–32767.
    pub right_stick_x: i16,
    /// Right stick Y axis, −32767–32767. Positive = up.
    pub right_stick_y: i16,
}

impl InputState {
    /// Set a digital button and return `self` (builder pattern).
    pub fn with_button(mut self, button: Button, pressed: bool) -> Self {
        match button {
            Button::A         => self.a = pressed,
            Button::B         => self.b = pressed,
            Button::X         => self.x = pressed,
            Button::Y         => self.y = pressed,
            Button::Lb        => self.lb = pressed,
            Button::Rb        => self.rb = pressed,
            Button::L3        => self.l3 = pressed,
            Button::R3        => self.r3 = pressed,
            Button::Start     => self.start = pressed,
            Button::Back      => self.back = pressed,
            Button::Guide     => self.guide = pressed,
            Button::DpadUp    => self.dpad_up = pressed,
            Button::DpadDown  => self.dpad_down = pressed,
            Button::DpadLeft  => self.dpad_left = pressed,
            Button::DpadRight => self.dpad_right = pressed,
        }
        self
    }

    /// Set the left trigger (0–255) and return `self`.
    pub fn with_left_trigger(mut self, v: u8) -> Self {
        self.left_trigger = v;
        self
    }

    /// Set the right trigger (0–255) and return `self`.
    pub fn with_right_trigger(mut self, v: u8) -> Self {
        self.right_trigger = v;
        self
    }

    /// Set the left stick axes (−32767–32767 each) and return `self`.
    pub fn with_left_stick(mut self, x: i16, y: i16) -> Self {
        self.left_stick_x = x;
        self.left_stick_y = y;
        self
    }

    /// Set the right stick axes (−32767–32767 each) and return `self`.
    pub fn with_right_stick(mut self, x: i16, y: i16) -> Self {
        self.right_stick_x = x;
        self.right_stick_y = y;
        self
    }
}

/// One controller slot inside a [`WirelessReceiver`](crate::WirelessReceiver).
///
/// Each slot has an index (0–3) and holds the current [`InputState`] that will
/// be serialised into HID packets when [`WirelessReceiver::send_input`](crate::WirelessReceiver::send_input) is called.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct ControllerSlot {
    index: u8,
    state: InputState,
}

impl ControllerSlot {
    pub fn new(index: u8) -> Self {
        Self {
            index,
            state: InputState::default(),
        }
    }

    /// Slot index within the receiver (0–3).
    pub fn index(&self) -> u8 {
        self.index
    }

    /// Current input state.
    pub fn state(&self) -> InputState {
        self.state
    }

    /// Replace the current input state.
    pub fn set_state(&mut self, state: InputState) {
        self.state = state;
    }
}
