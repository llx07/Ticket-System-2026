cmake_minimum_required(VERSION 3.16)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(TicketSystem2026 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(test_bpt
    src/test_bpt.cpp
)

target_include_directories(test_bpt PRIVATE
    ${PROJECT_SOURCE_DIR}/includes
)
