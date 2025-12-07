# BlxdMoon - Evasion & Obfuscation Guide

This document covers both **built-in evasion** (part of the codebase) and **external obfuscation tools**.

## Built-in Evasion Module

BlxdMoon includes a comprehensive evasion system in `src/evasion.c`. This is automatically initialized at startup via `InitEvasion()`.

### Features

| Feature | Header | MITRE ATT&CK |
|---------|--------|--------------|
| String Obfuscation | `include/obfuscate.h` | T1027 |
| API Hashing | `include/api_resolve.h` | T1027.007 |
| Direct Syscalls | `include/syscalls.h` | T1106 |
| VM Detection | `include/anti_analysis.h` | T1497 |
| Debugger Detection | `include/anti_analysis.h` | T1622 |
| Sandbox Evasion | `include/anti_analysis.h` | T1497.001 |
| AMSI Bypass | `src/evasion.c` | T1562.001 |
| ETW Patching | `src/evasion.c` | T1562.006 |
| ntdll Unhooking | `src/evasion.c` | T1562.001 |

### Configuration

Edit `include/evasion.h` to enable/disable specific techniques:

```c
#define EVASION_CHECK_VM            1   // Check for virtual machines
#define EVASION_CHECK_DEBUGGER      1   // Check for debuggers
#define EVASION_CHECK_SANDBOX       1   // Check for sandboxes
#define EVASION_PATCH_AMSI          1   // Patch AMSI
#define EVASION_PATCH_ETW           1   // Patch ETW
#define EVASION_UNHOOK_NTDLL        1   // Unhook ntdll.dll
```

### String Encryption Tool

Use the Python tool to generate new encrypted strings:

```bash
cd tools

# Basic usage
python encrypt_strings.py "VirtualAlloc"

# With custom XOR key
python encrypt_strings.py "kernel32.dll" --key 0xAB

# Generate API hash too
python encrypt_strings.py "CreateFileA" --hash

# Verify decryption works
python encrypt_strings.py "test string" --verify
```

Output example:
```c
// Original: "VirtualAlloc"
static const unsigned char ENC_VIRTUALALLOC[] = {
    0x0c, 0x3b, 0x28, 0x2c, 0x2b, 0x3f, 0x36, 0x1b, 0x36, 0x36, 0x35, 0x39
};
#define ENC_VIRTUALALLOC_LEN 12
```

### Adding New Encrypted Strings

1. Generate the encrypted array using `tools/encrypt_strings.py`
2. Add it to `include/obfuscate.h`
3. Use `DECRYPT_STRING()` macro in your code:

```c
#include "obfuscate.h"

void MyFunction() {
    DECRYPT_STRING(apiName, ENC_VIRTUALALLOC, ENC_VIRTUALALLOC_LEN);
    // apiName now contains "VirtualAlloc"

    // ... use apiName ...

    // Clear from memory when done
    SECURE_CLEAR(apiName, ENC_VIRTUALALLOC_LEN);
}
```

---

## External Obfuscation Tools

For additional source-code level obfuscation, use external tools before compilation.

### AvCleaner

[Scrt/AvCleaner](https://github.com/scrt/avcleaner) performs source code transformations to evade static analysis.

#### Installation

```bash
git clone https://github.com/scrt/avcleaner.git
cd avcleaner
docker build . -t avcleaner
docker run -v $(pwd):/home/toto -it avcleaner bash
```

Inside the container:

```bash
sudo pacman -Syu
mkdir CMakeBuild && cd CMakeBuild
cmake ..
make -j 2
./avcleaner.bin --help
```

#### Usage

```bash
# Obfuscate a single file
./avcleaner.bin -f /path/to/backdoor.c

# Options
./avcleaner.bin --help
```

### Other Tools

| Tool | Purpose | Link |
|------|---------|------|
| **Alcatraz** | PE obfuscator | [weak1337/Alcatraz](https://github.com/weak1337/Alcatraz) |
| **pe_obfuscator** | PE section manipulation | Various |
| **UPX** | Executable packer (easily detected) | [upx.github.io](https://upx.github.io/) |

---

## Documentation

For detailed explanations of each evasion technique with code examples and MITRE ATT&CK references, see:

**[docs/defender-evasion.md](docs/defender-evasion.md)**

Topics covered:
- How Windows Defender detection works
- String obfuscation implementation
- API hashing and PEB walking
- Direct syscalls
- AMSI/ETW bypass techniques
- Anti-VM/debugger/sandbox checks
- Real-world malware technique references
