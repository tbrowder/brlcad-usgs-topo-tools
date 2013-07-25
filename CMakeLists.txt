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

# needed system functions
# need some defines and a config file
include(CheckFunctionExists)
include(CheckIncludeFiles)

check_function_exists(atoi HAVE_ATOI)
check_function_exists(system HAVE_SYSTEM)
check_function_exists(unlink HAVE_UNLINK)

# prerequsites
include(cmake/gdal-library-prereq.cmake)
include(cmake/brlcad-library-prereq.cmake)

# where to install?
set(CMAKE_INSTALL_PREFIX "/usr/local")

add_subdirectory(progs)

#=========================================================
# uninstall target (per CMake FAQ)
include(cmake/uninstall-common.cmake)
