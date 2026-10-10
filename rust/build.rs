use std::env;
use std::fs;
use std::path::PathBuf;

fn main() {
    let manifest_dir = PathBuf::from(env::var_os("CARGO_MANIFEST_DIR").unwrap());
    let header = manifest_dir.join("../include/norx/syscall.h");
    println!("cargo:rerun-if-changed={}", header.display());

    let source = fs::read_to_string(&header).unwrap_or_else(|error| {
        panic!(
            "reading canonical syscall ABI {}: {error}",
            header.display()
        )
    });
    let definitions = parse_syscall_numbers(&source);

    let mut generated = String::from(
        "#[derive(Clone, Copy, Debug, PartialEq, Eq)]\n#[repr(u64)]\npub enum Number {\n",
    );
    for (name, value) in definitions {
        generated.push_str("    ");
        generated.push_str(&rust_variant_name(name));
        generated.push_str(" = ");
        generated.push_str(&value.to_string());
        generated.push_str(",\n");
    }
    generated.push_str("}\n");

    let output = PathBuf::from(env::var_os("OUT_DIR").unwrap()).join("syscall_number.rs");
    fs::write(&output, generated).unwrap_or_else(|error| {
        panic!(
            "writing generated syscall ABI {}: {error}",
            output.display()
        )
    });
}

fn parse_syscall_numbers(source: &str) -> Vec<(&str, u64)> {
    let start = source
        .find("enum norx_syscall_number {")
        .unwrap_or_else(|| panic!("canonical syscall ABI enum is missing"));
    let body = &source[start + "enum norx_syscall_number {".len()..];
    let end = body
        .find("};")
        .unwrap_or_else(|| panic!("canonical syscall ABI enum is unterminated"));

    let mut definitions = Vec::new();
    for line in body[..end].lines() {
        let line = line.trim();
        if line.is_empty() {
            continue;
        }
        let line = line
            .strip_suffix(',')
            .unwrap_or_else(|| panic!("malformed syscall ABI entry: {line}"));
        let (name, value) = line
            .split_once('=')
            .unwrap_or_else(|| panic!("malformed syscall ABI entry: {line}"));
        let name = name.trim();
        if !name.starts_with("NORX_SYS_") {
            panic!("unexpected syscall ABI entry: {name}");
        }
        let value = value
            .trim()
            .parse::<u64>()
            .unwrap_or_else(|error| panic!("invalid syscall number {value}: {error}"));
        if definitions
            .iter()
            .any(|(old_name, old_value)| *old_name == name || *old_value == value)
        {
            panic!("duplicate syscall ABI name or number: {name} = {value}");
        }
        definitions.push((&name["NORX_SYS_".len()..], value));
    }
    if definitions.is_empty() {
        panic!("canonical syscall ABI enum has no entries");
    }
    definitions
}

fn rust_variant_name(name: &str) -> String {
    match name {
        "GETPID" => "GetPid".to_owned(),
        "GETTID" => "GetTid".to_owned(),
        "SETPGID" => "SetProcessGroup".to_owned(),
        "GETPGID" => "GetProcessGroup".to_owned(),
        "KILLPG" => "KillProcessGroup".to_owned(),
        _ => name
            .split('_')
            .map(|word| {
                let mut chars = word.chars();
                match chars.next() {
                    Some(first) => {
                        first.to_ascii_uppercase().to_string()
                            + &chars.as_str().to_ascii_lowercase()
                    }
                    None => String::new(),
                }
            })
            .collect(),
    }
}
