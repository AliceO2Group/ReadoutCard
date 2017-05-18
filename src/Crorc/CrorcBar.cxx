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

void CrorcBar::checkReadSafe(int)
{
}

void CrorcBar::checkWriteSafe(int, uint32_t)
{
}

} // namespace roc
} // namespace AliceO2
