/// \file CruBar.h
/// \brief Definition of the CruBar class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#pragma once

#include "BarInterfaceBase.h"

namespace AliceO2 {
namespace roc {

/// TODO Currently a placeholder
class CruBar: public BarInterfaceBase
{
  public:
    CruBar(const Parameters& parameters);
    virtual ~CruBar();
    virtual CardType::type getCardType() override;
    virtual void checkReadSafe(int index) override;
    virtual void checkWriteSafe(int index, uint32_t value) override;
};

} // namespace roc
} // namespace AliceO2
