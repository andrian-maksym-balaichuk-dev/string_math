cmake_minimum_required(VERSION 3.20)

project(
    string_math
    VERSION 0.1.0
    DESCRIPTION "Header-only typed string math library with runtime and compile-time evaluation."
    LANGUAGES CXX
)

include(CTest)
include(GNUInstallDirs)

option(STRING_MATH_BUILD_TESTS "Build the string_math test suite." ${PROJECT_IS_TOP_LEVEL})
option(STRING_MATH_BUILD_EXAMPLES "Build the string_math demo executable." ${PROJECT_IS_TOP_LEVEL})

set(STRING_MATH_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")
set(STRING_MATH_FW_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/thirdParty/function_wrapper")

if(NOT TARGET fw::wrapper)
    if(NOT EXISTS "${STRING_MATH_FW_SOURCE_DIR}/CMakeLists.txt")
        find_package(fw CONFIG REQUIRED)
    else()
        set(FW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
        add_subdirectory("${STRING_MATH_FW_SOURCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}/function_wrapper" EXCLUDE_FROM_ALL)
    endif()
endif()

if(TARGET fw_wrapper)
    set_property(TARGET fw_wrapper PROPERTY INTERFACE_COMPILE_FEATURES "")
endif()

add_library(string_math INTERFACE)
add_library(string_math::string_math ALIAS string_math)

target_include_directories(
    string_math
    INTERFACE
        $<BUILD_INTERFACE:${STRING_MATH_INCLUDE_DIR}>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_link_libraries(string_math INTERFACE fw::wrapper)
target_compile_features(string_math INTERFACE cxx_std_23)
set_target_properties(string_math PROPERTIES CXX_EXTENSIONS OFF)

if(STRING_MATH_BUILD_EXAMPLES)
    add_executable(string_math_demo src/main.cpp)
    target_link_libraries(string_math_demo PRIVATE string_math::string_math)
    target_compile_features(string_math_demo PRIVATE cxx_std_23)
    set_target_properties(string_math_demo PROPERTIES CXX_EXTENSIONS OFF)
endif()

if(BUILD_TESTING AND STRING_MATH_BUILD_TESTS)
    function(string_math_add_test target_name standard source_file)
        add_executable(${target_name} ${source_file})
        target_link_libraries(${target_name} PRIVATE string_math::string_math)
        set_target_properties(
            ${target_name}
            PROPERTIES
                CXX_STANDARD ${standard}
                CXX_STANDARD_REQUIRED ON
                CXX_EXTENSIONS OFF
        )
        if(target_name MATCHES "^string_math_constexpr_")
            if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|AppleClang")
                target_compile_options(${target_name} PRIVATE -fconstexpr-steps=10000000)
            elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
                target_compile_options(${target_name} PRIVATE -fconstexpr-ops-limit=100000000)
            endif()
        endif()
        add_test(NAME ${target_name} COMMAND ${target_name})
    endfunction()

    string_math_add_test(string_math_runtime_cpp17 17 tests/runtime_test.cpp)
    string_math_add_test(string_math_runtime_cpp20 20 tests/runtime_test.cpp)
    string_math_add_test(string_math_runtime_cpp23 23 tests/runtime_test.cpp)
    string_math_add_test(string_math_constexpr_cpp20 20 tests/constexpr_test.cpp)
    string_math_add_test(string_math_constexpr_cpp23 23 tests/constexpr_test.cpp)
endif()

install(DIRECTORY ${STRING_MATH_INCLUDE_DIR}/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
