// Copyright CERN and copyright holders of ALICE O2. This software is
// distributed under the terms of the GNU General Public License v3 (GPL
// Version 3), copied verbatim in the file "COPYING".
//
// See http://alice-o2.web.cern.ch/license for full licensing information.
//
// In applying this license CERN does not waive the privileges and immunities
// granted to it by virtue of its status as an Intergovernmental Organization
// or submit itself to any jurisdiction.

/// \file CrorcBar.h
/// \brief Definition of the CrorcBar class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#ifndef ALICEO2_SRC_READOUTCARD_CRORC_CRORCBAR_H_
#define ALICEO2_SRC_READOUTCARD_CRORC_CRORCBAR_H_

#include "BarInterfaceBase.h"
#include "Crorc.h"
#include "Crorc/Constants.h"
#include "Utilities/Util.h"

namespace AliceO2
{
namespace roc
{

class CrorcBar final : public BarInterfaceBase
{
 public:
  CrorcBar(const Parameters& parameters, std::unique_ptr<RocPciDevice> rocPciDevice);
  virtual ~CrorcBar();
  //virtual void checkReadSafe(int index) override;
  //virtual void checkWriteSafe(int index, uint32_t value) override;

  virtual CardType::type getCardType() override
  {
    return CardType::Crorc;
  }

  virtual boost::optional<int32_t> getSerial() override;
  virtual boost::optional<std::string> getFirmwareInfo() override;
  virtual int getEndpointNumber() override;
};

} // namespace roc
} // namespace AliceO2

#endif // ALICEO2_SRC_READOUTCARD_CRORC_CRORCBAR_H_
