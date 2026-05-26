cmake_minimum_required(VERSION 3.16)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(TicketSystem2026 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(code
    src/main.cpp
)

target_include_directories(code PRIVATE
    ${PROJECT_SOURCE_DIR}/includes
)

if(NOT DEFINED ENV{LOCAL_ENV})
    return()
endif()

add_executable(test_bpt
    src/test_bpt.cpp
)

target_include_directories(test_bpt PRIVATE
    ${PROJECT_SOURCE_DIR}/includes
)

function(add_project_test target source)
    add_executable(${target}
        ${source}
    )
    target_include_directories(${target} PRIVATE
        ${PROJECT_SOURCE_DIR}/includes
    )
    target_link_libraries(${target} PRIVATE
        Catch2::Catch2WithMain
    )
    catch_discover_tests(${target})
endfunction()

include(FetchContent)
FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.5.4
)
FetchContent_MakeAvailable(Catch2)

list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
include(CTest)
include(Catch)

add_project_test(b_plus_tree_tests tests/b_plus_tree_test.cpp)
add_project_test(list_tests tests/list_test.cpp)
add_project_test(unordered_map_tests tests/unordered_map_test.cpp)
add_project_test(parser_tests tests/parser_test.cpp)
add_project_test(fixed_string_tests tests/fixed_string_test.cpp)

add_test(
    NAME basic_1_user
    COMMAND ./run-test basic_1
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)
