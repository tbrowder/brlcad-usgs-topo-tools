project(sdstdem2asc)

cmake_minimum_required(VERSION 2.8)

# where to install?
set(CMAKE_INSTALL_PREFIX "/usr/local")

# need some defines and maybe a config file
include(${CMAKE_ROOT}/Modules/CheckFunctionExists.cmake)
include(${CMAKE_ROOT}/Modules/CheckIncludeFiles.cmake)

# prereqs
find_library(GDAL REQUIRED
  NAMES gdal
  PATHS /usr/local/lib /usr/lib
)
if(GDAL)
 include_directories(${GDAL_INCLUDE_DIRS})
  message(
    "Required GDAL/OGR library was found."
  )
else()
  message(FATAL_ERROR
    "FATAL ERROR: Required GDAL/OGR library not found."
  )
endif()

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

# === uninstall target (per CMake FAQ) ==========
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY)
add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake)
