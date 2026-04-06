use std::collections::HashMap;

#[derive(Debug, Clone)]
pub enum DescriptorError {
    InvalidInterfaceCount(u8),
    StringNotFound(u8),
}

pub const USB_VENDOR: u16 = 0x045e; // Microsoft
pub const USB_PRODUCT: u16 = 0x028e; // Xbox 360 Controller (use this to support multiple inputs)
pub const USB_SPEED_HIGH: u8 = 2; // High-speed (480 Mbps)

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

pub type StringDescriptorMap = HashMap<u8, String>;

#[derive(Debug, Clone)]
pub struct ConfigDescriptorSet {
    pub identity: ReceiverIdentity,
    pub num_interfaces: u8,
    pub strings: StringDescriptorMap,
}

impl ConfigDescriptorSet {
    pub fn new(num_interfaces: u8) -> Result<Self, DescriptorError> {
        if num_interfaces < 1 || num_interfaces > 4 {
            return Err(DescriptorError::InvalidInterfaceCount(num_interfaces));
        }

        let mut strings = StringDescriptorMap::new();
        strings.insert(0x01, "©Microsoft Corporation".to_string());
        strings.insert(0x02, "Xbox 360 Wireless Receiver for Windows".to_string());
        strings.insert(0x03, "08FEC93".to_string());

        Ok(Self {
            identity: ReceiverIdentity::default(),
            num_interfaces,
            strings,
        })
    }

    pub fn with_serial(mut self, serial: String) -> Self {
        self.strings.insert(0x03, serial.clone());
        self.identity.serial = serial;
        self
    }

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
