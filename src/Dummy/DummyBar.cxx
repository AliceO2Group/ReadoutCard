// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file DummyBar.cxx
/// \brief Implementation of the DummyBar class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include "DummyBar.h"
#include <iostream>

// using std::cout;

namespace AliceO2
{
namespace roc
{

DummyBar::DummyBar(const Parameters& /*parameters*/)
{
  /*  mBarIndex = parameters.getChannelNumberRequired();
  auto id = parameters.getCardIdRequired();

//  cout << "DummyBar::DummyBar(";
  if (auto serial = boost::get<int>(&id)) {
//    cout << "serial:" << *serial << ", ";
  }
  if (auto address = boost::get<PciAddress>(&id)) {
//    cout << "address:" << address->toString() << ", ";
  }

//  cout << "BAR:" << mBarIndex << ")\n";*/
}

DummyBar::~DummyBar()
{
  //  cout << "DummyBar::~DummyBar()\n";
}

uint32_t DummyBar::readRegister(int /*index*/)
{
  //  cout << "DummyBar::readRegister(" << index << ")\n";
  return 0;
}

void DummyBar::writeRegister(int /*index*/, uint32_t /*value*/)
{
  //  cout << "DummyBar::writeRegister(index:" << index << ", value:" << value << ")\n";
}

void DummyBar::modifyRegister(int /*index*/, int /*position*/, int /*width*/, uint32_t /*value*/)
{
  //  cout << "DummyBar::modifyRegister(index:" << index << ", position:" << position ",
  //  width " << width ", value:" << value << ")\n";
}

void DummyBar::configure(bool /*force*/)
{
  std::cout << "Configure unavailable for dummy interfaces" << std::endl;
}
} // namespace roc
} // namespace AliceO2
