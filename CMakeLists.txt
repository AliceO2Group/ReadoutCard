####################################
# General project definition
####################################

CMAKE_MINIMUM_REQUIRED(VERSION 3.5.2 FATAL_ERROR)

# Set cmake policy by version: https://cmake.org/cmake/help/latest/manual/cmake-policies.7.html
if(${CMAKE_VERSION} VERSION_LESS 3.12)
  cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
else()
  cmake_policy(VERSION 3.12)
endif()

# Define project
project(ReadoutCard
  VERSION 0.0.0
  DESCRIPTION "O2 ReadoutCard library"
  LANGUAGES C CXX
)

# Documentation dir
add_subdirectory(doc)

# Add compiler flags for warnings and debug symbols
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pedantic -Wextra")

# Set fPIC for all targets
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Set CMAKE_INSTALL_LIBDIR explicitly to lib (to avoid lib64 on CC7)
set(CMAKE_INSTALL_LIBDIR lib)

# Set the default build type to "RelWithDebInfo"
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE "RelWithDebInfo"
    CACHE
    STRING "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel Coverage."
    FORCE
  )
endif()


####################################
# Populate the Cru/Constants.h file with the register addresses contained in CRU/cru_table.py
####################################
execute_process(COMMAND python ${CMAKE_CURRENT_SOURCE_DIR}/src/Cru/cru_constants_populate.py
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src/Cru
  RESULT_VARIABLE status)
IF(NOT "${status}" EQUAL "0")
  MESSAGE(WARNING "Couldn't update CRU Register Addresses")
ENDIF()


####################################
# Dependencies
####################################

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

if(NOT APPLE)
  set(boost_python_component "python27")
  find_package(Python2 2.7 COMPONENTS Development)
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
find_package(Common REQUIRED)
find_package(InfoLogger REQUIRED)
find_package(PDA)
find_package(DIM)

####################################
# Module, library and executable definition
####################################

set(SRCS
  src/CardType.cxx
  src/Factory/ChannelFactory.cxx
  src/DmaChannelBase.cxx
  src/ChannelPaths.cxx
  src/Dummy/DummyDmaChannel.cxx
  src/Dummy/DummyBar.cxx
  src/ExceptionInternal.cxx
  src/MemoryMappedFile.cxx
  src/Parameters.cxx
  src/ParameterTypes/GeneratorPattern.cxx
  src/ParameterTypes/LoopbackMode.cxx
  src/ParameterTypes/PciAddress.cxx
  src/ParameterTypes/ResetLevel.cxx
  src/ParameterTypes/ReadoutMode.cxx
  src/Utilities/Hugetlbfs.cxx
  src/Utilities/MemoryMaps.cxx
  src/Utilities/Numa.cxx
  src/CommandLineUtilities/AliceLowlevelFrontend/Sca.cxx
  src/CommandLineUtilities/AliceLowlevelFrontend/ServiceNames.cxx
  src/CommandLineUtilities/Common.cxx
  src/CommandLineUtilities/Options.cxx
)

# Add sources requiring PDA
if(PDA_FOUND)
  list(APPEND SRCS
    src/DmaChannelPdaBase.cxx
    src/BarInterfaceBase.cxx
    src/Crorc/Crorc.cxx
    src/Crorc/CrorcDmaChannel.cxx
    src/Crorc/CrorcBar.cxx
    src/Cru/CruDmaChannel.cxx
    src/Cru/CruBar.cxx
    src/Pda/PdaBar.cxx
    src/Pda/PdaDevice.cxx
    src/Pda/PdaDmaBuffer.cxx
    src/RocPciDevice.cxx
    src/Swt/Swt.cxx
    $<$<BOOL:${Python2_FOUND}>:src/PythonInterface.cxx>
  )
endif()

