#ifndef ALICEO2_READOUTCARD_SRC_FIRMWAREFEATURES_H
#define ALICEO2_READOUTCARD_SRC_FIRMWAREFEATURES_H

namespace AliceO2 {
namespace roc {

struct FirmwareFeatures
{
    /// Is the card's firmware a "standalone design"?
    bool standalone = false;

    /// Is a serial number available?
    bool serial = false;

    /// Is firmware information available?
    bool firmwareInfo = false;

    /// Is the special register for loopback at BAR2 0x8000020 enabled?
    bool dataSelection = false;

    /// Is the temperature sensor enabled?
    bool temperature = false;
};

} // namespace roc
} // namespace AliceO2

#endif // ALICEO2_READOUTCARD_SRC_FIRMWAREFEATURES_H
