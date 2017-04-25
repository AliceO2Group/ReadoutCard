/// \file Parameters.h
/// \brief Definition of the RORC Parameters and associated functions
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#ifndef ALICEO2_INCLUDE_RORC_PARAMETERS_H_
#define ALICEO2_INCLUDE_RORC_PARAMETERS_H_

#include <map>
#include <string>
#include <boost/optional.hpp>
#include <boost/variant.hpp>
#include "RORC/ParameterTypes/BufferParameters.h"
#include "RORC/ParameterTypes/GeneratorPattern.h"
#include "RORC/ParameterTypes/LoopbackMode.h"
#include "RORC/ParameterTypes/PciAddress.h"
#include "RORC/ParameterTypes/ReadoutMode.h"

namespace AliceO2 {
namespace Rorc {

/// Class that holds parameters for channels
/// Per parameter, it has three functions:
/// * Setter
/// * Non-throwing getter that returns the value wrapped in an optional if it is present, or an empty optional if it
///   was not
/// * Throwing getter that returns the value if it is present, or throws a ParameterException
class Parameters
{
  public:
    /// Macro for defining functions to get/set a parameter
    #define DEFINE_ALICEO2_RORC_PARAMETER_FUNCTIONS(_name, _type)\
      using _name##Type = _type;\
      auto set##_name(_type value) -> Parameters&;\
      auto get##_name() const -> boost::optional<_type>;\
      auto get##_name##Required() const -> _type;
    #undef DEFINE_ALICEO2_RORC_PARAMETER_FUNCTIONS

    // Types for parameter values

    /// Type for buffer parameters
    using BufferParametersType = boost::variant<BufferParameters::Memory, BufferParameters::File>;

    /// Type for the CardId parameter. It can hold either a serial number or PciAddress.
    using CardIdType = boost::variant<int, ::AliceO2::Rorc::PciAddress>;

    /// Type for the ChannelNumber parameter
    using ChannelNumberType = int32_t;

    /// Type for the DMA page size parameter
    using DmaPageSizeType = size_t;

    /// Type for the generator enabled parameter
    using GeneratorEnabledType = bool;

    /// Type for the generator data size parameter
    using GeneratorDataSizeType = size_t;

    /// Type for the LoopbackMode parameter
    using GeneratorLoopbackType = LoopbackMode::type;

    /// Type for the generator pattern parameter
    using GeneratorPatternType = GeneratorPattern::type;

    /// Type for the readout mode parameter
    using ReadoutModeType = ReadoutMode::type;

    // Setters

    /// Sets the CardId parameter
    /// \param value The value to set
    /// \return Reference to this object for chaining calls
    auto setCardId(CardIdType value) -> Parameters&;

    /// Sets the ChannelNumber parameter
    /// \param value The value to set
    /// \return Reference to this object for chaining calls
    auto setChannelNumber(ChannelNumberType value) -> Parameters&;

    /// Sets the DmaPageSize parameter
    /// \param value The value to set
    /// \return Reference to this object for chaining calls
    auto setDmaPageSize(DmaPageSizeType value) -> Parameters&;

    /// Sets the GeneratorEnabled parameter
    /// \param value The value to set
    /// \return Reference to this object for chaining calls
    auto setGeneratorEnabled(GeneratorEnabledType value) -> Parameters&;

    /// Sets the GeneratorDataSize parameter
    /// \param value The value to set
    /// \return Reference to this object for chaining calls
    auto setGeneratorDataSize(GeneratorDataSizeType value) -> Parameters&;

    /// Sets the GeneratorLoopback parameter
    /// \param value The value to set
    /// \return Reference to this object for chaining calls
    auto setGeneratorLoopback(GeneratorLoopbackType value) -> Parameters&;

    /// Sets the GeneratorPattern parameter
    /// \param value The value to set
    /// \return Reference to this object for chaining calls
    auto setGeneratorPattern(GeneratorPatternType value) -> Parameters&;

    /// Sets the BufferParameters parameter
    /// \param value The value to set
    /// \return Reference to this object for chaining calls
    auto setBufferParameters(BufferParametersType value) -> Parameters&;

    /// Sets the ReadoutMode parameter
    /// \param value The value to set
    /// \return Reference to this object for chaining calls
    auto setReadoutMode(ReadoutModeType value) -> Parameters&;

    // Non-throwing getters

