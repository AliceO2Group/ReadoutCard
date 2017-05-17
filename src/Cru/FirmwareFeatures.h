#ifndef ALICEO2_READOUTCARD_SRC_FIRMWAREFEATURES_H
#define ALICEO2_READOUTCARD_SRC_FIRMWAREFEATURES_H

namespace AliceO2 {
namespace roc {

struct FirmwareFeatures
{
    /// Is the card's firmware a "standalone design"?
    bool standalone;

    /// Is the special register for loopback at BAR2 0x8000020 enabled?
    bool loopback0x8000020Bar2Register;

    /// Is the temperature sensor enabled?
    bool temperature;
};

} // namespace roc
} // namespace AliceO2

#endif // ALICEO2_READOUTCARD_SRC_FIRMWAREFEATURES_H
