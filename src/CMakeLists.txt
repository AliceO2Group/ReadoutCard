# @author Pascal Boeschoten

# -------
# Files listing
# -------

set(RORC_LIB_HEADERS
  "${CMAKE_CURRENT_BINARY_DIR}/../include/RORC/Version.h" # Generated header
  # Normal headers
  ../include/RORC/CardType.h 
  ../include/RORC/ChannelFactory.h
  ../include/RORC/ChannelMasterInterface.h
  ../include/RORC/ChannelSlaveInterface.h
  ../include/RORC/Exception.h
  ../include/RORC/GeneratorPattern.h
  ../include/RORC/LoopbackMode.h
  ../include/RORC/Page.h
  ../include/RORC/Parameters.h
  ../include/RORC/PciId.h
  ../include/RORC/RegisterReadWriteInterface.h
  ../include/RORC/ResetLevel.h
  ../include/RORC/RORC.h
)

set(RORC_LIB_SOURCES
  CardType.cxx
  Factory/ChannelFactory.cxx
  Factory/ChannelUtilityFactory.cxx
  ChannelParameters.cxx
  ChannelPaths.cxx
  Dummy/DummyChannelMaster.cxx
  Dummy/DummyChannelSlave.cxx
  Exception.cxx
  LoopbackMode.cxx
  MemoryMappedFile.cxx
  ResetLevel.cxx
  RorcStatusCode.cxx
  Utilities/Common.cxx
  Utilities/Options.cxx
  Utilities/Program.cxx
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
    Cru/Temperature.cxx
    Pda/PdaBar.cxx
    Pda/PdaDevice.cxx
    Pda/PdaDmaBuffer.cxx
    RorcDevice.cxx
  )
  set(PDA_LIBRARIES_MAYBE ${PDA_LIBRARIES})
else()
  set(PDA_LIBRARIES_MAYBE "")
endif()


# -------
# RORC library
# -------

set(RORC_LIB_NAME rorc)
add_library(${RORC_LIB_NAME} SHARED ${RORC_LIB_SOURCES})
target_link_libraries(${RORC_LIB_NAME} 
  ${PDA_LIBRARIES_MAYBE}
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
  target_link_libraries(${execname} ${RORC_LIB_NAME} ${ARGN})
endfunction()

if(PDA_FOUND)
  build_util_exec(rorc-example Example.cxx)
  build_util_exec(rorc-reg-read Utilities/ProgramRegisterRead.cxx)
  build_util_exec(rorc-reg-read-range Utilities/ProgramRegisterReadRange)
  build_util_exec(rorc-reg-write Utilities/ProgramRegisterWrite.cxx)
  build_util_exec(rorc-list-cards Utilities/ProgramListCards.cxx)
  build_util_exec(rorc-print-fifo Utilities/ProgramPrintFifo.cxx)
  build_util_exec(rorc-cru-blink Utilities/ProgramCruBlink.cxx)
  build_util_exec(rorc-sanity-check Utilities/ProgramSanityCheck.cxx)
  build_util_exec(rorc-channel-init Utilities/ProgramInitChannel.cxx Configuration-static curl)
  build_util_exec(rorc-channel-cleanup Utilities/ProgramCleanup.cxx)
  build_util_exec(rorc-dma-bench Utilities/ProgramDmaBench.cxx)
  build_util_exec(rorc-cru-experimental-dma Utilities/ProgramCruExperimentalDma.cxx)
endif()

if(DIM_FOUND)
  build_util_exec(rorc-alf-server Utilities/ProgramAliceLowlevelFrontendServer.cxx ${DIM_LIBRARY})
  build_util_exec(rorc-alf-client Utilities/ProgramAliceLowlevelFrontendClient.cxx ${DIM_LIBRARY})
endif()

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

