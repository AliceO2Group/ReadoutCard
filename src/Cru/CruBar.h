/// \file CruBar.h
/// \brief Definition of the CruBar class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#ifndef ALICEO2_READOUTCARD_CRU_CRUBAR_H_
#define ALICEO2_READOUTCARD_CRU_CRUBAR_H_

#include "BarInterfaceBase.h"

namespace AliceO2 {
namespace roc {

/// TODO Currently a placeholder
class CruBar final : public BarInterfaceBase
{
  public:
    CruBar(const Parameters& parameters);
    virtual ~CruBar();
    virtual void checkReadSafe(int index) override;
    virtual void checkWriteSafe(int index, uint32_t value) override;

    virtual CardType::type getCardType() override
    {
      return CardType::Cru;
    }
};

} // namespace roc
} // namespace AliceO2

#endif // ALICEO2_READOUTCARD_CRU_CRUBAR_H_