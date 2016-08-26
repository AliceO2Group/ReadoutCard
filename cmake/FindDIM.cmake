# - Try to find DIM
# Once done this will define
#  DIM_FOUND        - System has DIM
#  DIM_INCLUDE_DIRS - The DIM include directories
#  DIM_LIBRARIES    - The libraries needed to use DIM

find_package(PkgConfig)

find_path(DIM_INCLUDE_DIR dim/dim.h)
find_library(DIM_LIBRARY NAMES dim)

set(DIM_LIBRARIES ${DIM_LIBRARY})
set(DIM_INCLUDE_DIRS ${DIM_INCLUDE_DIR})

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set DIM_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(DIM DEFAULT_MSG DIM_LIBRARY DIM_INCLUDE_DIR)

# Because case is a problem some times
set(DIM_FOUND ${DIM_FOUND})

mark_as_advanced(DIM_INCLUDE_DIR DIM_LIBRARY)
