#======================================================================
# BRL-CAD
find_library(BRLCAD
  NAMES bn bu brep rt
  PATHS ENV
)
if(BRLCAD)
  include_directories(${GDAL_INCLUDE_DIRS})
  message(
    "Required BRL-CAD libraries were found."
  )
else()
  message(FATAL_ERROR
    "FATAL ERROR: Required BRL-CAD libraries not found."
  )
endif()
