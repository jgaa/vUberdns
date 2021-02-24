cmake_minimum_required(VERSION 3.8)

if (NOT DEFINED VUDNS_VERSION)
    set(VUDNS_VERSION 0.1.2)
endif()

project(vUberdns VERSION ${VUDNS_VERSION} LANGUAGES CXX)

include(GNUInstallDirs)

find_package(ZLIB REQUIRED)
find_package(OpenSSL REQUIRED)

find_package(Boost 1.70 REQUIRED COMPONENTS
    system
    program_options
    filesystem
    context
    coroutine
    regex
    chrono)

add_library(boost INTERFACE IMPORTED)
set_property(TARGET boost PROPERTY
    INTERFACE_INCLUDE_DIRECTORIES ${Boost_INCLUDE_DIR})

message(STATUS "Using ${CMAKE_CXX_COMPILER}")

include(cmake_scripts/external-projects.cmake)

IF(CMAKE_BUILD_TYPE MATCHES Debug)
    set(RESTC_CPP_LIB restc-cppD)
else()
    set(RESTC_CPP_LIB restc-cpp)
endif()

add_subdirectory(src/dnslib)
add_subdirectory(src/dnsd)
