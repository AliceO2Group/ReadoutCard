/// \file CruBar.cxx
/// \brief Implementation of the CruBar class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include "CruBar.h"

namespace AliceO2 {
namespace roc {

CruBar::CruBar(const Parameters& parameters)
    : BarInterfaceBase(parameters)
{
}

CruBar::~CruBar()
{
}

CardType::type CruBar::getCardType()
{
  return CardType::Cru;
}

void CruBar::checkReadSafe(int index)
{
}

void CruBar::checkWriteSafe(int index, uint32_t value)
{
}

} // namespace roc
} // namespace AliceO2
