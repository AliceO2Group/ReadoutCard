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

/// \file TestChannelFactoryUtils.cxx
/// \brief Test of the ChannelFactoryUtils class
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include "ReadoutCard/BarInterface.h"
#include "ReadoutCard/ChannelFactory.h"
#include "ReadoutCard/DmaChannelInterface.h"
#include "Factory/ChannelFactoryUtils.h"
#include <memory>

#define BOOST_TEST_MODULE RORC_TestChannelFactoryUtils
#define BOOST_TEST_MAIN
#define BOOST_TEST_DYN_LINK
#include <boost/test/unit_test.hpp>
#include <assert.h>
#include <string>

using namespace ::o2::roc;

namespace
{

// Helper function for calling the DMA channel factory
std::unique_ptr<DmaChannelInterface> produceDma(int seqNumber)
{
  auto parameters = Parameters::makeParameters(PciSequenceNumber("#" + seqNumber), 0);
  return ChannelFactoryUtils::dmaChannelFactoryHelper<DmaChannelInterface>(parameters);
}

// Helper function for calling the BAR factory
std::unique_ptr<BarInterface> produceBar(int seqNumber)
{
  auto parameters = Parameters::makeParameters(PciSequenceNumber("#" + seqNumber), 0);
  return ChannelFactoryUtils::barFactoryHelper<BarInterface>(parameters);
}

// This tests if the FactoryHelper functions for the DMA channel and the BAR maps to the expected types.
// Unfortunately, we can't test the entire make function, since it depends on having actual PCI cards installed, so
// we test only part of its implementation.
BOOST_AUTO_TEST_CASE(ChannelFactoryHelperTest)
{
  BOOST_CHECK(nullptr != dynamic_cast<CrorcDmaChannel*>(produceDma(0).get()));
  BOOST_CHECK(nullptr != dynamic_cast<CrorcBar*>(produceBar(0).get()));
  BOOST_CHECK(nullptr != dynamic_cast<CruDmaChannel*>(produceDma(1).get()));
  BOOST_CHECK(nullptr != dynamic_cast<CruBar*>(produceBar(1).get()));
}

} // Anonymous namespace
