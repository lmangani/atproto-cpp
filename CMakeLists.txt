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

# Add cpp-httplib
add_library(httplib INTERFACE)
target_include_directories(httplib INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/external/cpp-httplib)
target_compile_definitions(httplib INTERFACE CPPHTTPLIB_OPENSSL_SUPPORT)
add_library(httplib::httplib ALIAS httplib)

# Build yyjson
set(YYJSON_BUILD_MISC OFF CACHE BOOL "" FORCE)
set(YYJSON_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(YYJSON_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(external/yyjson)

# Add library target
add_library(bluesky-client
    src/bluesky_client.cpp
)

# Set include directories for the library
target_include_directories(bluesky-client
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/external/cpp-httplib
        ${CMAKE_CURRENT_SOURCE_DIR}/external/yyjson/src/
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

add_executable(bluesky-post
    examples/post.cpp
)

target_link_libraries(bluesky-example
    PRIVATE
        bluesky-client
)

target_link_libraries(bluesky-post
    PRIVATE
        bluesky-client
)

# Testing configuration
if(BUILD_TESTS)
    # Enable testing
    enable_testing()
    
    # Include Google Test
    include(FetchContent)
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
