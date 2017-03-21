# @author  Barthélémy von Haller

# Compile C++ tests (here only one, but it will grab whatever cxx is in the test directory)
#file(GLOB_RECURSE TEST_FILES RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *.cxx)

set(TEST_FILES
    TestChannelFactoryUtils.cxx
    TestChannelPaths.cxx
    TestEnums.cxx
    TestInterprocessLock.cxx
    TestMemoryMappedFile.cxx
    TestParameters.cxx
    TestPciAddress.cxx
    TestProgramOptions.cxx
    TestRorcException.cxx
    TestSuperpageQueue.cxx
    TestUtil.cxx
)

# Iterate over all tests found. For each, declare an executable and add it to the tests list.
set(ALL_TESTS)
foreach(FILE ${TEST_FILES})
    set_test_name(${FILE} NAME)
    add_executable(${NAME} ${FILE})
    target_link_libraries(${NAME} 
        rorc 
        ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}) # link against our lib and boost test
    add_test(NAME ${NAME} COMMAND ${NAME})                              # this is how to add tests to CMake
endforeach(FILE ${TEST_FILES})

# TODO test against the shared libraries

include(CTest)
