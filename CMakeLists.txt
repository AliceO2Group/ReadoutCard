####################################
# General project definition
####################################

CMAKE_MINIMUM_REQUIRED(VERSION 3.5.2 FATAL_ERROR)

### CMP0025   Compiler id for Apple Clang is now AppleClang.
### CMP0042   MACOSX_RPATH is enabled by default.

FOREACH (p
        CMP0025 # CMake 3.0
        CMP0042 # CMake 3.0
        )
    IF (POLICY ${p})
        cmake_policy(SET ${p} NEW)
    ENDIF ()
endforeach ()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake) # project specific cmake dir
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/../cmake)

project(ReadoutCard)

# Load some basic macros which are needed later on
include(O2Utils)
include(ReadoutCardDependencies)
include(O2)

# Set the default build type to "RelWithDebInfo"
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "RelWithDebInfo"
            CACHE
            STRING "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel Coverage."
            FORCE)
endif(NOT CMAKE_BUILD_TYPE)

# Set the version number of your project here (format is MAJOR.MINOR.PATCHLEVEL - e.g. 1.0.0)
set(VERSION_MAJOR "0")
set(VERSION_MINOR "0")
set(VERSION_PATCH "0")
set(VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH})

# C++14
IF (CMAKE_VERSION VERSION_LESS 3.1)
    include(CheckCXXCompilerFlag)
    CHECK_CXX_COMPILER_FLAG(-std=c++14 COMPILER_SUPPORTS_CXX14)
    if (COMPILER_SUPPORTS_CXX14)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")
    else ()
        message(ERROR "The compiler ${CMAKE_CXX_COMPILER} has no C++14 support. Please use a different C++ compiler.")
    endif ()
ELSE ()
    set(CMAKE_CXX_STANDARD 14)  # proper way in CMake >= 3.1
ENDIF ()

# Add compiler flags for warnings and (more importantly) fPIC and debug symbols
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pedantic -Wextra -fPIC")


####################################
# Module, library and executable definition
####################################

set(MODULE_NAME "ReadoutCard")

link_o2_subproject(Common)
link_o2_subproject(InfoLogger)

O2_SETUP(NAME ${MODULE_NAME})

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
  src/RorcStatusCode.cxx
  src/CommandLineUtilities/Common.cxx
  src/CommandLineUtilities/Options.cxx
)

# Produce the final Version.h using template Version.h.in and substituting variables.
# We don't want to polute our source tree with it, thus putting it in binary tree.
configure_file("include/${MODULE_NAME}/Version.h.in"
        "${CMAKE_CURRENT_BINARY_DIR}/include/${MODULE_NAME}/Version.h"
        @ONLY
        )

include_directories(
  ${CMAKE_CURRENT_BINARY_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}/src
)
message("CMAKE INCLUDE: ${CMAKE_CURRENT_SOURCE_DIR}/src")

set(LIBRARY_NAME ${MODULE_NAME})
set(BUCKET_NAME o2_readoutcard_bucket)


if(ALICEO2_READOUTCARD_PDA_ENABLED)
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
  )
  if(PYTHONLIBS_FOUND)
    list(APPEND SRCS
      src/PythonInterface.cxx
    )
    set(BUCKET_NAME o2_readoutcard_pda_python)
  else()
    set(BUCKET_NAME o2_readoutcard_pda)
  endif()
endif()

O2_GENERATE_LIBRARY()

# Helper function to build the utility executables
function(build_util_exec execname filename)
  O2_GENERATE_EXECUTABLE(
        EXE_NAME ${execname}
        SOURCES src/${filename}
        MODULE_LIBRARY_NAME ${LIBRARY_NAME}
        BUCKET_NAME ${BUCKET_NAME}
  )
endfunction()

build_util_exec(roc-bench-dma CommandLineUtilities/ProgramDmaBench.cxx)
build_util_exec(roc-reset CommandLineUtilities/ProgramReset.cxx)
build_util_exec(roc-reg-read CommandLineUtilities/ProgramRegisterRead.cxx)
build_util_exec(roc-reg-read-range CommandLineUtilities/ProgramRegisterReadRange)
build_util_exec(roc-reg-write CommandLineUtilities/ProgramRegisterWrite.cxx)

if(ALICEO2_READOUTCARD_PDA_ENABLED)
  build_util_exec(roc-channel-cleanup CommandLineUtilities/ProgramCleanup.cxx)
  build_util_exec(roc-example Example.cxx)
  build_util_exec(roc-flash CommandLineUtilities/ProgramFlash.cxx)
  build_util_exec(roc-flash-read CommandLineUtilities/ProgramFlashRead.cxx)
  build_util_exec(roc-list-cards CommandLineUtilities/ProgramListCards.cxx)
  #build_util_exec(roc-sanity-check CommandLineUtilities/ProgramSanityCheck.cxx)

  if(DIM_FOUND)
    O2_GENERATE_EXECUTABLE(
        EXE_NAME roc-alf-client
        SOURCES src/CommandLineUtilities/ProgramAliceLowlevelFrontendClient.cxx
        MODULE_LIBRARY_NAME ${LIBRARY_NAME}
        BUCKET_NAME o2_readoutcard_dim
   )
   O2_GENERATE_EXECUTABLE(
        EXE_NAME roc-alf-server
        SOURCES src/CommandLineUtilities/ProgramAliceLowlevelFrontendServer.cxx
        MODULE_LIBRARY_NAME ${LIBRARY_NAME}
        BUCKET_NAME o2_readoutcard_dim
   )

  else()
    message(INFO "DIM not found, ALF client & server will not be built")
  endif()

  if(PYTHONLIBS_FOUND)
    O2_GENERATE_EXECUTABLE(
        EXE_NAME roc-run-script
        SOURCES src/CommandLineUtilities/ProgramRunScript.cxx
        MODULE_LIBRARY_NAME ${LIBRARY_NAME}
        BUCKET_NAME o2_readoutcard_python
   )
  else()
    message(INFO "PythonLibs not found, 'roc-run-script' utility will not be built")
  endif()
endif()


add_subdirectory(doc)

set(CTEST_TESTING_TIMEOUT 15)

set(TEST_SRCS
  test/TestChannelFactoryUtils.cxx
  test/TestChannelPaths.cxx
  test/TestEnums.cxx
  test/TestInterprocessLock.cxx
  test/TestMemoryMappedFile.cxx
  test/TestParameters.cxx
  test/TestPciAddress.cxx
  test/TestProgramOptions.cxx
  test/TestRorcException.cxx
  test/TestSuperpageQueue.cxx
)

O2_GENERATE_TESTS(
 MODULE_LIBRARY_NAME ${LIBRARY_NAME}
 BUCKET_NAME ${BUCKET_NAME}
 TIMEOUT 30
 TEST_SRCS ${TEST_SRCS}
)

# TODO RECONSIDER THE THINGS BELOW AFTER EXTRACTION TO ITS OWN REPO
# Make sure we tell the topdir CMakeLists that we exist (if build from topdir)
get_directory_property(hasParent PARENT_DIRECTORY)
if(hasParent)
    set(PROJECT_${PROJECT_NAME} true PARENT_SCOPE)
endif()
include(PackageConfigurator)
