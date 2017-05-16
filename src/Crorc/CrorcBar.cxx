/// \file CrorcBar.cxx
/// \brief Implementation of the CrorcBar class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include "Crorc/CrorcBar.h"

namespace AliceO2 {
namespace roc {

CrorcBar::CrorcBar(const Parameters& parameters)
    : BarInterfaceBase(parameters)
{
}

CrorcBar::~CrorcBar()
{
}

CardType::type CrorcBar::getCardType()
{
  return CardType::Crorc;
}

void CrorcBar::checkReadSafe(int)
{
}

void CrorcBar::checkWriteSafe(int, uint32_t)
{
}

} // namespace roc
} // namespace AliceO2
