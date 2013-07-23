include_directories(../inc)

add_executable(sdtsdem2asc
  sdtsdem2asc.cc
  ../libsrc/SafeFormat.cc
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
    "${PROJECT_SOURCE_DIR}/progs/sdtsdem2asc.1"
  DESTINATION "share/man/man1"
)
