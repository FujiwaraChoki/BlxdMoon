# BlxdMoon - Obfuscating C Source Code

To obfuscate the source code of BlxdMoon, use [Scrt/AvCleaner](https://github.com/scrt/avcleaner).

## Usage

```bash
git clone https://github.com/scrt/avcleaner.git
docker build . -t avcleaner
docker run -v {PATH_TO_AV_CLEANER_LOCAL}:/home/toto -it avcleaner bash
sudo pacman -Syu
mkdir CMakeBuild && cd CMakeBuild
cmake ..
make -j 2
./avcleaner.bin --help
```
