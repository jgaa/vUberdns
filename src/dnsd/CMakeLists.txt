project(vudnsd VERSION ${VUDNS_VERSION} LANGUAGES CXX)

message(STATUS "Configuring ${PROJECT_NAME} ${VUDNS_VERSION}")

add_executable(${PROJECT_NAME}
    src/main.cpp
    )

target_include_directories(${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../dnslib/include>
    PRIVATE ${CMAKE_BINARY_DIR}/generated-include/
    $<INSTALL_INTERFACE:include>
    PRIVATE src)

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 14)
add_dependencies(${PROJECT_NAME} externalWarlib vudnslib)

target_link_libraries(${PROJECT_NAME} PRIVATE
    ${Boost_LIBRARIES}
    warcore
    vudnslib
    )

install(TARGETS ${PROJECT_NAME} EXPORT ${PROJECT_NAME}App
    ARCHIVE  DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY  DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME  DESTINATION ${CMAKE_INSTALL_BINDIR})  # This is for Windows

target_compile_definitions(${PROJECT_NAME} PUBLIC -DBOOST_COROUTINES_NO_DEPRECATION_WARNING=1)
target_compile_definitions(${PROJECT_NAME} PUBLIC -DBOOST_COROUTINE_NO_DEPRECATION_WARNING=1)
target_compile_definitions(${PROJECT_NAME} PUBLIC -DAPP_VERSION=\"${VUDNS_VERSION}\")
