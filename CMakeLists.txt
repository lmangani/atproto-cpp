# CMakeLists.txt
cmake_minimum_required(VERSION 3.14)
project(bluesky-client VERSION 0.1.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Options
option(BUILD_TESTS "Build test cases" OFF)

# Find required packages
find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)

# Include FetchContent for downloading dependencies
include(FetchContent)

# Fetch cpp-httplib
FetchContent_Declare(
    httplib
    GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
    GIT_TAG v0.14.1
)
FetchContent_MakeAvailable(httplib)

# Fetch and build yyjson
FetchContent_Declare(
    yyjson
    GIT_REPOSITORY https://github.com/ibireme/yyjson.git
    GIT_TAG 0.8.0
)
FetchContent_GetProperties(yyjson)
if(NOT yyjson_POPULATED)
    FetchContent_Populate(yyjson)
    # Build yyjson as static library
    set(YYJSON_BUILD_MISC OFF CACHE BOOL "" FORCE)
    set(YYJSON_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(YYJSON_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    add_subdirectory(${yyjson_SOURCE_DIR} ${yyjson_BINARY_DIR})
endif()

# Add library target
add_library(bluesky-client
    src/bluesky_client.cpp
)

# Set include directories for the library
target_include_directories(bluesky-client
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${OPENSSL_INCLUDE_DIR}
)

# Link dependencies
target_link_libraries(bluesky-client
    PRIVATE
        OpenSSL::SSL
        OpenSSL::Crypto
        httplib::httplib
        yyjson
        Threads::Threads
)

# Add example executable
add_executable(bluesky-example
    examples/main.cpp
)

target_link_libraries(bluesky-example
    PRIVATE
        bluesky-client
)

# Testing configuration
if(BUILD_TESTS)
    # Enable testing
    enable_testing()
    
    # Fetch Google Test
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.14.0
    )
    FetchContent_MakeAvailable(googletest)
    
    # Add test executable
    add_executable(bluesky-tests
        tests/test_bluesky_client.cpp
    )
    
    target_link_libraries(bluesky-tests
        PRIVATE
            bluesky-client
            GTest::gtest_main
    )
    
    # Add tests to CTest
    include(GoogleTest)
    gtest_discover_tests(bluesky-tests)
endif()

# Install rules
install(TARGETS bluesky-client
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
    INCLUDES DESTINATION include
)

install(FILES include/bluesky_client.hpp
    DESTINATION include
)
