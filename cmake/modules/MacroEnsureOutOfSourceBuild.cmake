#=================================================================================================
#
# from:
#
#   http://cmake-modules.googlecode.com/svn/trunk/Modules/Macros/MacroEnsureOutOfSourceBuild.cmake
#
#=================================================================================================
#
# - MACRO_ENSURE_OUT_OF_SOURCE_BUILD(<errorMessage>)
# MACRO_ENSURE_OUT_OF_SOURCE_BUILD(<errorMessage>)
#
# Copyright (c) 2006, Alexander Neundorf, <neundorf@kde.org>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.
#
#=================================================================================================
#
# Modified by Tom Browder 
#
#
# Copyright (c) 2013, Tom Browder, <tom.browder@gmail.com>
#
#=================================================================================================

macro(macro_ensure_out_of_source_build)
  # ensure we are NOT in same dir
  string(COMPARE EQUAL "${CMAKE_SOURCE_DIR}" "${CMAKE_BINARY_DIR}" _insource)
  if (_insource)
    message(FATAL_ERROR 
            "\nBuilds are prohibited in all but the \"build\" subdir of the source directory!\n"
            "Be sure and remove file \"CMakeCache.txt\" and subdirectory \"CMakeFiles\" in this directory before continuing.\n"
    )
  endif ()

  # if we are in a 'build' subdir of the source dir we are good to go
  string(COMPARE EQUAL "${CMAKE_SOURCE_DIR}/build" "${CMAKE_BINARY_DIR}" _inbuild)
  if (_inbuild)
    message(STATUS "Builds okay in subdir build in the source directory.")
  else()
    message(FATAL_ERROR 
            "\nBuilds are prohibited in all but the \"build\" subdir of the source directory!\n"
            "Be sure and remove file \"CMakeCache.txt\" and subdirectory \"CMakeFiles\" in this directory before continuing.\n"
    )
  endif ()
endmacro()
