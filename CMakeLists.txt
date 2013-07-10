project(sdstdem2asc)

cmake_minimum_required(VERSION 2.8)

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
