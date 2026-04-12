use std::fs;
use std::io;
use std::path::Path;

fn detect_udc_in(path: &Path) -> io::Result<String> {
    let mut names = Vec::new();
    for entry in fs::read_dir(path)? {
        let entry = entry?;
        if entry.file_type()?.is_dir() {
            names.push(entry.file_name().to_string_lossy().into_owned());
        }
    }
    names.sort();
    names
        .into_iter()
        .next()
        .ok_or_else(|| io::Error::new(io::ErrorKind::NotFound, "no UDC found"))
}

pub fn detect_udc() -> io::Result<String> {
    detect_udc_in(Path::new("/sys/class/udc"))
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::time::{SystemTime, UNIX_EPOCH};

    fn temp_path(name: &str) -> std::path::PathBuf {
        let nonce = SystemTime::now()
            .duration_since(UNIX_EPOCH)
            .unwrap()
            .as_nanos();
        std::env::temp_dir().join(format!("x360-udc-{name}-{nonce}"))
    }

    #[test]
    fn detects_first_udc_name() {
        let root = temp_path("pick-first");
        fs::create_dir_all(root.join("20980000.usb")).unwrap();
        fs::create_dir_all(root.join("3f980000.usb")).unwrap();

        let udc = detect_udc_in(&root).unwrap();
        assert_eq!(udc, "20980000.usb");

        fs::remove_dir_all(root).unwrap();
    }

    #[test]
    fn errors_when_no_udc_exists() {
        let root = temp_path("empty");
        fs::create_dir_all(&root).unwrap();

        let err = detect_udc_in(&root).unwrap_err();
        assert_eq!(err.kind(), io::ErrorKind::NotFound);

        fs::remove_dir_all(root).unwrap();
    }
}
