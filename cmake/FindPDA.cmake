# - Try to find PDA (Portable Driver Architecture)
# Once done this will define
#  PDA_FOUND        - System has PDA
#  PDA_INCLUDE_DIRS - The PDA include directories
#  PDA_LIBRARIES    - The libraries needed to use PDA
#
# This script can use the following variables:
#  PDA_ROOT - Installation root to tell this module where to look. (it tries /usr and /usr/local otherwise)

find_package(PkgConfig)

# find includes
find_path(PDA_INCLUDE_DIR pda.h
        HINTS ${PDA_INCLUDE_DIR} ${PDA_ROOT} ${PDA_ROOT}/include /usr/local/include /usr/include)
# find libraries
find_library(PDA_LIBRARY NAMES pda HINTS ${PDA_LIBRARY} ${PDA_ROOT} ${PDA_ROOT}/lib /usr/local/lib /usr/lib )

set(PDA_LIBRARIES ${PDA_LIBRARY})
set(PDA_INCLUDE_DIRS ${PDA_INCLUDE_DIR})

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set PDA_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(PDA DEFAULT_MSG PDA_LIBRARY PDA_INCLUDE_DIR)

if(${PDA_FOUND})
    message(STATUS "PDA found : ${PDA_LIBRARIES}")
    mark_as_advanced(PDA_INCLUDE_DIR PDA_LIBRARY)

    # add target
    if(NOT TARGET pda::pda)
        add_library(pda::pda INTERFACE IMPORTED)
        set_target_properties(pda::pda PROPERTIES
          INTERFACE_INCLUDE_DIRECTORIES "${PDA_INCLUDE_DIR}"
          INTERFACE_LINK_LIBRARIES "${PDA_LIBRARY}"
        )
    endif()
endif()
