///
/// \file RorcDeviceEnumerator.h
/// \author Pascal Boeschoten
///

#pragma once

#include <string>
#include <vector>
#include "RORC/CardType.h"

namespace AliceO2 {
namespace Rorc {

/// Attempts to list RORC PCI devices
/// Note: some duplication of code with RorcDevice TODO refactor a bit
class RorcDeviceEnumerator
{
  public:
    struct CardDescriptor
    {
        CardType::type cardType;
        int serialNumber;
        std::string deviceId;
        std::string vendorId;
    };

    // Finds all RORC devices
    RorcDeviceEnumerator();

    // Finds RORC device with given serial number
    RorcDeviceEnumerator(int serialNumber);

    ~RorcDeviceEnumerator();

    const std::vector<CardDescriptor>& getCardsFound() const
    {
      return cardsFound;
    }

  private:
    std::vector<CardDescriptor> cardsFound;
};

} // namespace Rorc
} // namespace AliceO2
