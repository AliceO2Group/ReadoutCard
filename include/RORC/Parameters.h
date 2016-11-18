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
    /// Variant for the CardId parameter
    using CardIdVariant = boost::variant<int, ::AliceO2::Rorc::PciAddress>;
    using Variant = boost::variant<size_t, int32_t, bool, LoopbackMode::type, CardIdVariant>;
    using Key = std::string;
    using Map = std::map<Key, Variant>;

    /// Base type of parameters
    template <typename ValueType>
    struct Parameter
    {
        /// The type of the parameter's value
        using value_type = ValueType;
    };

    /// Helper macro to define Parameter types
    /// \param _value_type The type of the parameter's value
    /// \param _key_string The string for the parameter's map key
    /// \param _param_name The name of the parameter
#   define _DEFINE_PARAMETER(_value_type, _key_string, _param_name)\
      static constexpr struct _param_name : Parameter<_value_type>\
      {\
          static Key name() { return #_param_name; }\
          static Key key() { return _key_string; }\
      } _##_param_name = {}

    /// The serial number of the card.
//    _DEFINE_PARAMETER(int32_t, "serial_number", SerialNumber);

    /// An identifier of the card: either a serial number or PCI address
    _DEFINE_PARAMETER(CardIdVariant, "card_id", CardId);

    /// The number of the channel to open.
    _DEFINE_PARAMETER(int32_t, "channel_number", ChannelNumber);

    /// The size of the DMA buffer in bytes
    _DEFINE_PARAMETER(size_t, "dma_buffer_size", DmaBufferSize);

    /// The size of the DMA pages in bytes
    _DEFINE_PARAMETER(size_t, "dma_page_size", DmaPageSize);

    /// Enables or disables the internal data generator
    _DEFINE_PARAMETER(bool, "generator_enabled", GeneratorEnabled);

    /// Card loopback mode
    _DEFINE_PARAMETER(LoopbackMode::type, "generator_loopback", GeneratorLoopbackMode);

    /// Size of generator pages
    _DEFINE_PARAMETER(size_t, "generator_data_size", GeneratorDataSize);

    // Undefine macro for header safety
#   undef _DEFINE_PARAMETER

    /// Put a parameter in the map
    /// \tparam P The parameter type to put
    /// \param value The value to put
    /// Usage example:
    /// parameters.put<Parameters::SerialNumber>(12345);
    template <typename P>
    Parameters& put(const typename P::value_type& value)
    {
      mMap[P::key()] = value;
      return *this;
    }

    /// Get a parameter from the map
    /// \tparam P The parameter type to get
    /// \return The value wrapped in an optional if it was found, or boost::none if it was not
    /// Usage example:
    /// auto serial = parameters.get<Parameters::SerialNumber>().get_value_or(-1);
    template <typename P>
    boost::optional<typename P::value_type> get() const
    {
      auto iterator = mMap.find(P::key());
      if (iterator != mMap.end()) {
        Variant variant = iterator->second;
        auto value = boost::get<typename P::value_type>(variant);
        return value;
      } else {
        return boost::none;
      }
    }

    /// Get a parameter from the map, throwing an exception if it is not present
    /// \tparam P The parameter type to get
    /// \return The value
    /// Usage example:
    /// auto serial = parameters.getRequired<Parameters::SerialNumber>();
    template <typename P>
    typename P::value_type getRequired() const
    {
      if (auto optional = get<P>()) {
        return *optional;
      } else {
        BOOST_THROW_EXCEPTION(ParameterException()
            << errinfo_rorc_error_message("Parameter was not set")
            << errinfo_rorc_parameter_name(P::name())
            << errinfo_rorc_parameter_key(P::key()));
      }
    }

    /// Convenience function to make parameters object with card ID and channel number, since these are the most
    /// frequently used parameters
    static Parameters makeParameters(CardId::value_type cardId, ChannelNumber::value_type channel)
    {
      return Parameters()
          .put<Parameters::CardId>(cardId)
          .put<Parameters::ChannelNumber>(channel);
    }

    const Map& getMap() const
    {
      return mMap;
    }

  private:
    Map mMap;
};

} // namespace Rorc
} // namespace AliceO2
