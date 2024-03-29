// Copyright 2019-2020 CERN and copyright holders of ALICE O2.
// See https://alice-o2.web.cern.ch/copyright for details of the copyright holders.
// All rights not expressly granted are reserved.
//
// This software is distributed under the terms of the GNU General Public
// License v3 (GPL Version 3), copied verbatim in the file "COPYING".
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file TestMemoryMappedFile.cxx
/// \brief Test of the MemoryMappedFile class
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include "ReadoutCard/MemoryMappedFile.h"

#define BOOST_TEST_MODULE RORC_TestMemoryMappedFile
#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>
#include <assert.h>
#include <string>
#include "ReadoutCard/Exception.h"

namespace
{

using namespace ::o2::roc;

const std::string filePath("/tmp/AliceO2_MemoryMappedFile_Test");
const std::string badFilePath("/tmp/AliceO2_MemoryMappedFile_Test/12345abcdf/xyz/bad/path/");
const size_t fileSize(4 * 1024);

const std::string testString("HelloTest!");

BOOST_AUTO_TEST_CASE(MemoryMappedFileTest)
{
  // Write into it
  {
    MemoryMappedFile mmf(filePath.c_str(), fileSize);
    auto data = static_cast<char*>(mmf.getAddress());
    auto size = mmf.getSize();

    BOOST_CHECK_MESSAGE(size == fileSize, "Unexpected file size");

    for (size_t i = 0; i < size; ++i) {
      data[i] = char(i % 255);
    }
  }

  // Read from it
  {
    MemoryMappedFile mmf(filePath.c_str(), fileSize);
    auto data = static_cast<char*>(mmf.getAddress());
    auto size = mmf.getSize();

    BOOST_CHECK_MESSAGE(size == fileSize, "Unexpected file size");

    for (size_t i = 0; i < size; ++i) {
      BOOST_CHECK_MESSAGE(data[i] == char(i % 255), "Unexpected value");
    }
  }
}

BOOST_AUTO_TEST_CASE(MemoryMappedFileTest2)
{
  BOOST_CHECK_THROW(MemoryMappedFile(badFilePath.c_str(), fileSize), MemoryMapException);
}

} // Anonymous namespace
