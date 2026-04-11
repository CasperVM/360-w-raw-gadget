//! USB device identity and configuration descriptors.
//!
//! [`ConfigDescriptorSet`] is the primary entry point. Pass it to
//! [`WirelessReceiver::new`](crate::WirelessReceiver::new) to set the number
//! of controller slots (1–4) and the USB identity presented to the host.

use std::collections::HashMap;

/// Errors returned when constructing or querying descriptors.
#[derive(Debug, Clone)]
pub enum DescriptorError {
    /// `num_interfaces` was outside the valid range of 1–4.
    InvalidInterfaceCount(u8),
    /// A string descriptor with the given ID was not found.
    StringNotFound(u8),
}

impl std::fmt::Display for DescriptorError {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            Self::InvalidInterfaceCount(n) => write!(f, "invalid interface count: {n} (must be 1–4)"),
            Self::StringNotFound(id) => write!(f, "string descriptor not found: {id}"),
        }
    }
}

impl std::error::Error for DescriptorError {}

/// USB vendor ID for Microsoft Corporation.
pub const USB_VENDOR: u16 = 0x045e;
/// USB product ID for the Xbox 360 Wireless Receiver.
pub const USB_PRODUCT: u16 = 0x028e;
/// USB high-speed indicator (480 Mbps).
pub const USB_SPEED_HIGH: u8 = 2;

/// USB identity strings and VID/PID for the emulated receiver.
///
/// The default values mimic a real Xbox 360 Wireless Receiver for Windows,
/// which is what host drivers expect.
#[derive(Debug, Clone)]
pub struct ReceiverIdentity {
    pub vendor_id: u16,
    pub product_id: u16,
    pub manufacturer: String,
    pub product_name: String,
    pub serial: String,
}

impl Default for ReceiverIdentity {
    fn default() -> Self {
        Self {
            vendor_id: USB_VENDOR,
            product_id: USB_PRODUCT,
            manufacturer: "©Microsoft Corporation".to_string(),
            product_name: "Xbox 360 Wireless Receiver for Windows".to_string(),
            serial: "08FEC93".to_string(),
        }
    }
}

/// Map from string-descriptor index to string value.
pub type StringDescriptorMap = HashMap<u8, String>;

/// Full USB configuration for the emulated wireless receiver.
///
/// Create with [`ConfigDescriptorSet::new`], optionally customise with
/// [`with_serial`](Self::with_serial), then pass to
/// [`WirelessReceiver::new`](crate::WirelessReceiver::new).
///
/// # Example
///
/// ```rust
/// use x360_w_raw_gadget::ConfigDescriptorSet;
///
/// // One controller slot
/// let config = ConfigDescriptorSet::new(1).unwrap();
///
/// // Four controller slots with a custom serial
/// let config = ConfigDescriptorSet::new(4)
///     .unwrap()
///     .with_serial("DEADBEEF".to_string());
/// ```
#[derive(Debug, Clone)]
pub struct ConfigDescriptorSet {
    pub identity: ReceiverIdentity,
    /// Number of controller interfaces / slots (1–4).
    pub num_interfaces: u8,
    pub strings: StringDescriptorMap,
}

impl ConfigDescriptorSet {
    /// Create a new configuration with `num_interfaces` controller slots.
    ///
    /// Returns [`DescriptorError::InvalidInterfaceCount`] if
    /// `num_interfaces` is 0 or greater than 4.
    pub fn new(num_interfaces: u8) -> Result<Self, DescriptorError> {
        if !(1..=4).contains(&num_interfaces) {
            return Err(DescriptorError::InvalidInterfaceCount(num_interfaces));
        }

        let identity = ReceiverIdentity::default();
        let mut strings = StringDescriptorMap::new();
        strings.insert(0x01, identity.manufacturer.clone());
        strings.insert(0x02, identity.product_name.clone());
        strings.insert(0x03, identity.serial.clone());

        Ok(Self {
            identity,
            num_interfaces,
            strings,
        })
    }

    /// Override the USB serial number string and return `self`.
    pub fn with_serial(mut self, serial: String) -> Self {
        self.strings.insert(0x03, serial.clone());
        self.identity.serial = serial;
        self
    }

    /// Look up a string descriptor by index.
    ///
    /// Returns [`DescriptorError::StringNotFound`] if the index has no entry.
    pub fn string(&self, id: u8) -> Result<&str, DescriptorError> {
        self.strings
            .get(&id)
            .map(|s| s.as_str())
            .ok_or(DescriptorError::StringNotFound(id))
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn default_identity_is_microsoft() {
        let id = ReceiverIdentity::default();
        assert_eq!(id.vendor_id, 0x045e);
        assert_eq!(id.product_id, 0x028e);
    }

    #[test]
    fn config_accepts_1_to_4_interfaces() {
        assert!(ConfigDescriptorSet::new(0).is_err());
        assert!(ConfigDescriptorSet::new(1).is_ok());
        assert!(ConfigDescriptorSet::new(2).is_ok());
        assert!(ConfigDescriptorSet::new(4).is_ok());
        assert!(ConfigDescriptorSet::new(5).is_err());
    }

    #[test]
    fn config_has_default_strings() {
        let config = ConfigDescriptorSet::new(1).unwrap();
        assert_eq!(config.string(0x01).unwrap(), "©Microsoft Corporation");
    }

    #[test]
    fn serial_can_be_overridden() {
        let config = ConfigDescriptorSet::new(1)
            .unwrap()
            .with_serial("CUSTOM123".to_string());
        assert_eq!(config.string(0x03).unwrap(), "CUSTOM123");
    }
}