# Create library
add_library(ReadoutCard SHARED ${SRCS})
target_include_directories(ReadoutCard
  PUBLIC
    $<INSTALL_INTERFACE:include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Link targets
target_link_libraries(ReadoutCard
  PUBLIC
    AliceO2::InfoLogger
    AliceO2::Common
    $<$<BOOL:${Python2_FOUND}>:Boost::python27>
    $<$<BOOL:${Python2_FOUND}>:Python2::Python>
  PRIVATE
    $<$<BOOL:${PDA_FOUND}>:pda::pda>
    Boost::filesystem
    Boost::system
    pthread
)

target_compile_definitions(ReadoutCard
  PRIVATE
    $<$<BOOL:${PDA_FOUND}>:ALICEO2_READOUTCARD_PDA_ENABLED>
)

# Use C++14
target_compile_features(ReadoutCard PUBLIC cxx_std_14)


####################################
# Executables
####################################

set(EXE_SRCS
  ProgramDmaBench.cxx
  ProgramReset.cxx
  ProgramRegisterRead.cxx
  ProgramRegisterReadRange
  ProgramRegisterWrite.cxx
)

set(EXE_NAMES
  roc-bench-dma
  roc-reset
  roc-reg-read
  roc-reg-read-range
  roc-reg-write
)

if(PDA_FOUND)
  list(APPEND EXE_SRCS
    ProgramBarStress.cxx
    ProgramCleanup.cxx
    ../Example.cxx
    ProgramFlashRead.cxx
    ProgramListCards.cxx
    ProgramMetrics.cxx
  )
  list(APPEND EXE_NAMES
    roc-bar-stress
    roc-channel-cleanup
    roc-example
    roc-flash
    roc-flash-read
    roc-list-cards
    roc-metrics
  )

  if(DIM_FOUND)
    list(APPEND DIM_EXE_SRCS
      AliceLowlevelFrontend/ProgramAliceLowlevelFrontendClient.cxx
      AliceLowlevelFrontend/ProgramAlfScaWriteSequence.cxx
      AliceLowlevelFrontend/ProgramAliceLowlevelFrontendServer.cxx
    )
    list(APPEND DIM_EXE_NAMES
      roc-alf-client
      roc-alf-sca-write-seq
      roc-alf-server
    )
  else()
    message(INFO "DIM not found, ALF client & server will not be built")
  endif()

  if(Python2_FOUND)
    list(APPEND EXE_SRCS ProgramRunScript.cxx)
    list(APPEND EXE_NAMES roc-run-script)
  else()
    message(INFO "PythonLibs not found, 'roc-run-script' utility will not be built")
  endif()
endif()


list(APPEND ALL_EXE_SRCS ${EXE_SRCS} ${DIM_EXE_SRCS})
list(APPEND ALL_EXE_NAMES ${EXE_NAMES} ${DIM_EXE_NAMES})

list(LENGTH ALL_EXE_SRCS count)
math(EXPR count "${count}-1")
foreach(i RANGE ${count})
  list(GET ALL_EXE_SRCS ${i} src)
  list(GET ALL_EXE_NAMES ${i} name)
  add_executable(${name} src/CommandLineUtilities/${src})
  target_include_directories(${name}
    PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}/src
  )
  target_link_libraries(${name}
    PRIVATE
      ReadoutCard
      Boost::program_options
  )
  if (${src} MATCHES "^AliceLowlevelFrontend.*")
    target_link_libraries(${name} PRIVATE dim::dim)
  endif()
endforeach()

####################################
# Tests
####################################

enable_testing()

set(TEST_SRCS
  test/TestChannelFactoryUtils.cxx
  test/TestChannelPaths.cxx
  test/TestCruDataFormat.cxx
  test/TestEnums.cxx
  #test/TestInterprocessLock.cxx
  test/TestMemoryMappedFile.cxx
  test/TestParameters.cxx
  test/TestPciAddress.cxx
  test/TestProgramOptions.cxx
  test/TestRorcException.cxx
  test/TestSuperpageQueue.cxx
)

if(PDA_FOUND)
  list(APPEND TEST_SRCS test/TestCruBar.cxx)
endif()

foreach (test ${TEST_SRCS})
  get_filename_component(test_name ${test} NAME)
  string(REGEX REPLACE ".cxx" "" test_name ${test_name})

  add_executable(${test_name} ${test})
   target_include_directories(${test_name}
    PRIVATE
      ${CMAKE_CURRENT_SOURCE_DIR}/src
  )
  target_link_libraries(${test_name}
    PRIVATE 
      ReadoutCard
      Boost::unit_test_framework
  )
  add_test(NAME ${test_name} COMMAND ${test_name})
  set_tests_properties(${test_name} PROPERTIES TIMEOUT 15)
endforeach()


####################################
# Install
####################################

# Install the hugetlbfs script separately, because it's not a compiled target
install(
  FILES src/roc-setup-hugetlbfs.sh
  DESTINATION ${CMAKE_INSTALL_PREFIX}/bin
  PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE
  RENAME roc-setup-hugetlbfs)

include(GNUInstallDirs)

# Build targets with install rpath on Mac to dramatically speed up installation
# https://gitlab.kitware.com/cmake/community/wikis/doc/cmake/RPATH-handling
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
list(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}" isSystemDir)
if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
  if("${isSystemDir}" STREQUAL "-1")
    set(CMAKE_INSTALL_RPATH "@loader_path/../${CMAKE_INSTALL_LIBDIR}")
  endif()
  set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
endif()
unset(isSystemDir)

# Install library
install(TARGETS ReadoutCard ${EXE_NAMES}
  EXPORT ReadoutCardTargets
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Create version file
include(CMakePackageConfigHelpers)
write_basic_package_version_file("${CMAKE_CURRENT_BINARY_DIR}/cmake/ReadoutCardConfigVersion.cmake"
  VERSION ${PACKAGE_VERSION}
  COMPATIBILITY AnyNewerVersion
)

# Install headers
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/ReadoutCard DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")

# Export targets
install(EXPORT ReadoutCardTargets
  FILE
    ReadoutCardTargets.cmake
  NAMESPACE
    AliceO2::
  DESTINATION
    ${CMAKE_INSTALL_LIBDIR}/cmake/ReadoutCard
)

# Configure and install Config files
configure_package_config_file(
  cmake/ReadoutCardConfig.cmake.in cmake/ReadoutCardConfig.cmake
  INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/ReadoutCard"
  PATH_VARS CMAKE_INSTALL_PREFIX
)

install(
  FILES
    "${CMAKE_CURRENT_BINARY_DIR}/cmake/ReadoutCardConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake/ReadoutCardConfigVersion.cmake"
  DESTINATION
    ${CMAKE_INSTALL_LIBDIR}/cmake/ReadoutCard
)
