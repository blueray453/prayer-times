cmake_minimum_required(VERSION 3.16)
project(prayer_times)

set(CMAKE_AUTOMOC ON)
set(CMAKE_CXX_STANDARD 17)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network)

qt6_add_executable(prayer_times main.cpp)

target_link_libraries(prayer_times PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
)

# Copy config.json to build directory only if it doesn't exist
add_custom_command(TARGET prayer_times POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_SOURCE_DIR}/config.json
        ${CMAKE_BINARY_DIR}/config.json
    COMMENT "Copying config.json to build directory (if missing)"
)