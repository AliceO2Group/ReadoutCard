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

namespace AliceO2 {
namespace roc {

class CrorcBar final : public BarInterfaceBase
{
  public:
    CrorcBar(const Parameters& parameters);
    virtual ~CrorcBar();
    //virtual void checkReadSafe(int index) override;
    //virtual void checkWriteSafe(int index, uint32_t value) override;

    virtual CardType::type getCardType() override
    {
      return CardType::Crorc;
    }

    virtual boost::optional<int32_t> getSerial() override;
    virtual boost::optional<std::string> getFirmwareInfo() override;
};

} // namespace roc
} // namespace AliceO2

#endif // ALICEO2_SRC_READOUTCARD_CRORC_CRORCBAR_H_
