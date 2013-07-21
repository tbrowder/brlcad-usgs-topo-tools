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
