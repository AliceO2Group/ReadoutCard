if(APPLE)
    set(rt_lib "")
    set(boost_python_component "")
else()
    set(rt_lib "rt")
    set(boost_python_component "python")
endif()

find_package(Boost 1.56
  COMPONENTS 
  unit_test_framework 
  filesystem 
  system 
  program_options 
  ${boost_python_component}
  REQUIRED
  )
find_package(Git QUIET)

# Python
find_package(PythonLibs 2.7)
if(PYTHONLIBS_FOUND)
    include_directories(${PYTHON_INCLUDE_DIRS})
endif()

# PDA
find_package(PDA)
if(PDA_FOUND)
    message(STATUS "PDA found")
    # Add definition to enable the PDA implementation in the code
    set(ALICEO2_RORC_PDA_ENABLED TRUE)
    add_definitions(-DALICEO2_RORC_PDA_ENABLED)
    include_directories(SYSTEM ${PDA_INCLUDE_DIRS})
else()
    message(WARNING "PDA not found, RORC module will have a dummy implementation only (skip, no error)")
endif(PDA_FOUND)

# DIM
find_package(DIM)
if(DIM_FOUND)
    include_directories(${DIM_INCLUDE_DIRS})
else()
    message(WARNING "DIM not found, RORC module's ALF utilities will not be compiled")
endif(DIM_FOUND)

o2_define_bucket(
  NAME
  o2_rorc_bucket

  DEPENDENCIES
  ${PDA_LIBRARIES_MAYBE}
  ${Boost_FILESYSTEM_LIBRARY}
  ${Boost_SYSTEM_LIBRARY}
  ${Boost_PROGRAM_OPTIONS_LIBRARY}
  InfoLogger
  Common
  ${rt_lib}
  pthread

  SYSTEMINCLUDE_DIRECTORIES
  ${Boost_INCLUDE_DIR}
)

o2_define_bucket(
  NAME
  o2_rorc_pda

  DEPENDENCIES
  o2_rorc_bucket
  ${PDA_LIBRARIES}

  SYSTEMINCLUDE_DIRECTORIES
  ${Boost_INCLUDE_DIR}
)


o2_define_bucket(
  NAME
  o2_rorc_dim

  DEPENDENCIES
  ${DIM_LIBRARY}

  INCLUDE_DIRECTORIES
)

o2_define_bucket(
  NAME
  o2_rorc_python

  DEPENDENCIES
  ${Boost_PYTHON_LIBRARY}
  ${PYTHON_LIBRARIES}

  SYSTEMINCLUDE_DIRECTORIES
  ${Boost_INCLUDE_DIR}
)

o2_define_bucket(
  NAME
  o2_rorc_pda_python

  DEPENDENCIES
  o2_rorc_bucket
  o2_rorc_pda
  o2_rorc_python
  ${PDA_LIBRARIES}

  SYSTEMINCLUDE_DIRECTORIES
  ${Boost_INCLUDE_DIR}
)
