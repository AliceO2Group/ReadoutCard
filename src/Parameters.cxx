/// \file Parameters.cxx
/// \brief Implementation of the RORC Parameters and associated functions
///
/// \author Pascal Boeschoten (pascal.boeschoten@cern.ch)

#include "RORC/Parameters.h"
#include <map>
#include <set>
#include <string>
#include <boost/optional.hpp>
#include <boost/variant.hpp>
#include "ExceptionInternal.h"
#include "RORC/LoopbackMode.h"
#include "RORC/PciAddress.h"

namespace AliceO2 {
namespace Rorc {
namespace {
  template <class T>
  void setParam(Parameters::Map& map, std::string key, const T& value)
  {
      map[key] = value;
  }

  /// Get a parameter from the map
  /// \tparam T The parameter type to get
  /// \return The value wrapped in an optional if it was found, or boost::none if it was not
  template <typename T>
  auto getParam(const Parameters::Map& map, std::string key) -> boost::optional<T>
  {
    auto iterator = map.find(key);
    if (iterator != map.end()) {
      auto variant = iterator->second;
      auto value = boost::get<T>(variant);
      return value;
    } else {
      return boost::none;
    }
  }

  /// Get a parameter from the map, throwing an exception if it is not present
  /// \tparam T The parameter type to get
  /// \return The value
  template <typename T>
  auto getParamRequired(const Parameters::Map& map, std::string key) -> T
  {
    if (auto optional = getParam<T>(map, key)) {
      return *optional;
    } else {
      BOOST_THROW_EXCEPTION(ParameterException()
          << ErrorInfo::Message("Parameter was not set")
          << ErrorInfo::ParameterKey(key));
    }
  }
} // Anonymous namespace

/// Helper macro to implement getters/setters
/// \param _param_name The name of the parameter
/// \param _key_string The string for the parameter's map key
#   define _PARAMETER_FUNCTIONS(_param_name, _key_string)\
  auto Parameters::set##_param_name(_param_name##Type value) -> Parameters&\
  {\
    setParam(mMap, _key_string, value);\
    return *this;\
  }\
  \
  auto Parameters::get##_param_name() const -> boost::optional<_param_name##Type>\
  {\
    return getParam<_param_name##Type>(mMap, _key_string);\
  }\
  \
  auto Parameters::get##_param_name##Required() const -> _param_name##Type\
  {\
    return getParamRequired<_param_name##Type>(mMap, _key_string);\
  }\

_PARAMETER_FUNCTIONS(CardId, "card_id")
_PARAMETER_FUNCTIONS(ChannelNumber, "channel_number")
_PARAMETER_FUNCTIONS(DmaBufferSize, "dma_buffer_size")
_PARAMETER_FUNCTIONS(DmaPageSize, "dma_page_size")
_PARAMETER_FUNCTIONS(GeneratorEnabled, "generator_enabled")
_PARAMETER_FUNCTIONS(GeneratorLoopback, "generator_loopback")
_PARAMETER_FUNCTIONS(GeneratorDataSize, "generator_data_size")
_PARAMETER_FUNCTIONS(BufferParameters, "buffer_parameters")

#undef _PARAMETER_FUNCTIONS

} // namespace Rorc
} // namespace AliceO2
