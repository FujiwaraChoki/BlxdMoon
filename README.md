# BlxdMoon

> 🐀 **A RAT-System built in C, with persistence and multiple features.**

![Stars](https://img.shields.io/github/stars/FujiwaraChoki/BlxdMoon.svg)
![License](https://img.shields.io/github/license/FujiwaraChoki/BlxdMoon.svg)

[![BlxdMoon](repo/banner.png)](repo/banner.png)

> ⚠️ **Make sure to set correct IP Addresses & Ports in server.c and backdoor.c.**

## Features

- [x] Connection to a custom-set server
- [x] Receive commands from server, execute them and send back results
- [x] Persistence
- [x] Start/Spawn other programs
- [x] Navigate through the file system
- [x] Keylogger functionality
- [x] For Obfuscation, see [Obfuscation](Obfuscation.md)
- [x] Take screenshots
- [x] Download files from victim's computer
- [x] Upload files to victim's computer
- [x] Get Device Information

## Compilation

### CMake

Run the `configure.sh` file, with two supplied arguments:

```bash
./configure.sh {SERVER_IP} {SERVER_PORT}
```

Then, use CMake to compile the project:

```bash
mkdir build
cd build
cmake ..
make
```

It will save the binaries in the `build` directory.

### Manual Compilation

Replace all IP Addresses and Ports in `src/server.c` and `src/backdoor.c` with your own.

Create a build directory:

```bash
mkdir build
```

Server:

```bash
gcc src/server.c -o build/server.exe -lws2_32
```

Backdoor:

```bash
gcc src/backdoor.c -o build/backdoor.exe -lws2_32 -lgdi32
```

## Usage

To run the server, simply execute bin/server.exe:

```bash
build/server.exe
```

And to run the backdoor, execute bin/backdoor.exe:

```bash
build/backdoor.exe
```

## Commands

| Command           | Description                                                                                      |
| ----------------- | ------------------------------------------------------------------------------------------------ |
| `cd {DIR_NAME}`   | Change directory                                                                                 |
| `persist`         | Make the backdoor persistent                                                                     |
| `keylogger:start` | Start the keylogger, writes to random {UUID}.txt in `Temp` Directory                             |
| `screen`          | Take a screenshot of the current screen, writes to random {UUID}.txt in `Temp/screens` Directory |
| `download {FILE}` | Download a file from the victim's computer                                                       |
| `upload {FILE}`   | Upload a file from server to victim's computer                                                   |

## License

[MIT](LICENSE)

```
MIT License

Copyright (c) 2023 FujiwaraChoki

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
