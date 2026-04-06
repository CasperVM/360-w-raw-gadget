#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum Button {
    A, B, X, Y,
    Lb, Rb,
    L3, R3,
    Start, Back, Guide,
    DpadUp, DpadDown, DpadLeft, DpadRight,
}

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
    pub left_trigger: u8,
    pub right_trigger: u8,
    pub left_stick_x: i16,
    pub left_stick_y: i16,
    pub right_stick_x: i16,
    pub right_stick_y: i16,
}

impl InputState {
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

    pub fn with_left_trigger(mut self, v: u8) -> Self {
        self.left_trigger = v;
        self
    }

    pub fn with_right_trigger(mut self, v: u8) -> Self {
        self.right_trigger = v;
        self
    }

    pub fn with_left_stick(mut self, x: i16, y: i16) -> Self {
        self.left_stick_x = x;
        self.left_stick_y = y;
        self
    }

    pub fn with_right_stick(mut self, x: i16, y: i16) -> Self {
        self.right_stick_x = x;
        self.right_stick_y = y;
        self
    }
}

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

    pub fn index(&self) -> u8 {
        self.index
    }

    pub fn state(&self) -> InputState {
        self.state
    }

    pub fn set_state(&mut self, state: InputState) {
        self.state = state;
    }
}
