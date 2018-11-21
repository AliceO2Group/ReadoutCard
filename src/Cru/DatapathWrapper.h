/// \file Cru/DatapathWrapper.h
/// \brief Definition of Datpath Wrapper
///
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#ifndef ALICEO2_READOUTCARD_CRU_DATAPATHWRAPPER_H_
#define ALICEO2_READOUTCARD_CRU_DATAPATHWRAPPER_H_

#include "Common.h"
#include "Pda/PdaBar.h"

namespace AliceO2 {
namespace roc {

using Link = Cru::Link;

class DatapathWrapper{

  public:
    DatapathWrapper(std::shared_ptr<Pda::PdaBar> pdaBar);

    /// Set links with a bitmask
    void setLinksEnabled(uint32_t dwrapper, uint32_t mask);
    void setLinkEnabled(Link link);
    void setDatapathMode(Link link, uint32_t mode);
    void setPacketArbitration(int wrapperCount, int arbitrationMode=0);
    void setFlowControl(int wrapper, int allowReject=0, int forceReject=0);

  private:
    uint32_t getDatapathWrapperBaseAddress(int wrapper);

    std::shared_ptr<Pda::PdaBar> mPdaBar;
};
} // namespace roc
} // namespace AliceO2

#endif // ALICEO2_READOUTCARD_CRU_DATAPATHWRAPPER_H_
