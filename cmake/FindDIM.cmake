# - Try to find DIM
# Once done this will define
#  DIM_FOUND        - System has DIM
#  DIM_INCLUDE_DIRS - The DIM include directories
#  DIM_LIBRARIES    - The libraries needed to use DIM
#
# This script can use the following variables:
#  DIM_ROOT - Installation root to tell this module where to look. (it tries /usr and /usr/local otherwise)

find_package(PkgConfig)

# find includes
find_path(DIM_INCLUDE_DIR dim.h
        HINTS ${DIM_ROOT} /usr/local/include /usr/include PATH_SUFFIXES "dim")
# Remove the final "dim"
get_filename_component(DIM_INCLUDE_DIR ${DIM_INCLUDE_DIR} DIRECTORY)

# find libraries
find_library(DIM_LIBRARY NAMES dim HINTS /usr/local/lib /usr/lib ${DIM_ROOT})

set(DIM_LIBRARIES ${DIM_LIBRARY})
set(DIM_INCLUDE_DIRS ${DIM_INCLUDE_DIR})

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set DIM_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(DIM DEFAULT_MSG DIM_LIBRARY DIM_INCLUDE_DIR)

if(${DIM_FOUND})
    message(STATUS "DIM found : ${DIM_LIBRARY}")
    mark_as_advanced(DIM_INCLUDE_DIR DIM_LIBRARY)

    # add target
    if(NOT TARGET dim::dim)
        add_library(dim::dim INTERFACE IMPORTED)
        set_target_properties(dim::dim PROPERTIES
          INTERFACE_INCLUDE_DIRECTORIES "${DIM_INCLUDE_DIR}"
          INTERFACE_LINK_LIBRARIES "${DIM_LIBRARY}"
        )
    endif()
endif()
