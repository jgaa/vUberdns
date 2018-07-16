# Here are registered all external projects
#
# Usage:
# add_dependencies(TARGET externalProjectName)
# target_link_libraries(TARGET PRIVATE ExternalLibraryName)

set(EXTERNAL_PROJECTS_PREFIX ${CMAKE_BINARY_DIR}/external-projects)
set(EXTERNAL_PROJECTS_INSTALL_PREFIX ${EXTERNAL_PROJECTS_PREFIX}/installed)

include(GNUInstallDirs)

# MUST be called before any add_executable() # https://stackoverflow.com/a/40554704/8766845
link_directories(${EXTERNAL_PROJECTS_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR})
include_directories($<BUILD_INTERFACE:${EXTERNAL_PROJECTS_INSTALL_PREFIX}/${CMAKE_INSTALL_INCLUDEDIR}>)

include(ExternalProject)

ExternalProject_Add(externalWarlib
    PREFIX "${EXTERNAL_PROJECTS_PREFIX}"
    GIT_REPOSITORY "https://github.com/jgaa/warlib.git"
    GIT_TAG "master"
    CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${EXTERNAL_PROJECTS_INSTALL_PREFIX}  -DBoost_USE_STATIC_LIBS=${Boost_USE_STATIC_LIBS} -DWAR_AUTORUN_UNIT_TESTS=0
    TEST_BEFORE_INSTALL 0
    TEST_AFTER_INSTALL 0
    )
