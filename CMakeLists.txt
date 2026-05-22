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

include(FetchContent)
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.5.4
)
FetchContent_MakeAvailable(Catch2)

add_executable(b_plus_tree_tests
    tests/b_plus_tree_test.cpp
)

target_include_directories(b_plus_tree_tests PRIVATE
    ${PROJECT_SOURCE_DIR}/includes
)

target_link_libraries(b_plus_tree_tests PRIVATE
    Catch2::Catch2WithMain
)

add_executable(list_tests
    tests/list_test.cpp
)

target_include_directories(list_tests PRIVATE
    ${PROJECT_SOURCE_DIR}/includes
)

target_link_libraries(list_tests PRIVATE
    Catch2::Catch2WithMain
)


add_executable(unordered_map_tests
    tests/unordered_map_test.cpp
)
target_include_directories(unordered_map_tests PRIVATE
    ${PROJECT_SOURCE_DIR}/includes
)

target_link_libraries(unordered_map_tests PRIVATE
    Catch2::Catch2WithMain
)

add_executable(parser_tests
    tests/parser_test.cpp
)
target_include_directories(parser_tests PRIVATE
    ${PROJECT_SOURCE_DIR}/includes
)

target_link_libraries(parser_tests PRIVATE
    Catch2::Catch2WithMain
)


list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
include(CTest)
include(Catch)
catch_discover_tests(b_plus_tree_tests)
catch_discover_tests(list_tests)
catch_discover_tests(unordered_map_tests)
catch_discover_tests(parser_tests)
