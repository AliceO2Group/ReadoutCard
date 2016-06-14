///
/// \file CardFactory.h
/// \author Pascal Boeschoten
///

#pragma once

#include <memory>
#include "RORC/CardInterface.h"

namespace AliceO2 {
namespace Rorc {

/// Factory class for creating objects to access RORC cards
class CardFactory
{
  public:
    static constexpr int DUMMY_SERIAL_NUMBER = -1;

    CardFactory();
    virtual ~CardFactory();

    /// Get a card with the given serial number. It is not yet implemented fully, currently it will just pick the
    /// first CRORC it comes across. If the PDA dependency is not available, a dummy implementation will be returned.
    /// \param serialNumber The serial number of the card. Passing 'DUMMY_SERIAL_NUMBER' returns a dummy implementation
    std::shared_ptr<CardInterface> getCardFromSerialNumber(int serialNumber);
};

} // namespace Rorc
} // namespace AliceO2

