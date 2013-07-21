project(sdstdem2asc)

cmake_minimum_required(VERSION 2.8)

# append the local cmake/modules path inside the MAKE_MODULE_PATH
# variable which stores the directories of additional CMake modules
# (ie. MacroOutOfSourceBuild.cmake):
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/modules ${CMAKE_MODULE_PATH})

# the macro below forces the build directory to be the 'build'
# subdirectory of the source directory:
include(MacroEnsureOutOfSourceBuild)
macro_ensure_out_of_source_build()

# prerequsites
include(cmake/gdal-library-prereq.cmake)

# where to install?
set(CMAKE_INSTALL_PREFIX "/usr/local")

include_directories(inc)

add_executable(sdtsdem2asc
  sdtsdem2asc.cc
  libsrc/SafeFormat.cc
)
target_link_libraries(sdtsdem2asc
  gdal
)

#=== INSTALL ===================================
install(TARGETS sdtsdem2asc
        DESTINATION bin
)

install(
  FILES
    "${PROJECT_SOURCE_DIR}/sdtsdem2asc.1"
  DESTINATION "share/man/man1"
)

#=========================================================
# uninstall target (per CMake FAQ)
include(cmake/uninstall-common.cmake)
