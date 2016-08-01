# @author Pascal Boeschoten

# -------
# Files listing
# -------

set(RORC_LIB_HEADERS
  "${CMAKE_CURRENT_BINARY_DIR}/../include/RORC/Version.h" # Generated header
  ../include/RORC/ChannelFactory.h # Normal header
  ../include/RORC/ChannelMasterInterface.h # Normal header
  ../include/RORC/ChannelParameters.h # Normal header
  ../include/RORC/ChannelSlaveInterface.h # Normal header
  ../include/RORC/Page.h # Normal header
  ../include/RORC/RORC.h # Normal header
)

set(RORC_LIB_SOURCES
  CardType.cxx
  Factory/ChannelFactory.cxx
  Factory/ChannelUtilityFactory.cxx
  ChannelParameters.cxx
  ChannelPaths.cxx
  Dummy/DummyChannelMaster.cxx
  Dummy/DummyChannelSlave.cxx
  MemoryMappedFile.cxx
  RorcException.cxx
  RorcStatusCode.cxx
  RorcUtilities/RorcUtilsProgram.cxx
  Util.cxx
)

if(PDA_FOUND)
  list(APPEND RORC_LIB_SOURCES
    c/rorc/aux.c
    c/rorc/ddl.c
    c/rorc/rorc.c
    ChannelMaster.cxx
    ChannelSlave.cxx
    ChannelUtilityImpl.cxx
    Crorc/Crorc.cxx
    Crorc/CrorcChannelMaster.cxx
    Crorc/CrorcChannelSlave.cxx
    Cru/CruChannelMaster.cxx
    Cru/CruChannelSlave.cxx
    Pda/PdaBar.cxx
    Pda/PdaDevice.cxx
    Pda/PdaDmaBuffer.cxx
    RorcDevice.cxx
    RorcUtilities/RorcUtilsCommon.cxx
    RorcUtilities/RorcUtilsOptions.cxx
  )
endif()


# -------
# RORC library
# -------

set(RORC_LIB_NAME rorc)
add_library(${RORC_LIB_NAME} SHARED ${RORC_LIB_SOURCES})
target_link_libraries(${RORC_LIB_NAME} 
  ${PDA_LIBRARIES}
  ${Boost_FILESYSTEM_LIBRARY}
  ${Boost_SYSTEM_LIBRARY}
  ${Boost_PROGRAM_OPTIONS_LIBRARY}
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
# RORC utility executables
# -------

# Helper function to build the utility executables
function(build_util_exec execname filename)
  add_executable(${execname} ${filename})
  target_link_libraries(${execname} ${RORC_LIB_NAME})
endfunction()

build_util_exec(rorc-example Example.cxx)
build_util_exec(rorc-reg-read RorcUtilities/RorcUtilsReadRegister.cxx)
build_util_exec(rorc-reg-read-range RorcUtilities/RorcUtilsReadRegisterRange)
build_util_exec(rorc-reg-write RorcUtilities/RorcUtilsWriteRegister.cxx)
build_util_exec(rorc-dma-bench RorcUtilities/RorcUtilsDmaBench.cxx)
build_util_exec(rorc-list-cards RorcUtilities/RorcUtilsListCards.cxx)
build_util_exec(rorc-print-fifo RorcUtilities/RorcUtilsPrintFifo.cxx)
build_util_exec(rorc-cru-blink RorcUtilities/RorcUtilsCruBlink.cxx)
build_util_exec(rorc-sanity-check RorcUtilities/RorcUtilsSanityCheck.cxx)
build_util_exec(rorc-cleanup RorcUtilities/RorcUtilsCleanup.cxx)


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

