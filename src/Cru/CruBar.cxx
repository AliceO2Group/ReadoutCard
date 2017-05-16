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

void CruBar::checkReadSafe(int)
{
}

void CruBar::checkWriteSafe(int, uint32_t)
{
}

} // namespace roc
} // namespace AliceO2
