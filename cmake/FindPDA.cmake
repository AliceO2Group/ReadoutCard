# - Try to find PDA (Portable Driver Architecture)
# Once done this will define
#  PDA_FOUND        - System has PDA
#  PDA_INCLUDE_DIRS - The PDA include directories
#  PDA_LIBRARIES    - The libraries needed to use PDA
#  PDA_DEFINITIONS  - Compiler switches required for using PDA

find_package(PkgConfig)

#set(PDA_PREFIX "/usr/local" CACHE PATH "A path where to look for PDA in addition to default paths.")

find_path(PDA_INCLUDE_DIR pda/device_operator.h)
find_library(PDA_LIBRARY NAMES pda)

set(PDA_LIBRARIES ${PDA_LIBRARY})
set(PDA_INCLUDE_DIRS ${PDA_INCLUDE_DIR})

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set PDA_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(PDA DEFAULT_MSG PDA_LIBRARY PDA_INCLUDE_DIR)

# Because case is a problem some times
#set(PDA_FOUND ${PDA_FOUND}) # Not this time!

mark_as_advanced(PDA_INCLUDE_DIR PDA_LIBRARY)
