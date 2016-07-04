# @author Pascal Boeschoten

# -------
# Files listing
# -------

set(RORC_LIB_HEADERS
  "${CMAKE_CURRENT_BINARY_DIR}/../include/RORC/Version.h" # Generated header
  ../include/RORC/CardDummy.h # Normal header
  ../include/RORC/CardFactory.h # Normal header
  ../include/RORC/CardInterface.h # Normal header
  ../include/RORC/ChannelParameters.h # Normal header
  ../include/RORC/Page.h # Normal header
  ../include/RORC/RORC.h # Normal header
)

set(RORC_LIB_SOURCES
#  CardDummy.cxx
#  CardFactory.cxx
  ChannelParameters.cxx
  ChannelPaths.cxx
  DummyChannelMaster.cxx
  MemoryMappedFile.cxx
  ReadyFifoWrapper.cxx
  RorcDeviceFinder.cxx
)

if(PDA_FOUND)
  list(APPEND RORC_LIB_SOURCES
    c/interface/header_common.c
    c/interface/header_crorc.c
    c/interface/header_cru.c
    c/interface/interface.c
    c/rorc/aux.c
    c/rorc/ddl.c
    c/rorc/rorc.c
    #CardPdaBase.cxx
    #CardPdaCrorc.cxx
    ChannelMaster.cxx
    ChannelMasterFactory.cxx
    CrorcChannelMaster.cxx
    #CruChannelMaster.cxx
    PdaBar.cxx
    PdaDevice.cxx
    PdaDmaBuffer.cxx
  )
endif()

set(RORC_UTIL_EXAMPLE_SOURCES
  Example.cxx
)


# -------
# RORC library
# -------

set(RORC_LIB_NAME rorc)
add_library(${RORC_LIB_NAME} SHARED ${RORC_LIB_SOURCES})
target_link_libraries(${RORC_LIB_NAME} 
  ${PDA_LIBRARIES}
  #boost_interprocess
  boost_filesystem
  boost_system
  rt # For shared memory
  pthread
)

# Set the build version. It will be used in the name of the lib, with corresponding
# symlinks created. SOVERSION could also be specified for api version. 
set_target_properties(${RORC_LIB_NAME} PROPERTIES
  VERSION ${VERSION}  # ${VERSION} was defined in the main CMakeLists.
  FRAMEWORK FALSE
  PUBLIC_HEADER "${RORC_LIB_HEADERS}" # specify the public headers
)


# -------
# RORC utilities
# -------

message("BOOST FOUND:  " ${BOOST_FOUND})
message("INTERPROCESS: " ${BOOST_INTERPROCESS_LIBRARY})
message("FILESYSTEM:   " ${BOOST_FILESYSTEM_LIBRARY})
message("SYSTEM:       " ${BOOST_SYSTEM_LIBRARY})

set(RORC_UTIL_EXAMPLE_NAME RorcExample)
add_executable(${RORC_UTIL_EXAMPLE_NAME} ${RORC_UTIL_EXAMPLE_SOURCES})
target_link_libraries(${RORC_UTIL_EXAMPLE_NAME} ${RORC_LIB_NAME})

# -------
# Targets export for all libraries and executables
# -------

# Says how and where to install software
install(TARGETS ${RORC_LIB_NAME} ${RORC_UTIL_EXAMPLE_NAME}
  EXPORT ${PROJECT_NAME}Targets            # for downstream dependencies
  ARCHIVE DESTINATION lib COMPONENT libs   # static lib
  RUNTIME DESTINATION bin COMPONENT libs   # binaries
  LIBRARY DESTINATION lib COMPONENT libs   # shared lib
  FRAMEWORK DESTINATION bin COMPONENT libs # for mac
  PUBLIC_HEADER DESTINATION include/${PROJECT_NAME} COMPONENT devel   # headers for mac (note the different component -> different package)
  INCLUDES DESTINATION include             # headers
)

