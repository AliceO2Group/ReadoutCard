/// \file CrorcBar.h
/// \brief Definition of the CrorcBar class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#pragma once

#include "BarInterfaceBase.h"

namespace AliceO2 {
namespace roc {

/// TODO Currently a placeholder
class CrorcBar: public BarInterfaceBase
{
  public:

    CrorcBar(const Parameters& parameters);
    virtual ~CrorcBar();
    virtual CardType::type getCardType() override;
    virtual void checkReadSafe(int index) override;
    virtual void checkWriteSafe(int index, uint32_t value) override;
};

} // namespace roc
} // namespace AliceO2
