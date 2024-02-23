cmake_minimum_required(VERSION 3.10)

project(BlxdMoon)

set(CMAKE_CXX_STANDARD 17)

include_directories(include)

add_executable(backdoor src/backdoor.c)
add_executable(server src/server.c)

target_link_libraries(backdoor ws2_32 gdi32)
target_link_libraries(server ws2_32)
