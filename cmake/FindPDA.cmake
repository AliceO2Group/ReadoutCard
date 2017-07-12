# - Try to find PDA (Portable Driver Architecture)
# Once done this will define
#  PDA_FOUND        - System has PDA
#  PDA_INCLUDE_DIRS - The PDA include directories
#  PDA_LIBRARIES    - The libraries needed to use PDA
#  PDA_DEFINITIONS  - Compiler switches required for using PDA
#
# This script can use the following variables:
#  PDA_ROOT - Installation root to tell this module where to look. (it tries /usr and /usr/local otherwise)

find_package(PkgConfig)

# find includes
find_path(PDA_INCLUDE_DIR device_operator.h
        HINTS ${PDA_ROOT}/include /usr/local/include /usr/include PATH_SUFFIXES "pda")
# Remove the final "pda"
get_filename_component(PDA_INCLUDE_DIR ${PDA_INCLUDE_DIR} DIRECTORY)

# find libraries
find_library(PDA_LIBRARY NAMES pda HINTS /usr/local/lib /usr/lib ${PDA_ROOT}/lib)

set(PDA_LIBRARIES ${PDA_LIBRARY})
set(PDA_INCLUDE_DIRS ${PDA_INCLUDE_DIR})

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set PDA_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(PDA DEFAULT_MSG PDA_LIBRARY PDA_INCLUDE_DIR)

mark_as_advanced(PDA_INCLUDE_DIR PDA_LIBRARY)
