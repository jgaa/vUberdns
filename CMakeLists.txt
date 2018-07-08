cmake_minimum_required(VERSION 3.5)

if (NOT DEFINED VUDNS_VERSION)
    set(VUDNS_VERSION 0.1.1)
endif()

project(vUberdns VERSION ${VUDNS_VERSION} LANGUAGES CXX)

include(GNUInstallDirs)

find_package(Boost REQUIRED COMPONENTS
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

add_subdirectory(src/dnslib)
add_subdirectory(src/dnsd)
