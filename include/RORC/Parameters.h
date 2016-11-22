/// \file Parameters.h
/// \brief Definition of the RORC Parameters and associated functions
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#pragma once

#include <map>
#include <set>
#include <string>
#include <boost/optional.hpp>
#include <boost/variant.hpp>
#include "RORC/Exception.h"
#include "RORC/LoopbackMode.h"
#include "RORC/PciAddress.h"

namespace AliceO2 {
namespace Rorc {

/// Class for channel parameters
class Parameters
{
  public:
    /// Variant for the CardId parameter. It can be either a serial number or PciAddress.
    using CardIdVariant = boost::variant<int, ::AliceO2::Rorc::PciAddress>;

    using CardIdType = CardIdVariant;
    auto setCardId(CardIdType value) -> Parameters&;
    auto getCardId() const -> boost::optional<CardIdType>;
    auto getCardIdRequired() const -> CardIdType;

    using ChannelNumberType = int32_t;
    auto setChannelNumber(ChannelNumberType value) -> Parameters&;
    auto getChannelNumber() const -> boost::optional<ChannelNumberType>;
    auto getChannelNumberRequired() const -> ChannelNumberType;

    using DmaBufferSizeType = size_t;
    auto setDmaBufferSize(DmaBufferSizeType value) -> Parameters&;
    auto getDmaBufferSize() const -> boost::optional<DmaBufferSizeType>;
    auto getDmaBufferSizeRequired() const -> DmaBufferSizeType;

    using DmaPageSizeType = size_t;
    auto setDmaPageSize(DmaPageSizeType value) -> Parameters&;
    auto getDmaPageSize() const -> boost::optional<DmaPageSizeType>;
    auto getDmaPageSizeRequired() const -> DmaPageSizeType;

    using GeneratorEnabledType = bool;
    auto setGeneratorEnabled(GeneratorEnabledType value) -> Parameters&;
    auto getGeneratorEnabled() const -> boost::optional<GeneratorEnabledType>;
    auto getGeneratorEnabledRequired() const -> GeneratorEnabledType;

    using GeneratorLoopbackType = LoopbackMode::type;
    auto setGeneratorLoopback(GeneratorLoopbackType value) -> Parameters&;
    auto getGeneratorLoopback() const -> boost::optional<GeneratorLoopbackType>;
    auto getGeneratorLoopbackRequired() const -> GeneratorLoopbackType;

    using GeneratorDataSizeType = size_t;
    auto setGeneratorDataSize(GeneratorDataSizeType value) -> Parameters&;
    auto getGeneratorDataSize() const -> boost::optional<GeneratorDataSizeType>;
    auto getGeneratorDataSizeRequired() const -> GeneratorDataSizeType;

    /// Convenience function to make parameters object with card ID and channel number, since these are the most
    /// frequently used parameters
    static Parameters makeParameters(CardIdType cardId, ChannelNumberType channel)
    {
      return Parameters().setCardId(cardId).setChannelNumber(channel);
    }

  private:
    /// Variant used for internal storage of parameters
    using Variant = boost::variant<size_t, int32_t, bool, LoopbackMode::type, CardIdVariant>;

    /// Map used for internal storage of parameters
    using Map = std::map<std::string, Variant>;

    /// Map for storage of parameters
    Map mMap;
};

} // namespace Rorc
} // namespace AliceO2
