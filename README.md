# BlxdMoon

> 📌 **Make sure to set correct IP Addresses & Ports in server.c and backdoor.c.**

![Stars](https://img.shields.io/github/stars/FujiwaraChoki/BlxdMoon.svg)
![License](https://img.shields.io/github/license/FujiwaraChoki/BlxdMoon.svg)

[![BlxdMoon](repo/banner.png)](repo/banner.png)

## Features

- [x] Connection to a custom-set server
- [x] Receive commands from server, execute them and send back results
- [x] Persistence
- [x] Start/Spawn other programs
- [x] Navigate through the file system
- [x] Keylogger functionality
- [x] For Obfuscation, see [Obfuscation](Obfuscation.md)
- [x] Take screenshots
- [x] Download files from victim's computer (in progress)

## Compilation

Server:

```bash
gcc src/server.c -o bin/server.exe -lws2_32
```

Backdoor:

```bash
gcc src/backdoor.c -o bin/backdoor.exe -lws2_32 -lgdi32
```

## Usage

To run the server, simply execute bin/server.exe:

```bash
bin/server.exe
```

And to run the backdoor, execute bin/backdoor.exe:

```bash
bin/backdoor.exe
```

## Commands

| Command           | Description                                                                                      |
| ----------------- | ------------------------------------------------------------------------------------------------ |
| `cd`              | Change directory                                                                                 |
| `persist`         | Make the backdoor persistent                                                                     |
| `keylogger:start` | Start the keylogger, writes to random {UUID}.txt in `Temp` Directory                             |
| `screen`          | Take a screenshot of the current screen, writes to random {UUID}.txt in `Temp/screens` Directory |

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