    /// Gets the CardId parameter
    /// \return The value wrapped in an optional if it is present, or an empty optional if it was not
    auto getCardId() const -> boost::optional<CardIdType>;

    /// Gets the ChannelNumber parameter
    /// \return The value wrapped in an optional if it is present, or an empty optional if it was not
    auto getChannelNumber() const -> boost::optional<ChannelNumberType>;

    /// Gets the DmaPageSize parameter
    /// \return The value wrapped in an optional if it is present, or an empty optional if it was not
    auto getDmaPageSize() const -> boost::optional<DmaPageSizeType>;

    /// Gets the GeneratorEnabled parameter
    /// \return The value wrapped in an optional if it is present, or an empty optional if it was not
    auto getGeneratorEnabled() const -> boost::optional<GeneratorEnabledType>;

    /// Gets the GeneratorDataSize parameter
    /// \return The value wrapped in an optional if it is present, or an empty optional if it was not
    auto getGeneratorDataSize() const -> boost::optional<GeneratorDataSizeType>;

    /// Gets the GeneratorLoopback parameter
    /// \return The value wrapped in an optional if it is present, or an empty optional if it was not
    auto getGeneratorLoopback() const -> boost::optional<GeneratorLoopbackType>;

    /// Gets the GeneratorPattern parameter
    /// \return The value wrapped in an optional if it is present, or an empty optional if it was not
    auto getGeneratorPattern() const -> boost::optional<GeneratorPatternType>;

    /// Gets the BufferParameters parameter
    /// \return The value wrapped in an optional if it is present, or an empty optional if it was not
    auto getBufferParameters() const -> boost::optional<BufferParametersType>;

    /// Gets the ReadoutMode parameter
    /// \return The value wrapped in an optional if it is present, or an empty optional if it was not
    auto getReadoutMode() const -> boost::optional<ReadoutModeType>;

    // Throwing getters

    /// Gets the CardId parameter
    /// \exception ParameterException The parameter was not present
    /// \return The value
    auto getCardIdRequired() const -> CardIdType;

    /// Gets the ChannelNumber parameter
    /// \exception ParameterException The parameter was not present
    /// \return The value
    auto getChannelNumberRequired() const -> ChannelNumberType;

    /// Gets the DmaPageSize parameter
    /// \exception ParameterException The parameter was not present
    /// \return The value
    auto getDmaPageSizeRequired() const -> DmaPageSizeType;

    /// Gets the GeneratorEnabled parameter
    /// \exception ParameterException The parameter was not present
    /// \return The value
    auto getGeneratorEnabledRequired() const -> GeneratorEnabledType;

    /// Gets the GeneratorDataSize parameter
    /// \exception ParameterException The parameter was not present
    /// \return The value
    auto getGeneratorDataSizeRequired() const -> GeneratorDataSizeType;

    /// Gets the GeneratorLoopback parameter
    /// \exception ParameterException The parameter was not present
    /// \return The value
    auto getGeneratorLoopbackRequired() const -> GeneratorLoopbackType;

    /// Gets the GeneratorPattern parameter
    /// \exception ParameterException The parameter was not present
    /// \return The value
    auto getGeneratorPatternRequired() const -> GeneratorPatternType;

    /// Gets the BufferParameters parameter
    /// \exception ParameterException The parameter was not present
    /// \return The value
    auto getBufferParametersRequired() const -> BufferParametersType;

    /// Gets the ReadoutMode parameter
    /// \exception ParameterException The parameter was not present
    /// \return The value
    auto getReadoutModeRequired() const -> ReadoutModeType;

    // Helper functions

    /// Convenience function to make a Parameters object with card ID and channel number, since these are the most
    /// frequently used parameters
    static Parameters makeParameters(CardIdType cardId, ChannelNumberType channel)
    {
      return Parameters().setCardId(cardId).setChannelNumber(channel);
    }

  private:
    /// Variant used for internal storage of parameters
    using Variant = boost::variant<size_t, int32_t, bool, BufferParametersType, CardIdType, GeneratorLoopbackType,
        GeneratorPatternType, ReadoutModeType>;

    /// Map used for internal storage of parameters
    using Map = std::map<std::string, Variant>;

    /// Map for storage of parameters
    Map mMap;
};

} // namespace Rorc
} // namespace AliceO2

#endif // ALICEO2_INCLUDE_RORC_PARAMETERS_H_
