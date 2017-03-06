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
  ../include/RORC/Parameters.h
  ../include/RORC/PciAddress.h
  ../include/RORC/PciId.h
  ../include/RORC/RegisterReadWriteInterface.h
  ../include/RORC/ResetLevel.h
  ../include/RORC/RORC.h
)

set(RORC_LIB_SOURCES
  CardType.cxx
  Factory/ChannelFactory.cxx
  Factory/ChannelUtilityFactory.cxx
  ChannelBase.cxx
  ChannelMasterBase.cxx
  ChannelPaths.cxx
  Dummy/DummyChannelMaster.cxx
  Dummy/DummyChannelSlave.cxx
  Exception.cxx
  ExceptionInternal.cxx
  GeneratorPattern.cxx
  LoopbackMode.cxx
  MemoryMappedFile.cxx
  Parameters.cxx
  PciAddress.cxx
  ResetLevel.cxx
  RorcStatusCode.cxx
  CommandLineUtilities/Common.cxx
  CommandLineUtilities/Options.cxx
  CommandLineUtilities/Program.cxx
  Utilities/System.cxx
)

if(ALICEO2_RORC_PDA_ENABLED)
  list(APPEND RORC_LIB_SOURCES
    c/rorc/aux.c
    c/rorc/ddl.c
    c/rorc/rorc.c
    ChannelMasterPdaBase.cxx
    ChannelSlave.cxx
    ChannelUtilityImpl.cxx
    Crorc/Crorc.cxx
    Crorc/CrorcChannelMaster.cxx
    Crorc/CrorcChannelSlave.cxx
    Cru/CruChannelMaster.cxx
    Cru/CruChannelSlave.cxx
    Pda/Pda.cxx
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
target_link_libraries(
  ${RORC_LIB_NAME} 
  ${PDA_LIBRARIES_MAYBE}
  ${Boost_FILESYSTEM_LIBRARY}
  ${Boost_SYSTEM_LIBRARY}
  ${Boost_PROGRAM_OPTIONS_LIBRARY}
  InfoLogger-shared
  Common
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

build_util_exec(rorc-dma-bench CommandLineUtilities/ProgramDmaBench.cxx)
build_util_exec(rorc-reset CommandLineUtilities/ProgramReset.cxx)
build_util_exec(rorc-reg-read CommandLineUtilities/ProgramRegisterRead.cxx)
build_util_exec(rorc-reg-read-range CommandLineUtilities/ProgramRegisterReadRange)
build_util_exec(rorc-reg-write CommandLineUtilities/ProgramRegisterWrite.cxx)

if(ALICEO2_RORC_PDA_ENABLED)
  build_util_exec(rorc-channel-cleanup CommandLineUtilities/ProgramCleanup.cxx)
  build_util_exec(rorc-cru-blink CommandLineUtilities/ProgramCruBlink.cxx)
  build_util_exec(rorc-cru-experimental-dma CommandLineUtilities/ProgramCruExperimentalDma.cxx)
  build_util_exec(rorc-example Example.cxx)
  build_util_exec(rorc-list-cards CommandLineUtilities/ProgramListCards.cxx)
  build_util_exec(rorc-print-fifo CommandLineUtilities/ProgramPrintFifo.cxx)
  build_util_exec(rorc-sanity-check CommandLineUtilities/ProgramSanityCheck.cxx)
  
  if(DIM_FOUND)
    build_util_exec(rorc-alf-client CommandLineUtilities/ProgramAliceLowlevelFrontendClient.cxx ${DIM_LIBRARY})
    build_util_exec(rorc-alf-server CommandLineUtilities/ProgramAliceLowlevelFrontendServer.cxx ${DIM_LIBRARY})
  else()
    message(INFO "DIM not found, ALF client & server will not be built")
  endif()
  
  if(PYTHONLIBS_FOUND)
    include_directories(${PYTHON_INCLUDE_DIRS})
    build_util_exec(rorc-run-script CommandLineUtilities/ProgramRunScript.cxx ${Boost_PYTHON_LIBRARY} ${PYTHON_LIBRARIES})
  else()
    message(INFO "PythonLibs not found, 'rorc-run-script' utility will not be built")
  endif()
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

