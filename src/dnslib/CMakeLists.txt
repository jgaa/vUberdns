project(vudnslib VERSION ${VUDNS_VERSION} LANGUAGES CXX)

message(STATUS "Configuring ${PROJECT_NAME} ${VUDNS_VERSION}")

add_library(${PROJECT_NAME}
    src/AnswerBase.cpp
    src/DnsDaemon.cpp
    src/DnsDaemonImpl.cpp
    src/Zone.cpp
    src/ZoneMemoryImpl.cpp
    src/ZoneMemoryImpl.h
    src/ZoneMgr.cpp
    src/ZoneMgrMemory.cpp
    src/ZoneMgrMemory.h
    src/ZoneMgrJson.cpp
    src/ZoneMgrJson.h
    src/HttpServer.cpp
    include/vudnslib//HttpServer.h
    )

target_include_directories(${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${Boost_INCLUDE_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    PRIVATE ${CMAKE_BINARY_DIR}/generated-include/
    $<INSTALL_INTERFACE:include>
    PRIVATE $Boost_INCLUDE_DIR
    PRIVATE src)

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 17)
add_dependencies(${PROJECT_NAME} externalWarlib externalRestcCpp)

install(TARGETS ${PROJECT_NAME} EXPORT ${PROJECT_NAME}Config
    ARCHIVE  DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY  DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME  DESTINATION ${CMAKE_INSTALL_BINDIR})  # This is for Windows
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(EXPORT ${PROJECT_NAME}Config DESTINATION share/${PROJECT_NAME}/cmake)

# This makes the project importable from the build directory
export(TARGETS ${PROJECT_NAME} FILE DnsLibraryConfig.cmake)

target_compile_definitions(${PROJECT_NAME} PUBLIC -DBOOST_COROUTINES_NO_DEPRECATION_WARNING=1)
target_compile_definitions(${PROJECT_NAME} PUBLIC -DBOOST_COROUTINE_NO_DEPRECATION_WARNING=1)

