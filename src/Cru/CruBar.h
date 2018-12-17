/// \file CruBar.h
/// \brief Definition of the CruBar class.
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)
/// \author Kostas Alexopoulos (kostas.alexopoulos@cern.ch)

#ifndef ALICEO2_READOUTCARD_CRU_CRUBAR_H_
#define ALICEO2_READOUTCARD_CRU_CRUBAR_H_

#include <cstddef>
#include <set>
#include <boost/optional/optional.hpp>
#include "BarInterfaceBase.h"
#include "Common.h"
#include "Cru/Constants.h"
#include "Cru/FirmwareFeatures.h"
#include "ExceptionInternal.h"
#include "Pda/PdaBar.h"
#include "ReadoutCard/Parameters.h"
#include "Utilities/Util.h"

namespace AliceO2 {
namespace roc {

class CruBar final : public BarInterfaceBase
{
  using Link = Cru::Link;

  public:

    CruBar(const Parameters& parameters);
    CruBar(std::shared_ptr<Pda::PdaBar> bar);
    virtual ~CruBar();
    //virtual void checkReadSafe(int index) override;
    //virtual void checkWriteSafe(int index, uint32_t value) override;

    virtual CardType::type getCardType() override
    {
      return CardType::Cru;
    }

    virtual boost::optional<int32_t> getSerial() override;
    virtual boost::optional<float> getTemperature() override;
    virtual boost::optional<std::string> getFirmwareInfo() override;
    virtual boost::optional<std::string> getCardId() override;
    virtual int32_t getDroppedPackets() override;
    virtual uint32_t getCTPClock() override;
    virtual uint32_t getLocalClock() override;
    virtual int32_t getLinks() override;
    virtual int32_t getLinksPerWrapper(int wrapper) override;



    void pushSuperpageDescriptor(uint32_t link, uint32_t pages, uintptr_t busAddress);
    uint32_t getSuperpageCount(uint32_t link);
    void setDataEmulatorEnabled(bool enabled);
    void resetDataGeneratorCounter();
    void resetCard();
    void setDataGeneratorPattern(GeneratorPattern::type pattern, size_t size, bool randomEnabled);
    void dataGeneratorInjectError();
    void setDataSource(uint32_t source);
    FirmwareFeatures getFirmwareFeatures();
 
    static FirmwareFeatures convertToFirmwareFeatures(uint32_t reg);

    static void setDataGeneratorPatternBits(uint32_t& bits, GeneratorPattern::type pattern);

    static void setDataGeneratorSizeBits(uint32_t& bits, size_t size);
    static void setDataGeneratorEnableBits(uint32_t& bits, bool enabled);
    static void setDataGeneratorRandomSizeBits(uint32_t& bits, bool enabled);

    void setWrapperCount();
    void configure() override;
    Cru::ReportInfo report();

  private:
    uint32_t getSerialNumber();
    uint32_t getTemperatureRaw();
    boost::optional<float> convertTemperatureRaw(uint32_t registerValue);
    boost::optional<float> getTemperatureCelsius();
    uint32_t getFirmwareCompileInfo();
    uint32_t getFirmwareGitHash();
    uint32_t getFirmwareDateEpoch();
    uint32_t getFirmwareDate();
    uint32_t getFirmwareTime();
    uint32_t getFpgaChipHigh();
    uint32_t getFpgaChipLow();
    std::vector<Link> populateLinkList();
    void disableDataTaking();
    int getDdgBurstLength();
    //void checkParameters();

    FirmwareFeatures parseFirmwareFeatures(); 
    FirmwareFeatures mFeatures;

    Clock::type mClock;
    DatapathMode::type mDatapathMode;
    DownstreamData::type mDownstreamData;
    GbtMode::type mGbtMode;
    GbtMux::type mGbtMux;
    uint32_t mLoopback;
    int mWrapperCount = 0;
    std::set<uint32_t> mLinkMask;
    std::vector<Link> mLinkList;
    std::map<uint32_t, uint32_t> mRegisterMap;
    std::map<uint32_t, GbtMux::type> mGbtMuxMap;

    /// Checks if this is the correct BAR. Used to check for BAR 2 for special functions.
    void assertBarIndex(int index, std::string message) const
    {
      if (mPdaBar->getIndex() != index) {
        BOOST_THROW_EXCEPTION(Exception() << ErrorInfo::Message(message) << ErrorInfo::BarIndex(mPdaBar->getIndex()));
      }
    }
};

} // namespace roc
} // namespace AliceO2

#endif // ALICEO2_READOUTCARD_CRU_CRUBAR_H_
